#pragma once

#include <QVector>
#include <QWidget>
#include <QLabel>
#include <functional>

namespace medical {

class SrViewer;

// ImageJ-style Histogram window: HU distribution of the active slice + statistics.
class HistogramDialog : public QWidget
{
    Q_OBJECT
public:
    explicit HistogramDialog(std::function<SrViewer *()> getViewer, QWidget *parent = nullptr);

    void recompute();

protected:
    void showEvent(QShowEvent *e) override;

private:
    void drawHistogram();

    std::function<SrViewer *()> m_getViewer;
    class QLabel *m_canvas = nullptr;
    class QLabel *m_stats = nullptr;

    QVector<int> m_bins;
    float m_huMin = -1024.0f;
    float m_huMax = 3071.0f;
    int m_n = 0;
    float m_mean = 0.0f;
    float m_std = 0.0f;
    float m_min = 0.0f;
    float m_max = 0.0f;
};

} // namespace medical
