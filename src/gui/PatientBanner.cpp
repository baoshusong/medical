#include "gui/PatientBanner.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QMap>

namespace medical {

PatientBanner::Field PatientBanner::makeField(const QString &key, const QString &caption)
{
    auto *field = new QWidget(this);
    field->setObjectName(QStringLiteral("bannerField"));
    auto *vbox = new QVBoxLayout(field);
    vbox->setContentsMargins(12, 4, 12, 4);
    vbox->setSpacing(2);

    auto *label = new QLabel(caption, field);
    label->setObjectName(QStringLiteral("bannerLabel"));
    auto *value = new QLabel(QStringLiteral("—"), field);
    value->setObjectName(QStringLiteral("bannerValue"));

    vbox->addWidget(label);
    vbox->addWidget(value);

    m_row->addWidget(field);
    Field f{label, value};
    m_fields.insert(key, f);
    return f;
}

void PatientBanner::addSeparator()
{
    auto *sep = new QFrame(this);
    sep->setObjectName(QStringLiteral("bannerSep"));
    m_row->addWidget(sep);
}

PatientBanner::PatientBanner(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("patientBanner"));
    setMinimumHeight(46);
    setMaximumHeight(46);

    m_row = new QHBoxLayout(this);
    m_row->setContentsMargins(8, 0, 8, 0);
    m_row->setSpacing(0);
    m_row->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    makeField(QStringLiteral("patient"),   QStringLiteral("患者"));
    makeField(QStringLiteral("sexAge"),    QStringLiteral("性别 / 年龄"));
    makeField(QStringLiteral("patientId"), QStringLiteral("患者 ID"));
    addSeparator();
    makeField(QStringLiteral("modality"),     QStringLiteral("模态"));
    makeField(QStringLiteral("description"),  QStringLiteral("检查描述"));
    makeField(QStringLiteral("studyDate"),    QStringLiteral("检查日期"));
    makeField(QStringLiteral("accession"),    QStringLiteral("检查号"));
    addSeparator();
    makeField(QStringLiteral("series"),    QStringLiteral("序列"));
    makeField(QStringLiteral("frames"),    QStringLiteral("帧数"));

    // Accent the patient name field.
    m_fields.value(QStringLiteral("patient")).value->setProperty("accent", true);

    m_row->addStretch(1);
    applyEmpty();
}

void PatientBanner::setField(const QString &key, const QString &value)
{
    auto it = m_fields.find(key);
    if (it != m_fields.end())
        it.value().value->setText(value.isEmpty() ? QStringLiteral("—") : value);
}

void PatientBanner::setStudy(const Study &study)
{
    if (m_empty) { m_empty->hide(); m_empty = nullptr; }
    setField(QStringLiteral("patient"),   study.patient.name);
    setField(QStringLiteral("sexAge"),
             QStringLiteral("%1 / %2").arg(
                 study.patient.sex.isEmpty() ? QStringLiteral("—") : study.patient.sex,
                 study.patient.age > 0 ? QStringLiteral("%1 岁").arg(study.patient.age)
                                       : QStringLiteral("—")));
    setField(QStringLiteral("patientId"), study.patient.patientId);
    setField(QStringLiteral("modality"),   study.modality);
    setField(QStringLiteral("description"), study.description);
    setField(QStringLiteral("studyDate"),
             study.dateTime.isValid() ? study.dateTime.toString(QStringLiteral("yyyy-MM-dd HH:mm"))
                                      : QStringLiteral("—"));
    setField(QStringLiteral("accession"),  study.accessionNumber);
    setField(QStringLiteral("series"),    QString::number(study.seriesCount));
    setField(QStringLiteral("frames"),    QString::number(study.frameCount));
}

void PatientBanner::clearStudy()
{
    applyEmpty();
}

void PatientBanner::applyEmpty()
{
    for (auto it = m_fields.begin(); it != m_fields.end(); ++it)
        it.value().value->setText(QStringLiteral("—"));
    if (!m_empty) {
        m_empty = new QLabel(QStringLiteral("未载入检查 — 请通过「文件 ▸ 导入序列」载入 DICOM 数据"), this);
        m_empty->setObjectName(QStringLiteral("bannerEmpty"));
        m_row->addWidget(m_empty);
        m_row->addStretch(1);
    }
    m_empty->show();
}

} // namespace medical
