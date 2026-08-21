#pragma once

#include <QWidget>
#include <QVector>

class QTableWidget;

namespace medical {

// ImageJ-style "Results" table: accumulates ROI measurements across the session.
class ResultsWindow : public QWidget
{
    Q_OBJECT
public:
    static ResultsWindow *instance();

    // row: [视图, 类型, 面积(mm²), 均值HU, 标准差, 最小值, 最大值, 像素数]
    void addRow(const QStringList &row);
    void clearResults();

private:
    explicit ResultsWindow(QWidget *parent = nullptr);
    QTableWidget *m_table = nullptr;
    int m_count = 0;
};

} // namespace medical
