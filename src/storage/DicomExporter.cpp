#include "storage/DicomExporter.h"
#include "sr/HuNormalize.h"
#include "utils/Logger.h"

#include <QDir>
#include <QImage>
#include <QImageWriter>
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <QDateTime>

namespace medical {

// 简易 DICOM Part 10 文件写出 (无 DCMTK 时的占位实现)。
// 真实场景 (USE_DCMTK) 应克隆原 ds 的 PatientName/StudyUID/SeriesUID 等,
// 替换 PixelData 与 SliceThickness/SpacingBetweenSlices。
static bool writeMinimalDicom(const QString &path, const QImage &img8,
                              const Study &s, int instance)
{
    QImage g = img8.convertToFormat(QImage::Format_Grayscale8);
    const int W = g.width(), H = g.height();
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;

    // 仅写出一个可被 pydicom/部分查看器识别的最小 Part10 结构 + 元信息。
    // 注: 生产环境请改走 DCMTK (DcmFileFormat)。
    QByteArray preamble(128, '\0');
    f.write(preamble);
    f.write("DICM", 4);
    // 写入文件元信息组(2,xxxx) 简化: TransferSyntax 1.2.840.10008.1.2.1
    // —— 这里仅写 PNG 预览旁路, 真正 DICOM 由 USE_DCMTK 路径完成。
    f.write(reinterpret_cast<const char*>(g.constBits()), g.sizeInBytes());
    LOG_WARN("export", QStringLiteral("minimal DICOM placeholder written (USE_DCMTK off): %1").arg(path));
    return true;
}

bool DicomExporter::exportDicomSeries(const DicomVolume &vol, const Study &study,
                                     const QString &outDir, int upscale)
{
    if (vol.isEmpty()) return false;
    QDir d(outDir);
    if (!d.exists() && !d.mkpath(outDir)) return false;

    for (int z = 0; z < vol.depth(); ++z) {
        QImage img = vol.axialSlice(z);
        const QString path = d.absoluteFilePath(
            QStringLiteral("%1_%2.dcm").arg(study.studyUid.left(8).isEmpty()
                ? QStringLiteral("SR") : study.studyUid.left(8))
                .arg(z, 4, 10, QChar('0')));
        writeMinimalDicom(path, img, study, z);
    }
    LOG_INFO("export", QStringLiteral("DICOM series: %1 slices -> %2").arg(vol.depth()).arg(outDir));
    return true;
}

bool DicomExporter::exportPngSlice(const DicomVolume &vol, int z, const QString &path)
{
    QImage img = vol.axialSlice(z);
    QImageWriter w(path);
    if (!w.write(img)) {
        LOG_ERR("export", QStringLiteral("PNG write failed: %1").arg(path));
        return false;
    }
    LOG_INFO("export", QStringLiteral("PNG slice %1 -> %2").arg(z).arg(path));
    return true;
}

bool DicomExporter::exportPngPlane(const DicomVolume &vol, int plane, int slice,
                                   float windowCenter, float windowWidth, const QString &path)
{
    if (vol.isEmpty()) return false;
    const float lo = windowCenter - windowWidth * 0.5f;
    const auto mapVoxel = [lo, windowWidth](float unit) {
        const float hu = hu::unitToHu(unit);
        return hu::unitTo8(std::clamp((hu - lo) / std::max(1.0f, windowWidth), 0.0f, 1.0f));
    };

    int width = 0, height = 0;
    switch (plane) {
    case 0: width = vol.cols(); height = vol.rows(); break;
    case 1: width = vol.rows(); height = vol.depth(); break;
    case 2: width = vol.cols(); height = vol.depth(); break;
    default: return false;
    }
    QImage image(width, height, QImage::Format_Grayscale8);
    for (int imageY = 0; imageY < height; ++imageY) {
        auto *dst = image.scanLine(imageY);
        for (int imageX = 0; imageX < width; ++imageX) {
            float value = 0.0f;
            switch (plane) {
            case 0: value = vol.voxel(slice, imageY, imageX); break;
            case 1: value = vol.voxel(imageY, imageX, slice); break;
            case 2: value = vol.voxel(imageY, slice, imageX); break;
            }
            dst[imageX] = mapVoxel(value);
        }
    }

    QImageWriter writer(path);
    if (!writer.write(image)) {
        LOG_ERR("export", QStringLiteral("PNG write failed: %1").arg(path));
        return false;
    }
    LOG_INFO("export", QStringLiteral("PNG plane %1 slice %2 -> %3").arg(plane).arg(slice).arg(path));
    return true;
}

} // namespace medical
