#include "dicom/DcmtkLoader.h"
#include "utils/Logger.h"
#include "core/Study.h"

#ifdef USE_DCMTK

#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmimgle/dcmimage.h>
#include <dcmtk/dcmimgle/dipixel.h>   // DiPixel (getInterData)

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QDateTime>
#include <algorithm>
#include <cmath>
#include <vector>

namespace medical {

DcmtkLoader::DcmtkLoader() = default;
DcmtkLoader::~DcmtkLoader() = default;

namespace {

QString getS(DcmDataset *ds, const DcmTagKey &key)
{
    const char *v = nullptr;
    if (ds && ds->findAndGetString(key, v).good() && v)
        return QString::fromLatin1(v);
    return {};
}
double getDf(DcmDataset *ds, const DcmTagKey &key, double def = 0.0)
{
    Float64 v;
    if (ds && ds->findAndGetFloat64(key, v).good()) return double(v);
    return def;
}
Uint16 getU16(DcmDataset *ds, const DcmTagKey &key, Uint16 def = 0)
{
    Uint16 v;
    if (ds && ds->findAndGetUint16(key, v).good()) return v;
    return def;
}
Sint32 getS32(DcmDataset *ds, const DcmTagKey &key, Sint32 def = 0)
{
    Sint32 v;
    if (ds && ds->findAndGetSint32(key, v).good()) return v;
    return def;
}

// 读取 ImagePositionPatient(0020,0032) 与 ImageOrientationPatient(0020,0037),
// 计算切片在法线方向上的投影位置(用于按真实层位排序)。
// 返回是否成功取到位置; pos 存放投影值。
bool slicePos(DcmDataset *ds, double &pos)
{
    if (!ds) return false;
    Float64 px, py, pz;
    if (ds->findAndGetFloat64(DCM_ImagePositionPatient, px, 0).bad() ||
        ds->findAndGetFloat64(DCM_ImagePositionPatient, py, 1).bad() ||
        ds->findAndGetFloat64(DCM_ImagePositionPatient, pz, 2).bad())
        return false;
    Float64 rx, ry, rz, cx, cy, cz;
    if (ds->findAndGetFloat64(DCM_ImageOrientationPatient, rx, 0).bad() ||
        ds->findAndGetFloat64(DCM_ImageOrientationPatient, ry, 1).bad() ||
        ds->findAndGetFloat64(DCM_ImageOrientationPatient, rz, 2).bad() ||
        ds->findAndGetFloat64(DCM_ImageOrientationPatient, cx, 3).bad() ||
        ds->findAndGetFloat64(DCM_ImageOrientationPatient, cy, 4).bad() ||
        ds->findAndGetFloat64(DCM_ImageOrientationPatient, cz, 5).bad())
        return false;
    // 法线 = 行方向 × 列方向
    const double nx = ry * cz - rz * cy;
    const double ny = rz * cx - rx * cz;
    const double nz = rx * cy - ry * cx;
    pos = px * nx + py * ny + pz * nz;
    return true;
}

// 将一帧像素填入 DicomFrame.rawPixels (约定 = HU + 1000) 并渲染 8-bit image
void fillFrame(DicomFrame &f, int W, int H, const std::vector<int> &hu,
               float wc, float ww)
{
    f.width = W; f.height = H;
    f.rawPixels.resize(W * H);
    for (int i = 0; i < W * H; ++i)
        f.rawPixels[i] = quint16(std::clamp(hu[size_t(i)] + 1000, 0, 65535));

    QImage img(W, H, QImage::Format_Grayscale8);
    const float lo = wc - ww * 0.5f;
    for (int y = 0; y < H; ++y) {
        auto *dst = img.scanLine(y);
        for (int x = 0; x < W; ++x) {
            const float hv = float(hu[size_t(y * W + x)]);
            const float v = std::clamp((hv - lo) / std::max(1.0f, ww), 0.0f, 1.0f);
            dst[x] = quint8(v * 255.0f);
        }
    }
    f.image = img;
}

bool loadOneFile(const QString &path, DicomFrame &out, Study &study, bool &studyFilled)
{
    DcmFileFormat ff;
    const auto *const fn = path.toLocal8Bit().constData();
    if (ff.loadFile(fn).bad()) {
        LOG_WARN("dcmtk", QStringLiteral("skip (not DICOM?): %1").arg(path));
        return false;
    }
    DcmDataset *ds = ff.getDataset();

    if (!studyFilled) {
        study.patient.name       = getS(ds, DCM_PatientName);
        study.patient.patientId  = getS(ds, DCM_PatientID);
        study.patient.sex         = getS(ds, DCM_PatientSex);
        study.patient.age        = getS(ds, DCM_PatientAge).replace(QStringLiteral("Y"),QString()).toInt();
        study.patient.birthDate   = getS(ds, DCM_PatientBirthDate);
        study.studyUid     = getS(ds, DCM_StudyInstanceUID);
        study.accessionNumber = getS(ds, DCM_AccessionNumber);
        study.modality     = getS(ds, DCM_Modality);
        study.description  = getS(ds, DCM_StudyDescription);
        if (study.modality.isEmpty()) study.modality = QStringLiteral("CT");
        studyFilled = true;
    }

    const QString seriesUid = getS(ds, DCM_SeriesInstanceUID);

    const int H = int(getU16(ds, DCM_Rows)), W = int(getU16(ds, DCM_Columns));
    if (W <= 0 || H <= 0) { LOG_WARN("dcmtk", "no Rows/Cols: " + path); return false; }

    double slope = 1.0, intercept = 0.0;
    { Float64 s, c;
      if (ds->findAndGetFloat64(DCM_RescaleSlope, s).good())     slope = double(s);
      if (ds->findAndGetFloat64(DCM_RescaleIntercept, c).good()) intercept = double(c); }

    double spacingX = 0.7, spacingY = 0.7, thick = 1.25;
    { Float64 v;
      if (ds->findAndGetFloat64(DCM_PixelSpacing, v, 0).good()) spacingX = double(v);
      if (ds->findAndGetFloat64(DCM_PixelSpacing, v, 1).good()) spacingY = double(v);
      if (ds->findAndGetFloat64(DCM_SliceThickness, v).good())   thick   = double(v); }
    float wc = -600.0f, ww = 1500.0f;
    { Float64 c, w;
      if (ds->findAndGetFloat64(DCM_WindowCenter, c).good()) wc = float(c);
      if (ds->findAndGetFloat64(DCM_WindowWidth, w).good())  ww = float(w); }

    std::vector<int> hu(size_t(W) * H, 0);

    // 优先: 未压缩 OW 直接取原始存储值 -> HU = stored*slope + intercept
    const Uint16 *raw16 = nullptr;
    unsigned long count = 0;
    if (ds->findAndGetUint16Array(DCM_PixelData, raw16, &count).good() && raw16
        && count >= static_cast<unsigned long>(W * H)) {
        for (int i = 0; i < W * H; ++i)
            hu[size_t(i)] = int(std::round(double(raw16[i]) * slope + intercept));
    } else {
        DicomImage dimg(ds, EXS_Unknown, 0);
        if (dimg.getStatus() != EIS_Normal) {
            LOG_WARN("dcmtk", QStringLiteral("decode failed: %1 (status=%2)")
                                .arg(path).arg(int(dimg.getStatus())));
            return false;
        }
        const DiPixel *inter = dimg.getInterData();
        if (!inter || inter->getCount() < size_t(W * H)) {
            LOG_WARN("dcmtk", "no inter data: " + path); return false;
        }
        const EP_Representation rep = inter->getRepresentation();
        const void *p = inter->getData();
        const size_t n = size_t(W) * H;
        if (rep == EPR_Uint16) {
            const Uint16 *u = static_cast<const Uint16*>(p);
            for (size_t i = 0; i < n; ++i) hu[i] = int(u[i]);
        } else if (rep == EPR_Sint16) {
            const Sint16 *s = static_cast<const Sint16*>(p);
            for (size_t i = 0; i < n; ++i) hu[i] = int(s[i]);
        } else if (rep == EPR_Uint8) {
            const Uint8 *u = static_cast<const Uint8*>(p);
            for (size_t i = 0; i < n; ++i) hu[i] = int(u[i]);
        } else {
            LOG_WARN("dcmtk", "unsupported pixel representation: " + path); return false;
        }
    }

    DicomFrame f;
    f.spacingX = float(spacingX); f.spacingY = float(spacingY);
    f.sliceThickness = float(thick);
    f.defaultWindow = { wc, ww };
    f.seriesUid = seriesUid;
    f.sourceFile = path;
    f.instanceNumber = int(getS32(ds, DCM_InstanceNumber, -1));
    { double sp = 0.0;
      if (slicePos(ds, sp)) { f.slicePosition = sp; f.hasSlicePosition = true; } }
    fillFrame(f, W, H, hu, wc, ww);
    out = std::move(f);
    return true;
}

QStringList collectFiles(const QString &path)
{
    QFileInfo fi(path);
    if (fi.isFile()) return { path };
    if (!fi.isDir()) return {};
    QStringList out;
    for (const auto &e : QDir(path).entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name)) {
        const QString s = e.suffix().toLower();
        if (s.isEmpty() || s == "dcm" || s == "ima" || s == "dicom" || s == "dc3")
            out << e.absoluteFilePath();
    }
    return out;
}

} // namespace

