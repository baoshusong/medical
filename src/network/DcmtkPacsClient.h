#pragma once

#include "network/PacsClient.h"

#ifdef USE_DCMTK
namespace medical {

// DCMTK 真实 PACS 客户端：C-FIND/C-MOVE (SCU)。
// TODO(USE_DCMTK): T_ASC_Network / DIMSE C-FIND-RSP 解析，填充 Study 列表。
class DcmtkPacsClient : public PacsClient
{
    Q_OBJECT
public:
    explicit DcmtkPacsClient(QObject *parent = nullptr);

    QList<Study> find(const QString &patientId, const QString &name) override;
    bool move(const QString &studyUid) override;
    QString connectionInfo() const override { return QStringLiteral("PACS 已连接 (DCMTK)"); }
};

} // namespace medical
#endif // USE_DCMTK
