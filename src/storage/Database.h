#pragma once

#include "core/Patient.h"
#include "core/Study.h"
#include "core/AiResult.h"
#include <QSqlDatabase>
#include <QString>

namespace medical {

// 本地数据存储 (SQLite via QtSql)。缓存病历、检查、标注、AI 报告。
// 如需局域网共享：客户端不直连 PostgreSQL，走 RestClient 中间服务。
class Database
{
public:
    Database();
    ~Database();

    bool open(const QString &path = QStringLiteral("aimedical.db"));

    int  addPatient(const Patient &p);
    int  addStudy(int patientId, const Study &s);
    void addAnnotations(int studyId, const QList<Annotation> &anns);
    int  addReport(int studyId, const QString &content);

    struct ReportRow { int id; QString date; QString modality; QString result; };
    QList<ReportRow> recentReports(int limit = 50);

private:
    void ensureSchema();

    QSqlDatabase m_db;
};

} // namespace medical
