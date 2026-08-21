#pragma once

#include "ai/IInferenceEngine.h"

#ifdef USE_OPENVINO
namespace medical {

// OpenVINO 引擎：无独显设备时 Intel CPU/核显加速。
// TODO(USE_OPENVINO): ov::Core/CompiledModel，异构推理，输出做 NMS。
class OpenVinoInferenceEngine : public IInferenceEngine
{
public:
    OpenVinoInferenceEngine();
    ~OpenVinoInferenceEngine() override;

    QString name() const override   { return QStringLiteral("OpenVINO"); }
    QString device() const override { return QStringLiteral("Intel"); }

    AiResult infer(const DicomFrame &frame, const QString &modelPath) override;
};

} // namespace medical
#endif // USE_OPENVINO
