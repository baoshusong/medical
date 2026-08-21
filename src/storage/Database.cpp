#include "storage/Database.h"
#include "utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>

namespace medical {

Database::Database()
{
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
}

Database::~Database() { if (m_db.isOpen()) m_db.close(); }

bool Database::open(const QString &path)
{
    m_db.setDatabaseName(path);
    if (!m_db.open()) {
        LOG_ERR("db", QStringLiteral("open failed: %1").arg(m_db.lastError().text()));
        return false;
    }
    ensureSchema();
    LOG_INFO("db", QStringLiteral("opened %1").arg(path));
    return true;
}

void Database::ensureSchema()
{
    QSqlQuery q(m_db);
    const char *sql[] = {
        "CREATE TABLE IF NOT EXISTS patients("
        " id INTEGER PRIMARY KEY AUTOINCREMENT, patient_id TEXT, name TEXT, sex TEXT, age INT, birth TEXT)",
        "CREATE TABLE IF NOT EXISTS studies("
        " id INTEGER PRIMARY KEY AUTOINCREMENT, patient_id INT, study_uid TEXT, modality TEXT,"
        " description TEXT, study_date TEXT, frame_count INT)",
        "CREATE TABLE IF NOT EXISTS annotations("
        " id INTEGER PRIMARY KEY AUTOINCREMENT, study_id INT, type TEXT, x REAL, y REAL,"
        " w REAL, h REAL, label TEXT, score REAL, location TEXT, finding TEXT)",
        "CREATE TABLE IF NOT EXISTS reports("
        " id INTEGER PRIMARY KEY AUTOINCREMENT, study_id INT, content TEXT, created TEXT)"
    };
    for (const char *s : sql) {
        if (!q.exec(s))
            LOG_ERR("db", q.lastError().text());
    }
}

int Database::addPatient(const Patient &p)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO patients(patient_id,name,sex,age,birth) VALUES(?,?,?,?,?)");
    q.addBindValue(p.patientId); q.addBindValue(p.name); q.addBindValue(p.sex);
    q.addBindValue(p.age); q.addBindValue(p.birthDate);
    q.exec();
    return q.lastInsertId().toInt();
}

int Database::addStudy(int patientId, const Study &s)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO studies(patient_id,study_uid,modality,description,study_date,frame_count)"
              " VALUES(?,?,?,?,?,?)");
    q.addBindValue(patientId); q.addBindValue(s.studyUid); q.addBindValue(s.modality);
    q.addBindValue(s.description); q.addBindValue(s.dateTime.toString(Qt::ISODate));
    q.addBindValue(s.frameCount);
    q.exec();
    return q.lastInsertId().toInt();
}

void Database::addAnnotations(int studyId, const QList<Annotation> &anns)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO annotations(study_id,type,x,y,w,h,label,score,location,finding)"
              " VALUES(?,?,?,?,?,?,?,?,?,?,?)");
    for (const auto &a : anns) {
        q.addBindValue(studyId);
        q.addBindValue(a.type == Annotation::Box ? "Box" :
                       a.type == Annotation::Measure ? "Measure" : "Arrow");
        q.addBindValue(a.rect.x()); q.addBindValue(a.rect.y());
        q.addBindValue(a.rect.width()); q.addBindValue(a.rect.height());
        q.addBindValue(a.label); q.addBindValue(a.score);
        q.addBindValue(a.location); q.addBindValue(a.finding);
        q.exec();
    }
}

int Database::addReport(int studyId, const QString &content)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO reports(study_id,content,created) VALUES(?,?,?)");
    q.addBindValue(studyId); q.addBindValue(content);
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    q.exec();
    return q.lastInsertId().toInt();
}

QList<Database::ReportRow> Database::recentReports(int limit)
{
    QList<ReportRow> out;
    QSqlQuery q(m_db);
    q.prepare("SELECT r.id, r.created, s.modality, substr(r.content,1,40) "
              "FROM reports r LEFT JOIN studies s ON s.id=r.study_id "
              "ORDER BY r.id DESC LIMIT ?");
    q.addBindValue(limit);
    if (q.exec()) {
        while (q.next()) {
            out.append({ q.value(0).toInt(), q.value(1).toString(),
                         q.value(2).toString(), q.value(3).toString() });
        }
    }
    return out;
}

} // namespace medical
