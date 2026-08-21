#include "ResultsWindow.h"
#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

namespace medical {

ResultsWindow *ResultsWindow::instance()
{
    static ResultsWindow *w = new ResultsWindow;
    return w;
}

ResultsWindow::ResultsWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("结果 — 测量 (ImageJ)"));
    setMinimumSize(560, 240);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    m_table = new QTableWidget(0, 8, this);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("视图"), QStringLiteral("类型"),
        QStringLiteral("面积 (mm²)"), QStringLiteral("均值 HU"),
        QStringLiteral("标准差"), QStringLiteral("最小值"),
        QStringLiteral("最大值"), QStringLiteral("像素数") });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    layout->addWidget(m_table);
}

void ResultsWindow::addRow(const QStringList &row)
{
    const int r = m_table->rowCount();
    m_table->insertRow(r);
    m_table->setVerticalHeaderItem(r, new QTableWidgetItem(QString::number(++m_count)));
    for (int c = 0; c < m_table->columnCount() && c < row.size(); ++c)
        m_table->setItem(r, c, new QTableWidgetItem(row.at(c)));
}

void ResultsWindow::clearResults()
{
    m_table->setRowCount(0);
    m_count = 0;
}

} // namespace medical
