#pragma once

#include "core/Study.h"
#include <QList>

namespace medical {

// AI 推理结果
struct AiResult
{
    QList<Annotation> detections;   // 检测框
    QString            modelName;    // 如 "LungNet-CT-v3"
    QString            summary;      // 文字结论
    qint64             elapsedMs = 0;
    int                totalPositive = 0;  // 阳性病灶数
};

} // namespace medical
