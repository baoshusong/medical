#include "ai/OpenVinoInferenceEngine.h"

#ifdef USE_OPENVINO
#include "utils/Logger.h"
// #include <openvino/openvino.hpp>

namespace medical {

OpenVinoInferenceEngine::OpenVinoInferenceEngine() = default;
OpenVinoInferenceEngine::~OpenVinoInferenceEngine() = default;

AiResult OpenVinoInferenceEngine::infer(const DicomFrame &frame, const QString &modelPath)
{
    AiResult out;
    out.modelName = QStringLiteral("OpenVINO");
    // TODO: ov::Core{}.read_model(modelPath) → compile_model，infer_request 推理。
    LOG_WARN("openvino", "OpenVINO infer() not implemented (TODO)");
    Q_UNUSED(frame) Q_UNUSED(modelPath)
    return out;
}

} // namespace medical
#endif // USE_OPENVINO
