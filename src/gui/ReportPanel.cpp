#include "gui/ReportPanel.h"
#include "storage/Database.h"
#include "storage/ReportStore.h"
#include "core/Study.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QDateTime>

namespace medical {

ReportPanel::ReportPanel(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("历史报告"));
    title->setStyleSheet(QStringLiteral("font-weight:600;"));
    root->addWidget(title);

    m_list = new QListWidget;
    root->addWidget(m_list, 1);

    m_view = new QPlainTextEdit;
    m_view->setReadOnly(true);
    m_view->setPlaceholderText(QStringLiteral("报告内容 (运行 AI 后生成)"));
    root->addWidget(m_view, 2);

    m_new = new QPushButton(QStringLiteral("＋ 新建报告"));
    connect(m_new, &QPushButton::clicked, this, &ReportPanel::newReportRequested);
    root->addWidget(m_new);
}

void ReportPanel::setLastAiResult(const AiResult &r, int studyId, Database *db)
{
    m_lastResult = r;
    Study s;
    s.modality = QStringLiteral("CT");
    s.patient.name = QStringLiteral("当前检查");
    s.frameCount = 0;
    m_content = ReportStore::buildContent(s, r);
    m_view->setPlainText(m_content);
    if (db && studyId >= 0) {
        const int rid = db->addReport(studyId, m_content);
        db->addAnnotations(studyId, r.detections);
        refresh(db);
        Q_UNUSED(rid)
    }
}

void ReportPanel::refresh(Database *db)
{
    if (!db) return;
    m_list->clear();
    for (const auto &row : db->recentReports()) {
        const QString text = QStringLiteral("%1  %2  %3")
            .arg(row.date.left(10), row.modality, row.result);
        m_list->addItem(text);
    }
}

QString ReportPanel::currentContent() const
{
    return m_view->toPlainText();
}

} // namespace medical
