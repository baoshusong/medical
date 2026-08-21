#pragma once

#include "core/Patient.h"
#include <QString>
#include <QList>
#include <QDateTime>
#include <QRectF>

namespace medical {

// 检查/序列
struct Study
{
    QString studyUid;        // Study Instance UID
    QString accessionNumber;
    QString modality;        // CT / MR / XA ...
    QString description;     // 检查描述
    QDateTime dateTime;
    int      seriesCount = 0;
    int      frameCount  = 0;  // 帧数

    Patient patient;

    QString display() const
    {
        return QStringLiteral("%1 %2  %3帧")
            .arg(modality, description).arg(frameCount);
    }
};

// 标注 (病灶框 / 测量 / 箭头)
struct Annotation
{
    enum Type { Box, Measure, Arrow };

    Type     type   = Box;
    QRectF   rect;            // 像素坐标
    QString  label;           // 如 "肺结节"
    float    score  = 0.0f;   // AI 置信度
    QString  location;        // 部位 如 RUL
    QString  finding;         // 阳性/阴性
};

} // namespace medical
