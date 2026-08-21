#include "network/PacsClient.h"
#include "utils/Logger.h"

namespace medical {

PacsClient::PacsClient(QObject *parent) : QObject(parent) {}

// Mock：未启用 DCMTK 时返回空，提示已断开。USE_DCMTK 由 DcmtkPacsClient 覆盖。
QList<Study> PacsClient::find(const QString &patientId, const QString &name)
{
    LOG_INFO("pacs", QStringLiteral("C-FIND (mock) patientId=%1 name=%2 -> empty").arg(patientId, name));
    return {};
}

bool PacsClient::move(const QString &studyUid)
{
    LOG_INFO("pacs", QStringLiteral("C-MOVE (mock) study=%1 -> noop").arg(studyUid));
    return false;
}

QString PacsClient::connectionInfo() const
{
    return QStringLiteral("PACS 未连接 (Mock)");
}

} // namespace medical
