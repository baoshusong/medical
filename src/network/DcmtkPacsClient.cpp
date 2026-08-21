#include "network/DcmtkPacsClient.h"

#ifdef USE_DCMTK
#include "utils/Logger.h"
// #include <dcmtk/dcmnet/dimse.h>
// #include <dcmtk/dcmnet/scu.h>

namespace medical {

DcmtkPacsClient::DcmtkPacsClient(QObject *parent) : PacsClient(parent) {}

QList<Study> DcmtkPacsClient::find(const QString &patientId, const QString &name)
{
    // TODO: TSCU + DIMSE C-FIND，按 PatientID/PatientName 检索。
    LOG_WARN("pacs", "DCMTK C-FIND not implemented (TODO)");
    Q_UNUSED(patientId) Q_UNUSED(name)
    return {};
}

bool DcmtkPacsClient::move(const QString &studyUid)
{
    // TODO: C-MOVE-RSP，等待 C-STORE 子操作落盘。
    LOG_WARN("pacs", "DCMTK C-MOVE not implemented (TODO)");
    Q_UNUSED(studyUid)
    return false;
}

} // namespace medical
#endif // USE_DCMTK
