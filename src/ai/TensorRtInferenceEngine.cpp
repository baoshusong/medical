#include "ai/TensorRtInferenceEngine.h"

#ifdef USE_TENSORRT
#include "utils/Logger.h"
// #include <NvInfer.h>
// #include <cuda_runtime_api.h>

namespace medical {

TensorRtInferenceEngine::TensorRtInferenceEngine() = default;
TensorRtInferenceEngine::~TensorRtInferenceEngine() = default;

AiResult TensorRtInferenceEngine::infer(const DicomFrame &frame, const QString &modelPath)
{
    AiResult out;
    out.modelName = QStringLiteral("TensorRT");
    // TODO: nvinfer1::IRuntime::deserializeCudaEngine，execute context，
    // cudaMemcpy 回传 + NMS。目标 ~12ms/帧。
    LOG_WARN("tensorrt", "TensorRT infer() not implemented (TODO)");
    Q_UNUSED(frame) Q_UNUSED(modelPath)
    return out;
}

} // namespace medical
#endif // USE_TENSORRT
