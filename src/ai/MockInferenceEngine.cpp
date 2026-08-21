#include "ai/MockInferenceEngine.h"
#include "utils/Logger.h"

#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QtMath>

namespace medical {

// 模拟肺结节检测：在影像右肺区生成 1~3 个检测框，附置信度与部位。
// 与 MockDicomLoader 在 z≈6 处植入的结节位置基本吻合，演示完整闭环。
AiResult MockInferenceEngine::infer(const DicomFrame &frame, const QString &modelPath)
{
    Q_UNUSED(modelPath)
    AiResult out;
    out.modelName = QStringLiteral("LungNet-CT-v3 (mock)");

    QElapsedTimer t; t.start();

    const int n = QRandomGenerator::global()->bounded(1, 4); // 1..3
    for (int i = 0; i < n; ++i) {
        Annotation a;
        a.type = Annotation::Box;
        const float cx = frame.width  * (0.55f + 0.10f * i);
        const float cy = frame.height * (0.40f + 0.06f * i);
        const float w  = 22 + QRandomGenerator::global()->bounded(18);
        const float h  = 22 + QRandomGenerator::global()->bounded(18);
        a.rect = QRectF(cx - w / 2, cy - h / 2, w, h);
        a.score = 0.72f + 0.20f * QRandomGenerator::global()->generateDouble();
        static const char *locs[] = { "RUL", "RML", "RLL", "LUL", "LLL" };
        a.location = QLatin1String(locs[i % 5]);
        a.label   = QStringLiteral("肺结节");
        a.finding = QStringLiteral("阳性");
        out.detections.append(a);
    }

    out.totalPositive = out.detections.size();
    out.summary = QStringLiteral("检出 %1 枚疑似肺结节，建议结合薄层复查。")
                      .arg(out.totalPositive);
    out.elapsedMs = t.elapsed() + 8 + QRandomGenerator::global()->bounded(20); // 模拟 8~28ms

    LOG_INFO("mock-ai", QStringLiteral("inferred %1 nodule(s) in %2 ms")
                       .arg(out.totalPositive).arg(out.elapsedMs));
    return out;
}

} // namespace medical
