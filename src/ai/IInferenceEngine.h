#pragma once

#include "core/AiResult.h"
#include "core/DicomFrame.h"
#include <memory>

namespace medical {

// AI 推理引擎接口。解耦：Qt 主程序调用接口，实现可为
// ONNX Runtime / TensorRT / OpenVINO / Mock，由 USE_* 开关选择。
class IInferenceEngine
{
public:
    virtual ~IInferenceEngine() = default;

    virtual QString name() const = 0;          // 引擎名称 (显示在状态栏)
    virtual QString device() const = 0;         // CPU / GPU:TensorRT / Intel

    // 对一帧执行检测/分割，返回结果。
    virtual AiResult infer(const DicomFrame &frame, const QString &modelPath) = 0;

    static std::unique_ptr<IInferenceEngine> create();
};

} // namespace medical
