#pragma once

#include "core/Study.h"
#include <QObject>
#include <QStringList>

namespace medical {

// 院内 PACS 客户端：DICOM C-FIND / C-MOVE 检索与拉取。
// USE_DCMTK 时用 DcmtkPacsClient 真实实现；否则 Mock 返回空列表。
class PacsClient : public QObject
{
    Q_OBJECT
public:
    explicit PacsClient(QObject *parent = nullptr);

    // C-FIND：按病人号/姓名检索检查列表。
    virtual QList<Study> find(const QString &patientId, const QString &name);
    // C-MOVE：拉取检查到本地。
    virtual bool move(const QString &studyUid);

    virtual QString connectionInfo() const;
};

} // namespace medical
