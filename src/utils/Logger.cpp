#include "utils/Logger.h"

#include <QDebug>
#include <QDateTime>

namespace medical {

Logger &Logger::instance()
{
    static Logger inst;
    return inst;
}

static const char *levelStr(Logger::Level l)
{
    switch (l) {
    case Logger::Debug:   return "DBG";
    case Logger::Info:    return "INF";
    case Logger::Warning: return "WRN";
    case Logger::Error:   return "ERR";
    }
    return "???";
}

void Logger::log(Level level, const QString &tag, const QString &msg)
{
    if (level < m_level)
        return;
    const QString line = QStringLiteral("[%1][%2][%3] %4")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
        .arg(QLatin1String(levelStr(level)))
        .arg(tag, msg);
    qDebug().noquote() << line;
}

} // namespace medical
