#pragma once

#include <QString>
#include <QTextStream>

namespace medical {

// 简单日志：输出到 stdout / 可扩展为文件。临床软件统一日志入口。
class Logger
{
public:
    enum Level { Debug, Info, Warning, Error };

    static Logger &instance();

    void log(Level level, const QString &tag, const QString &msg);

    void setLevel(Level l) { m_level = l; }

private:
    Logger() = default;
    Level m_level = Info;
};

#define LOG_INFO(tag, msg)  medical::Logger::instance().log(medical::Logger::Info,  tag, msg)
#define LOG_WARN(tag, msg)  medical::Logger::instance().log(medical::Logger::Warning, tag, msg)
#define LOG_ERR(tag, msg)   medical::Logger::instance().log(medical::Logger::Error, tag, msg)

} // namespace medical
