#pragma once

#include "core/AiResult.h"
#include <QWidget>

class QListWidget;
class QPlainTextEdit;
class QPushButton;

namespace medical {

class Database;

// 右侧历史报告面板 (对应 1.png 右栏)。
class ReportPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ReportPanel(QWidget *parent = nullptr);

    AiResult lastResult() const { return m_lastResult; }
    QString  currentContent() const;

    void setLastAiResult(const AiResult &r, int studyId, Database *db);
    void refresh(Database *db);

signals:
    void newReportRequested();

private:
    QListWidget   *m_list = nullptr;
    QPlainTextEdit *m_view = nullptr;
    QPushButton   *m_new = nullptr;
    AiResult       m_lastResult;
    QString        m_content;
};

} // namespace medical
