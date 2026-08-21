#include "network/RestClient.h"
#include "utils/Logger.h"

namespace medical {

RestClient::RestClient(QObject *parent) : QObject(parent) {}

void RestClient::setBaseUrl(const QString &url) { m_baseUrl = url; }

QString RestClient::fetchReport(int reportId)
{
#ifdef USE_HTTPLIB
    // TODO: httplib::Client(m_baseUrl) GET /reports/<id>，返回响应体。
    LOG_WARN("rest", "httplib fetchReport not implemented (TODO)");
#else
    LOG_INFO("rest", QStringLiteral("fetchReport (mock) id=%1").arg(reportId));
#endif
    Q_UNUSED(reportId)
    return {};
}

bool RestClient::uploadFindings(const QString &json)
{
#ifdef USE_HTTPLIB
    // TODO: POST /findings, body=json，校验 200。
    LOG_WARN("rest", "httplib uploadFindings not implemented (TODO)");
#else
    LOG_INFO("rest", QStringLiteral("uploadFindings (mock) %1 bytes").arg(json.size()));
#endif
    return false;
}

QString RestClient::connectionInfo() const
{
#ifdef USE_HTTPLIB
    return m_baseUrl.isEmpty() ? QStringLiteral("REST 未配置") : QStringLiteral("REST → %1").arg(m_baseUrl);
#else
    return QStringLiteral("REST 未启用 (Mock)");
#endif
}

} // namespace medical
