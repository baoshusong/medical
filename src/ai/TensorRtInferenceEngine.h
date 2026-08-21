#pragma once

#include "ai/IInferenceEngine.h"

#ifdef USE_TENSORRT
namespace medical {

// TensorRT 引擎：NVIDIA GPU 高性能加速，肺部 CT/X 光分割/检测首选。
// TODO(USE_TENSORRT): 反序列化 .engine，构建 CUDA stream/execution context，
// 推理 + NMS。模型可由 .onnx 经 trtexec 转换得到。
class TensorRtInferenceEngine : public IInferenceEngine
{
public:
    TensorRtInferenceEngine();
    ~TensorRtInferenceEngine() override;

    QString name() const override   { return QStringLiteral("TensorRT"); }
    QString device() const override { return QStringLiteral("GPU:TensorRT"); }

    AiResult infer(const DicomFrame &frame, const QString &modelPath) override;
};

} // namespace medical
#endif // USE_TENSORRT
