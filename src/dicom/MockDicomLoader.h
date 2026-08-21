#pragma once

#include "dicom/IDicomLoader.h"

namespace medical {

// Mock：未启用 DCMTK 时生成合成胸部 CT 影像 (含模拟肺结节)，
// 使界面在无任何医学库时即可演示完整交互。
class MockDicomLoader : public IDicomLoader
{
public:
    bool load(const QString &path, Study &studyOut) override;
    QVector<DicomFrame> frames() const override { return m_frames; }

private:
    void generateSyntheticCt(Study &study);

    QVector<DicomFrame> m_frames;
};

} // namespace medical
