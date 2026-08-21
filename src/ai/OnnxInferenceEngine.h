#pragma once

#include "ai/IInferenceEngine.h"

#ifdef USE_ONNXRUNTIME
namespace medical {

// ONNX Runtime 推理引擎：跨 Intel/NVIDIA 通用。
// 对单帧执行通用 ONNX 推理，并做启发式输出解析：
//   - 单通道概率图 [1,1,H,W] → 阈值统计阳性区域/面积（分割）
//   - 检测输出 [N,>=5]       → 解析为框 + 评分（检测）
class OnnxInferenceEngine : public IInferenceEngine
{
    struct Impl;
public:
    OnnxInferenceEngine();
    ~OnnxInferenceEngine() override;

    QString name() const override   { return QStringLiteral("ONNX Runtime"); }
    QString device() const override { return m_deviceStr; }

    AiResult infer(const DicomFrame &frame, const QString &modelPath) override;

private:
    Impl *m_impl = nullptr;
    QString m_deviceStr = QStringLiteral("CPU");
};

} // namespace medical
#endif // USE_ONNXRUNTIME
