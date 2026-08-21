#include "ai/IInferenceEngine.h"

#include "ai/MockInferenceEngine.h"
#if defined(USE_ONNXRUNTIME)
#  include "ai/OnnxInferenceEngine.h"
#elif defined(USE_TENSORRT)
#  include "ai/TensorRtInferenceEngine.h"
#elif defined(USE_OPENVINO)
#  include "ai/OpenVinoInferenceEngine.h"
#endif

#include <memory>

namespace medical {

// 优先级：TensorRT (NVIDIA) > ONNX Runtime (通用) > OpenVINO (Intel) > Mock。
std::unique_ptr<IInferenceEngine> IInferenceEngine::create()
{
#if defined(USE_TENSORRT)
    return std::make_unique<TensorRtInferenceEngine>();
#elif defined(USE_ONNXRUNTIME)
    return std::make_unique<OnnxInferenceEngine>();
#elif defined(USE_OPENVINO)
    return std::make_unique<OpenVinoInferenceEngine>();
#else
    return std::make_unique<MockInferenceEngine>();
#endif
}

} // namespace medical
