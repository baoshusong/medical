#pragma once

#include <QString>
#include <QDateTime>

namespace medical {

// 病人
struct Patient
{
    QString patientId;       // HIS/PACS 病人号
    QString name;            // 姓名
    QString sex;             // 性别
    int     age = 0;         // 年龄
    QString birthDate;       // YYYY-MM-DD

    QString display() const
    {
        return QStringLiteral("%1  %2  %3岁  %4")
            .arg(name, sex).arg(age).arg(patientId);
    }
};

} // namespace medical
