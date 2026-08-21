#include "HistogramDialog.h"
#include "SrViewer.h"
#include "SrImageView.h"
#include "sr/HuNormalize.h"
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>

namespace medical {

HistogramDialog::HistogramDialog(std::function<SrViewer *()> getViewer, QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::WindowStaysOnTopHint)
    , m_getViewer(std::move(getViewer))
{
    setWindowTitle(QStringLiteral("直方图 (ImageJ)"));
    setMinimumSize(560, 360);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    m_canvas = new QLabel(this);
    m_canvas->setMinimumSize(520, 240);
    m_canvas->setStyleSheet(QStringLiteral("background:#ffffff; border:1px solid #c0c0c0;"));
    layout->addWidget(m_canvas, 1);

    m_stats = new QLabel(this);
    m_stats->setWordWrap(true);
    layout->addWidget(m_stats);

    auto *bar = new QHBoxLayout;
    auto *refresh = new QPushButton(QStringLiteral("刷新"), this);
    bar->addWidget(refresh);
    bar->addStretch(1);
    layout->addLayout(bar);

    connect(refresh, &QPushButton::clicked, this, &HistogramDialog::recompute);

    recompute();
}

void HistogramDialog::recompute()
{
    SrViewer *v = m_getViewer();
    m_bins.assign(512, 0);
    m_n = 0; m_mean = 0; m_std = 0; m_min = 1e30f; m_max = -1e30f;
    if (!v) { drawHistogram(); return; }

    SrViewer::SliceContext ctx = v->activeContext();
    const DicomVolume *vol = ctx.volume;
    if (!vol || vol->isEmpty()) { drawHistogram(); return; }

    const int plane = ctx.plane;
    const int slice = ctx.slice;
    double sum = 0, sumsq = 0;
    const float lo = m_huMin, hi = m_huMax;
    const float span = hi - lo;
    vol->forEachUnitInPlane(plane, slice, [&](float u) {
        const float hu = hu::unitToHu(u);
        ++m_n; sum += hu; sumsq += double(hu) * hu;
        if (hu < m_min) m_min = hu;
        if (hu > m_max) m_max = hu;
        int bin = int((hu - lo) / span * m_bins.size());
        bin = qBound(0, bin, m_bins.size() - 1);
        ++m_bins[bin];
    });
    if (m_n > 0) {
        m_mean = float(sum / m_n);
        m_std = float(std::sqrt(std::max(0.0, sumsq / m_n - double(m_mean) * m_mean)));
    } else {
        m_min = m_max = 0;
    }
    drawHistogram();
}

void HistogramDialog::drawHistogram()
{
    const int W = 520, H = 240;
    QPixmap pm(W, H);
    pm.fill(Qt::white);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int padL = 8, padR = 8, padT = 8, padB = 22;
    const int plotW = W - padL - padR;
    const int plotH = H - padT - padB;

    // baseline
    p.setPen(QColor(200, 200, 200));
    p.drawLine(padL, padT + plotH, padL + plotW, padT + plotH);

    if (m_n > 0) {
        int maxBin = 1;
        for (int b : m_bins) if (b > maxBin) maxBin = b;
        const float logMax = std::log(float(maxBin) + 1.0f);
        p.setPen(QColor(51, 102, 204));
        p.setBrush(QColor(51, 102, 204, 80));
        for (int i = 0; i < m_bins.size(); ++i) {
            const int h = int(std::log(float(m_bins[i]) + 1.0f) / logMax * plotH);
            if (h <= 0) continue;
            const int x = padL + i * plotW / m_bins.size();
            p.drawRect(x, padT + plotH - h, plotW / m_bins.size() + 1, h);
        }
        // HU axis ticks
        p.setPen(QColor(136, 136, 136));
        p.setFont(QFont(QStringLiteral("Consolas"), 9));
        const float step = (m_huMax - m_huMin) / 4.0f;
        for (int k = 0; k <= 4; ++k) {
            const int x = padL + k * plotW / 4;
            const QString lbl = QString::number(int(m_huMin + step * k));
            p.drawText(x - 16, H - 6, lbl);
            p.drawLine(x, padT + plotH, x, padT + plotH + 3);
        }
    } else {
        p.setPen(QColor(136, 136, 136));
        p.drawText(pm.rect(), Qt::AlignCenter, QStringLiteral("无影像数据"));
    }
    m_canvas->setPixmap(pm);

    m_stats->setText(QStringLiteral(
        "像素数 n = %1   均值 = %2 HU   标准差 = %3   最小值 = %4   最大值 = %5   (范围 %6 ~ %7 HU)")
        .arg(m_n).arg(int(m_mean)).arg(int(m_std))
        .arg(int(m_min)).arg(int(m_max)).arg(int(m_huMin)).arg(int(m_huMax)));
}

void HistogramDialog::showEvent(QShowEvent *e)
{
    recompute();
    QWidget::showEvent(e);
}

} // namespace medical
