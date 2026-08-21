#pragma once

#include <QObject>
#include <QString>

namespace medical {

// 窗宽窗位 (Window Width / Window Level) —— 放射阅片核心交互。
// 不同组织预设：肺(LUNG) 软组织(MEDIA) 骨(BONE) 脑(BRAIN)。
struct WindowLevel
{
    float center = 40.0f;   // WC
    float width  = 400.0f;  // WW

    static WindowLevel lung()   { return { -600.0f, 1500.0f }; }
    static WindowLevel media()  { return {  60.0f,  360.0f }; }
    static WindowLevel bone()   { return { 400.0f, 1500.0f }; }
    static WindowLevel brain()  { return {  40.0f,  80.0f }; }

    QString presetName() const
    {
        if (*this == lung())   return QObject::tr("肺窗");
        if (*this == media())  return QObject::tr("纵隔窗");
        if (*this == bone())   return QObject::tr("骨窗");
        if (*this == brain())  return QObject::tr("脑窗");
        return QObject::tr("自定义");
    }

    bool operator==(const WindowLevel &o) const
    { return center == o.center && width == o.width; }
};

} // namespace medical
