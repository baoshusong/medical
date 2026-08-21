#pragma once

#include "dicom/IDicomLoader.h"

#ifdef USE_DCMTK
namespace medical {

// DCMTK 真实 DICOM 加载器：解析 .dcm 文件 → DicomFrame。
// TODO(USE_DCMTK): 用 DcmFileFormat/DcmDataset 读取 PixelData/RescaleSlope/
// WindowCenter/Width/PixelSpacing，按窗位生成 8-bit QImage，原始 HU 存入 rawPixels。
class DcmtkLoader : public IDicomLoader
{
public:
    DcmtkLoader();
    ~DcmtkLoader() override;

    bool load(const QString &path, Study &studyOut) override;
    QVector<DicomFrame> frames() const override { return m_frames; }

private:
    QVector<DicomFrame> m_frames;
};

} // namespace medical
#endif // USE_DCMTK
