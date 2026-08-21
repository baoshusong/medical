#pragma once

#include "core/DicomFrame.h"
#include "core/Study.h"
#include <QVector>
#include <memory>

namespace medical {

// DICOM 加载器接口。实现可为 DCMTK / Mock。
// 真实库受 USE_DCMTK 控制；关闭时由 MockDicomLoader 生成合成影像。
class IDicomLoader
{
public:
    virtual ~IDicomLoader() = default;

    // 载入一个检查目录或文件，返回所有帧。
    virtual bool load(const QString &path, Study &studyOut) = 0;

    virtual QVector<DicomFrame> frames() const = 0;

    // 工厂：根据编译开关返回真实或 Mock 实现。
    static std::unique_ptr<IDicomLoader> create();
};

} // namespace medical
