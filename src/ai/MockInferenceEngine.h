#pragma once

#include "ai/IInferenceEngine.h"

namespace medical {

// Mock 推理引擎：无任何 AI 库时返回模拟肺结节检测结果，
// 使界面与流程可完整演示。
class MockInferenceEngine : public IInferenceEngine
{
public:
    QString name() const override   { return QStringLiteral("Mock-LungNet"); }
    QString device() const override { return QStringLiteral("CPU"); }

    AiResult infer(const DicomFrame &frame, const QString &modelPath) override;
};

} // namespace medical
