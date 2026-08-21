// 生成合成 CT 序列 (.dcm) 用于验证 DcmtkLoader —— 不随主程序构建, 手动编译。
// 用法: gen_test_dicom <out_dir> <numSlices> <rows> <cols>
#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dctypes.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <direct.h>
#include <sys/stat.h>

static void putUI(DcmDataset *d, const DcmTagKey &k, const char *v){ d->putAndInsertString(k, v); }
static void putSH(DcmDataset *d, const DcmTagKey &k, const char *v){ d->putAndInsertString(k, v); }
static void putLO(DcmDataset *d, const DcmTagKey &k, const char *v){ d->putAndInsertString(k, v); }
static void putIS(DcmDataset *d, const DcmTagKey &k, int v){ d->putAndInsertString(k, std::to_string(v).c_str()); }
static void putDS(DcmDataset *d, const DcmTagKey &k, double v){ char b[32]; snprintf(b,sizeof(b),"%.6f",v); d->putAndInsertString(k,b); }
static void putUS(DcmDataset *d, const DcmTagKey &k, Uint16 v){ d->putAndInsertUint16(k, v); }

int main(int argc, char *argv[])
{
    const char *outDir = argc > 1 ? argv[1] : "test_dcm";
    int slices = argc > 2 ? std::atoi(argv[2]) : 16;
    int rows = argc > 3 ? std::atoi(argv[3]) : 256;
    int cols = argc > 4 ? std::atoi(argv[4]) : 256;

#ifdef _WIN32
    _mkdir(outDir);
#else
    mkdir(outDir, 0755);
#endif

    const std::string studyUID = "1.2.826.0.1.3680043.8.498.10";
    const std::string seriesUID = "1.2.826.0.1.3680043.8.498.20";
    double z = 0.0;
    const double sliceGap = 5.0; // mm (厚层, 待超分)

    for (int s = 0; s < slices; ++s, z += sliceGap) {
        DcmFileFormat ff;
        DcmDataset *d = ff.getDataset();

        // Patient / Study / Series
        putLO(d, DCM_PatientName, "Test^Phantom");
        putLO(d, DCM_PatientID, "TEST-0001");
        putSH(d, DCM_PatientSex, "O");
        putSH(d, DCM_PatientAge, "045Y");
        putLO(d, DCM_StudyDescription, "Synthetic CT for DcmtkLoader test");
        putSH(d, DCM_Modality, "CT");
        putUI(d, DCM_StudyInstanceUID, studyUID.c_str());
        putUI(d, DCM_SeriesInstanceUID, seriesUID.c_str());
        putUI(d, DCM_SOPClassUID, UID_CTImageStorage);
        char sop[128]; snprintf(sop,sizeof(sop),"1.2.826.0.1.3680043.8.498.30.%d", s);
        putUI(d, DCM_SOPInstanceUID, sop);
        putSH(d, DCM_AccessionNumber, "ACC0001");
        putIS(d, DCM_InstanceNumber, s + 1);

        // Image
        putUS(d, DCM_Rows, Uint16(rows));
        putUS(d, DCM_Columns, Uint16(cols));
        putUS(d, DCM_BitsAllocated, 16);
        putUS(d, DCM_BitsStored, 16);
        putUS(d, DCM_HighBit, 15);
        putUS(d, DCM_SamplesPerPixel, 1);
        putUS(d, DCM_PixelRepresentation, 0); // unsigned
        putSH(d, DCM_PhotometricInterpretation, "MONOCHROME2");
        putDS(d, DCM_PixelSpacing, 0.7);
        // PixelSpacing 是 DS VM 2, 第二个值:
        d->putAndInsertString(DCM_PixelSpacing, "0.7\\0.7");
        putDS(d, DCM_SliceThickness, sliceGap);
        putDS(d, DCM_RescaleSlope, 1.0);
        putDS(d, DCM_RescaleIntercept, -1024.0);
        putDS(d, DCM_WindowCenter, -600.0);
        d->putAndInsertString(DCM_WindowCenter, "-600");
        d->putAndInsertString(DCM_WindowWidth, "1500");

        char ipp[64]; snprintf(ipp,sizeof(ipp),"0\\0\\%.4f", z);
        d->putAndInsertString(DCM_ImagePositionPatient, ipp);
        d->putAndInsertString(DCM_ImageOrientationPatient, "1\\0\\0\\0\\1\\0");

        // 像素: 合成 CT phantom (空气-1000, 软组织40, 骨600, 随 z 变化的球)
        const size_t n = size_t(rows) * cols;
        std::vector<Uint16> px(n, 0); // 存储值; HU = stored - 1024
        const float t = float(s) / float(slices - 1);
        const float cx = cols * 0.5f, cy = rows * 0.5f;
        const float bodyR = float(cols) * 0.40f;
        const float sphR = float(cols) * 0.10f * (0.5f + 0.5f * std::sin(t * 3.14159f));
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                const float dx = x - cx, dy = y - cy;
                const float r = std::sqrt(dx*dx + dy*dy);
                int hu = -1000;
                if (r < bodyR) {
                    hu = -700; // 肺
                    if (r > bodyR - 14) hu = 40; // 胸壁
                }
                if (std::sqrt(dx*dx + (dy - rows*0.2f)*(dy - rows*0.2f)) < 20) hu = 600; // 脊柱
                if (r < sphR) hu = 60; // 模拟结节
                px[size_t(y*cols + x)] = Uint16(hu + 1024);
            }
        }
        d->putAndInsertUint16Array(DCM_PixelData, px.data(), Uint32(n));

        char path[512]; snprintf(path,sizeof(path),"%s/IM_%04d.dcm", outDir, s);
        if (ff.saveFile(path, EXS_LittleEndianExplicit).good())
            printf("wrote %s\n", path);
        else
            printf("FAIL %s\n", path);
    }
    printf("done: %d slices -> %s\n", slices, outDir);
    return 0;
}
