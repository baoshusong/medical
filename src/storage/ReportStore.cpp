#include "storage/ReportStore.h"
#include "utils/Logger.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>

namespace medical {

QString ReportStore::buildContent(const Study &study, const AiResult &ai)
{
    QString s;
    s += QStringLiteral("===== 影像 AI 辅助阅片报告 =====\n");
    s += QStringLiteral("生成时间: %1\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    s += QStringLiteral("病人: %1  %2  %3岁\n")
            .arg(study.patient.name, study.patient.sex).arg(study.patient.age);
    s += QStringLiteral("检查: %1 %2  帧数:%3\n")
            .arg(study.modality, study.description).arg(study.frameCount);
    s += QStringLiteral("AI 引擎: %1  耗时: %2 ms\n").arg(ai.modelName).arg(ai.elapsedMs);
    s += QStringLiteral("----- 检出病灶 -----\n");
    if (ai.detections.isEmpty()) {
        s += QStringLiteral("未检出明显病灶。\n");
    } else {
        int i = 1;
        for (const auto &d : ai.detections) {
            s += QStringLiteral("%1. %2  部位:%3  置信度:%4%  位置:(%5,%6,%7x%8)  %9\n")
                    .arg(i++).arg(d.label, d.location).arg(int(d.score * 100))
                    .arg(d.rect.x(), 0, 'f', 0).arg(d.rect.y(), 0, 'f', 0)
                    .arg(d.rect.width(), 0, 'f', 0).arg(d.rect.height(), 0, 'f', 0)
                    .arg(d.finding);
        }
    }
    s += QStringLiteral("----- 结论 -----\n");
    s += ai.summary + QStringLiteral("\n");
    s += QStringLiteral("================================\n注: 本报告由 AI 辅助生成, 须经执业医师确认。\n");
    return s;
}

bool ReportStore::exportToFile(const QString &content, const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERR("report", QStringLiteral("export failed: %1").arg(filePath));
        return false;
    }
    QTextStream(&f) << content;
    LOG_INFO("report", QStringLiteral("exported: %1").arg(filePath));
    return true;
}

} // namespace medical
