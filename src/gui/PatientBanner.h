#pragma once

#include "core/Study.h"
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QMap>

namespace medical {

// Compact patient / study demographics strip shown beneath the command ribbon,
// in the style of a PACS viewer (patient identity, modality, study, etc.).
class PatientBanner : public QWidget
{
    Q_OBJECT
public:
    explicit PatientBanner(QWidget *parent = nullptr);

public slots:
    void setStudy(const Study &study);
    void clearStudy();

private:
    struct Field { QLabel *label; QLabel *value; };
    Field makeField(const QString &key, const QString &caption);
    void addSeparator();
    void setField(const QString &key, const QString &value);
    void applyEmpty();

    QHBoxLayout *m_row = nullptr;
    QLabel      *m_empty = nullptr;
    QMap<QString, Field> m_fields;
};

} // namespace medical
