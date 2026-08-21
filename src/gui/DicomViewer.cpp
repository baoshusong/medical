#include "gui/DicomViewer.h"
#include "core/AiResult.h"
#include "utils/Logger.h"

#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QLinearGradient>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace medical {

DicomViewer::DicomViewer(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(420, 420);
    setBackgroundRole(QPalette::Dark);
    setAutoFillBackground(true);
}

void DicomViewer::load(const QVector<DicomFrame> &frames, const Study &study)
{
    m_frames = frames;
    m_study  = study;
    m_index  = 0;
    m_zoom   = 1.0f;
    m_pan    = QPointF(0, 0);
    m_window = frames.isEmpty() ? WindowLevel::lung() : frames.first().defaultWindow;
    m_detections.clear();
    renderCurrent();
    update();
    emit frameChanged(m_index);
}

DicomFrame DicomViewer::currentFrame() const
{
    if (m_index >= 0 && m_index < m_frames.size())
        return m_frames[m_index];
    return {};
}

void DicomViewer::prevFrame()
{
    if (m_index > 0) { --m_index; renderCurrent(); update(); emit frameChanged(m_index); }
}

void DicomViewer::nextFrame()
{
    if (m_index < m_frames.size() - 1) { ++m_index; renderCurrent(); update(); emit frameChanged(m_index); }
}

void DicomViewer::applyWindow(const WindowLevel &wl)
{
    m_window = wl;
    renderCurrent();
    update();
}

void DicomViewer::showDetections(const QList<Annotation> &anns)
{
    m_detections = anns;
    update();
}

// 按当前窗位从 rawPixels 重新渲染 8-bit 画布。
void DicomViewer::renderCurrent()
{
    if (m_index < 0 || m_index >= m_frames.size()) { m_canvas = QImage(); return; }
    const DicomFrame &f = m_frames[m_index];

    if (!f.rawPixels.isEmpty() && f.width > 0 && f.height > 0) {
        QImage img(f.width, f.height, QImage::Format_Grayscale8);
        const float wc = m_window.center, ww = std::max(1.0f, m_window.width);
        const float lo = wc - ww * 0.5f;
        for (int y = 0; y < f.height; ++y) {
            const auto *src = f.rawPixels.constData() + y * f.width;
            auto *dst = img.scanLine(y);
            for (int x = 0; x < f.width; ++x) {
                const float hu = float(src[x]) - 1000.0f;
                float v = (hu - lo) / ww;
                v = std::clamp(v, 0.0f, 1.0f);
                dst[x] = static_cast<quint8>(v * 255.0f);
            }
        }
        m_canvas = img;
    } else {
        m_canvas = f.image;
    }
}

void DicomViewer::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    if (m_canvas.isNull()) {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter,
            QStringLiteral("无影像\n文件 → 打开检查 / 运行演示数据"));
        return;
    }

    // fit + zoom + pan
    const QSizeF view = size();
    const QSizeF img  = m_canvas.size();
    const float fit = std::min(view.width() / img.width(), view.height() / img.height()) * 0.96f;
    const float scale = fit * m_zoom;
    const QSizeF target = img * scale;
    QPointF origin((view.width() - target.width()) * 0.5f + m_pan.x(),
                   (view.height() - target.height()) * 0.5f + m_pan.y());

    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(QRectF(origin, target), m_canvas);

    // 检测框叠加
    if (!m_detections.isEmpty()) {
        const float sx = scale, sy = scale;
        QFont font = p.font(); font.setPointSize(8); p.setFont(font);
        for (const auto &d : m_detections) {
            const QRectF r(origin.x() + d.rect.x() * sx,
                           origin.y() + d.rect.y() * sy,
                           d.rect.width() * sx, d.rect.height() * sy);
            p.setPen(QPen(QColor(255, 80, 80), 2));
            p.drawRect(r);
            const QString tag = QStringLiteral("%1 %2%")
                .arg(d.label).arg(int(d.score * 100));
            p.setPen(Qt::red);
            p.fillRect(QRectF(r.left(), r.top() - 14, tag.size() * 7 + 8, 14), QColor(0, 0, 0, 160));
            p.drawText(QPointF(r.left() + 4, r.top() - 3), tag);
        }
    }

    // HUD
    p.setPen(QColor(180, 220, 255));
    QFont f = p.font(); f.setPointSize(9); p.setFont(f);
    p.drawText(8, 16, QStringLiteral("%1  %2  %3/%4")
        .arg(m_study.modality, m_window.presetName())
        .arg(m_index + 1).arg(m_frames.size()));
    p.drawText(8, 32, QStringLiteral("WC:%1 WW:%2  Zoom:%3%")
        .arg(m_window.center, 0, 'f', 0).arg(m_window.width, 0, 'f', 0)
        .arg(int(m_zoom * 100)));
    p.drawText(8, 48, m_study.patient.display());
}

void DicomViewer::wheelEvent(QWheelEvent *e)
{
    const int delta = e->angleDelta().y();
    if (delta != 0) {
        m_zoom *= (delta > 0 ? 1.1f : 1.0f / 1.1f);
        m_zoom = std::clamp(m_zoom, 0.2f, 8.0f);
        update();
    }
}

void DicomViewer::mousePressEvent(QMouseEvent *e)
{
    m_dragStart = e->position().toPoint();
    if (e->button() == Qt::LeftButton) {
        m_drag = Windowing;
        m_dragStartWindow = m_window;
    } else if (e->button() == Qt::RightButton || e->button() == Qt::MiddleButton) {
        m_drag = Panning;
        m_dragStartPan = m_pan;
    }
}

void DicomViewer::mouseMoveEvent(QMouseEvent *e)
{
    if (m_drag == None) return;
    const QPointF d = e->position().toPoint() - m_dragStart;
    if (m_drag == Windowing) {
        m_window.width  = std::max(1.0f, m_dragStartWindow.width  + float(d.x()) * 4.0f);
        m_window.center = m_dragStartWindow.center - float(d.y()) * 4.0f;
        renderCurrent();
        update();
    } else if (m_drag == Panning) {
        m_pan = m_dragStartPan + d;
        update();
    }
}

void DicomViewer::mouseReleaseEvent(QMouseEvent *) { m_drag = None; }

void DicomViewer::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
}

} // namespace medical