bool DcmtkLoader::load(const QString &path, Study &studyOut)
{
    const QStringList files = collectFiles(path);
    if (files.isEmpty()) { LOG_ERR("dcmtk", "no candidate files: " + path); return false; }
    LOG_INFO("dcmtk", QStringLiteral("scanning %1 files in %2").arg(files.size()).arg(path));

    std::vector<DicomFrame> items;
    bool studyFilled = false;
    for (const QString &f : files) {
        DicomFrame frame;
        if (loadOneFile(f, frame, studyOut, studyFilled))
            items.push_back(std::move(frame));
    }
    if (items.empty()) { LOG_ERR("dcmtk", "no valid DICOM frames parsed"); return false; }

    // 按真实层位排序, 而非文件名/UID 的字典序:
    //  1) seriesUid 分组(多序列场景)
    //  2) ImagePositionPatient 在法线方向的投影(真实空间层位)
    //  3) InstanceNumber 数值(影像显示顺序约定)
    //  4) 源文件名(最终兜底)
    std::sort(items.begin(), items.end(), [](const DicomFrame &a, const DicomFrame &b){
        if (a.seriesUid != b.seriesUid) return a.seriesUid < b.seriesUid;
        if (a.hasSlicePosition && b.hasSlicePosition) {
            if (a.slicePosition != b.slicePosition)
                return a.slicePosition < b.slicePosition;
        } else if (a.hasSlicePosition != b.hasSlicePosition) {
            return a.hasSlicePosition; // 有位置的排前, 无位置的兜底
        }
        if (a.instanceNumber != b.instanceNumber)
            return a.instanceNumber < b.instanceNumber;
        return a.sourceFile < b.sourceFile;
    });
    m_frames.clear();
    m_frames.reserve(int(items.size()));
    for (auto &it : items) m_frames.append(std::move(it));

    studyOut.seriesCount = 1;
    studyOut.frameCount  = int(m_frames.size());
    if (studyOut.dateTime.isNull()) studyOut.dateTime = QDateTime::currentDateTime();
    LOG_INFO("dcmtk", QStringLiteral("loaded %1 frames, modality=%2, %3×%4")
                       .arg(m_frames.size()).arg(studyOut.modality)
                       .arg(m_frames.isEmpty()?0:m_frames[0].width)
                       .arg(m_frames.isEmpty()?0:m_frames[0].height));
    return true;
}

} // namespace medical
#endif // USE_DCMTK
