#pragma once

#include "sr/ISuperResEngine.h"

namespace medical {

// Mock 超分引擎: Qt SmoothTransformation (双三次) 面内 4× 上采样 (两维同时放大)。
// 无任何 AI 库即可运行, 作为 SR 全流程的基线与占位。
class MockSuperResEngine : public ISuperResEngine
{
public:
    QString name() const override   { return QStringLiteral("Bicubic-Mock"); }
    QString device() const override { return QStringLiteral("CPU"); }

    QImage upsampleImage(const QImage &img) override;
};

} // namespace medical
