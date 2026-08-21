#include "gui/SrImageView.h"
#include "sr/HuNormalize.h"

#include <QContextMenuEvent>
#include <QFont>
#include <QInputDialog>
#include <QLineEdit>
#include <QLineF>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QWheelEvent>
#include <QtConcurrentRun>
#include <algorithm>
#include <cmath>

namespace medical {
namespace {

QImage renderSliceSnapshot(const DicomVolume &volume, int plane, int slice,
                           float wc, float ww, bool invert)
{
    if (volume.isEmpty()) return {};
    const int W = plane == 1 ? volume.rows() : volume.cols();
    const int H = plane == 0 ? volume.rows() : volume.depth();
    if (W <= 0 || H <= 0) return {};

    const float lo = wc - ww * 0.5f;
    const float span = std::max(1.0f, ww);
    const auto map = [lo, span, invert](float unit) {
        const float huValue = hu::unitToHu(unit);
        quint8 value = hu::unitTo8(std::clamp((huValue - lo) / span, 0.0f, 1.0f));
        return invert ? quint8(255) - value : value;
    };

    QImage image(W, H, QImage::Format_Grayscale8);
    if (plane == 0) {
        const int z = qBound(0, slice, volume.depth() - 1);
        for (int y = 0; y < H; ++y) {
            auto *row = image.scanLine(y);
            for (int x = 0; x < W; ++x) row[x] = map(volume.voxel(z, y, x));
        }
    } else if (plane == 1) {
        const int x = qBound(0, slice, volume.cols() - 1);
        for (int z = 0; z < H; ++z) {
            auto *row = image.scanLine(z);
            for (int y = 0; y < W; ++y) row[y] = map(volume.voxel(z, y, x));
        }
    } else {
        const int y = qBound(0, slice, volume.rows() - 1);
        for (int z = 0; z < H; ++z) {
            auto *row = image.scanLine(z);
            for (int x = 0; x < W; ++x) row[x] = map(volume.voxel(z, y, x));
        }
    }
    return image;
}

} // namespace

SrImageView::SrImageView(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(180, 180);
    m_renderWatcher = new QFutureWatcher<RenderResult>(this);
    connect(m_renderWatcher, &QFutureWatcher<RenderResult>::finished, this, [this] {
        if (!m_renderWatcher) return;
        const RenderResult result = m_renderWatcher->result();
        if (result.generation != m_renderGeneration || result.key != m_requestedBaseKey
            || result.image.isNull()) {
            if (m_requestedGeneration != m_renderGeneration)
                requestBaseRender();
            return;
        }
        m_baseCache = result.image;
        m_baseKey = result.key;
        m_dispKey.clear();
        m_dispCache = QImage();
        update();
    });
}

void SrImageView::setSource(const DicomVolume *vol, quint64 revision)
{
    if (m_vol == vol && m_sourceRevision == revision)
        return;
    m_vol = vol;
    m_sourceRevision = revision;
    ++m_renderGeneration;
    m_requestedBaseKey.clear();
    m_baseCache = QImage();
    m_dispCache = QImage();
    update();
}

void SrImageView::setCrosshairVisible(bool visible)
{
    if (m_crosshairVisible == visible) return;
    m_crosshairVisible = visible;
    update();
}

void SrImageView::setInvert(bool on)
{
    if (m_invert == on) return;
    m_invert = on;
    ++m_renderGeneration;
    m_requestedBaseKey.clear();
    update();
}

void SrImageView::setParams(int plane, int slice, float wc, float ww, float zoom, QPointF pan,
                            QPointF crosshair, bool diffMode)
{
    const bool pixelsChanged = m_plane != plane || m_slice != slice
                               || qRound(m_wc) != qRound(wc)
                               || qRound(m_ww) != qRound(ww);
    m_plane = plane;
    m_slice = slice;
    m_wc = wc;
    m_ww = ww;
    m_zoom = zoom;
    m_pan = pan;
    m_crosshair = crosshair;
    m_diff = diffMode;
    m_angleStage = 0;
    if (pixelsChanged) {
        ++m_renderGeneration;
        m_requestedBaseKey.clear();
    }
    update();
}

void SrImageView::setTool(int t)
{
    m_tool = t;
    m_angleStage = 0;
    setCursor(m_tool == ToolNone ? Qt::ArrowCursor : Qt::CrossCursor);
}

void SrImageView::sliceDims(int &w, int &h) const
{
    if (!m_vol || m_vol->isEmpty()) { w = 0; h = 0; return; }
    switch (m_plane) {
    case 0: w = m_vol->cols(); h = m_vol->rows(); break;
    case 1: w = m_vol->rows(); h = m_vol->depth(); break;
    case 2: w = m_vol->cols(); h = m_vol->depth(); break;
    default: w = 0; h = 0; break;
    }
}

int SrImageView::planeSliceCount() const
{
    if (!m_vol || m_vol->isEmpty()) return 0;
    switch (m_plane) {
    case 0: return m_vol->depth();
    case 1: return m_vol->cols();
    case 2: return m_vol->rows();
    default: return 0;
    }
}

quint8 SrImageView::mapVoxel(float unit) const
{
    const float hu = hu::unitToHu(unit);
    const float lo = m_wc - m_ww * 0.5f;
    quint8 v = hu::unitTo8(std::clamp((hu - lo) / std::max(1.0f, m_ww), 0.0f, 1.0f));
    if (m_invert) v = quint8(255) - v;
    return v;
}

QTransform SrImageView::displayTransform() const
{
    int W = 0, H = 0;
    sliceDims(W, H);
    const QPointF c(W / 2.0, H / 2.0);
    QTransform t;
    t.translate(c.x(), c.y());
    if (m_flipH) t.scale(-1, 1);
    if (m_flipV) t.scale(1, -1);
    if (m_rot)   t.rotate(90.0 * m_rot);
    t.translate(-c.x(), -c.y());
    return t;
}

QPointF SrImageView::displayToBase(QPointF dp) const
{
    bool ok = false;
    QTransform inv = displayTransform().inverted(&ok);
    return ok ? inv.map(dp) : dp;
}

QImage SrImageView::displayImage() const
{
    // base 缓存键: 切面 + 窗位窗宽 + 反相 (这些会改变像素)
    const QString bkey = QStringLiteral("%1|%2|%3|%4|%5|%6")
        .arg(m_sourceRevision).arg(m_plane).arg(m_slice)
        .arg(qRound(m_wc)).arg(qRound(m_ww)).arg(m_invert);
    if (bkey != m_baseKey || m_baseCache.isNull()) {
        const_cast<SrImageView *>(this)->requestBaseRender();
        return m_baseCache;
    }
    // 展示缓存键: 在 base 之上叠加翻转/旋转 (平移缩放不影响像素, 仅 drawImage)
    const QString dkey = bkey + QLatin1Char('|') + QString::number(m_flipH)
        + QLatin1Char('|') + QString::number(m_flipV)
        + QLatin1Char('|') + QString::number(m_rot);
    if (dkey != m_dispKey || m_dispCache.isNull()) {
        m_dispCache = m_baseCache.transformed(displayTransform(), Qt::SmoothTransformation);
        m_dispKey = dkey;
    }
    return m_dispCache;
}

void SrImageView::requestBaseRender()
{
    if (!m_vol || m_vol->isEmpty() || !m_renderWatcher || m_renderWatcher->isRunning())
        return;

    const QString key = QStringLiteral("%1|%2|%3|%4|%5|%6")
        .arg(m_sourceRevision).arg(m_plane).arg(m_slice)
        .arg(qRound(m_wc)).arg(qRound(m_ww)).arg(m_invert);
    if (key == m_requestedBaseKey && m_requestedGeneration == m_renderGeneration)
        return;

    m_requestedBaseKey = key;
    const quint64 generation = m_renderGeneration;
    m_requestedGeneration = generation;
    const DicomVolume snapshot = *m_vol;
    const int plane = m_plane;
    const int slice = m_slice;
    const float wc = m_wc;
    const float ww = m_ww;
    const bool invert = m_invert;
    m_renderWatcher->setFuture(QtConcurrent::run(
        [snapshot, plane, slice, wc, ww, invert, generation, key]() {
            RenderResult result;
            result.image = renderSliceSnapshot(snapshot, plane, slice, wc, ww, invert);
            result.generation = generation;
            result.key = key;
            return result;
        }));
}

QString SrImageView::key() const
{
    return QString::number(m_plane) + '|' + QString::number(m_slice);
}

QPointF SrImageView::widgetToImage(QPointF wp) const
{
    const QPointF disp((wp.x() - m_origin.x()) / m_scale, (wp.y() - m_origin.y()) / m_scale);
    return displayToBase(disp);
}

float SrImageView::huAtImage(float ix, float iy) const
{
    if (!m_vol || m_vol->isEmpty()) return std::nanf("");
    const int X = int(std::floor(ix));
    const int Y = int(std::floor(iy));
    float u = -1.0f;
    switch (m_plane) {
    case 0: {
        const int z = qBound(0, m_slice, m_vol->depth() - 1);
        if (X < 0 || X >= m_vol->cols() || Y < 0 || Y >= m_vol->rows()) return std::nanf("");
        u = m_vol->voxel(z, Y, X);
        break;
    }
    case 1: {
        const int x = qBound(0, m_slice, m_vol->cols() - 1);
        if (X < 0 || X >= m_vol->rows() || Y < 0 || Y >= m_vol->depth()) return std::nanf("");
        u = m_vol->voxel(Y, X, x);
        break;
    }
    case 2: {
        const int y = qBound(0, m_slice, m_vol->rows() - 1);
        if (X < 0 || X >= m_vol->cols() || Y < 0 || Y >= m_vol->depth()) return std::nanf("");
        u = m_vol->voxel(Y, y, X);
        break;
    }
    default: return std::nanf("");
    }
    return hu::unitToHu(u);
}

void SrImageView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    int W = 0, H = 0;
    sliceDims(W, H);
    if (!m_vol || m_vol->isEmpty() || W <= 0 || H <= 0) {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, m_label.isEmpty() ? QStringLiteral("无影像") : m_label);
        return;
    }

    const QImage img = displayImage();
    if (img.isNull()) {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("正在生成切面…"));
        return;
    }
    const QSizeF view = size();
    const QSizeF im = img.size();
    const float fit = std::min(view.width() / im.width(), view.height() / im.height()) * 0.95f;
    m_scale = fit * m_zoom;
    const QSizeF target = im * m_scale;
    m_origin = QPointF((view.width() - target.width()) * 0.5f + m_pan.x(),
                       (view.height() - target.height()) * 0.5f + m_pan.y());
    p.setRenderHint(QPainter::SmoothPixmapTransform, m_zoom >= 1.0f);
    p.drawImage(QRectF(m_origin, target), img);

    p.save();
    QTransform disp = QTransform::fromTranslate(m_origin.x(), m_origin.y());
    disp.scale(m_scale, m_scale);
    disp = disp * displayTransform();      // map base-slice coords -> widget, incl. flips/rotation
    p.setTransform(disp);
    drawAnnotations(p);

    const QPointF cross = m_crosshair;
    if (m_crosshairVisible) {
        p.setPen(QPen(QColor(80, 220, 255, 210), 1.0 / m_scale, Qt::DashLine));
        p.drawLine(QPointF(cross.x(), 0), QPointF(cross.x(), H));
        p.drawLine(QPointF(0, cross.y()), QPointF(W, cross.y()));
    }
    p.restore();

    drawOverlay(p);
}

void SrImageView::drawOverlay(QPainter &p) const
{
    if (!m_vol || m_vol->isEmpty()) return;
    QRect r = rect();

    QFont f(QStringLiteral("Cascadia Code"), 9);
    f.setStyleHint(QFont::Monospace);
    p.setFont(f);

    const QString planeName = m_plane == 0 ? QStringLiteral("AXIAL")
                            : m_plane == 1 ? QStringLiteral("SAGITTAL")
                                           : QStringLiteral("CORONAL");
    const int total = planeSliceCount();
    const int idx = m_slice + 1;

    auto block = [&](const QStringList &lines, const QRect &box, int align, const QColor &col) {
        const QString text = lines.join(QStringLiteral("\n"));
        p.setPen(QColor(0, 0, 0, 150));
        p.drawText(box.translated(1, 1), align, text);
        p.setPen(col);
        p.drawText(box, align, text);
    };

    const QColor base(207, 233, 240);   // light cyan overlay
    const int pad = 10;
    const int halfW = r.width() / 2;

    // top-left: series / reconstruction label + modality
    QStringList tl;
    tl << QString(m_label).replace(QLatin1Char('\n'), QStringLiteral(" · "))
       << QStringLiteral("CT · MPR");
    block(tl, QRect(pad, 6, halfW - pad * 2, 36), Qt::AlignLeft | Qt::AlignTop, base);

    // top-right: window level + zoom
    block({ QStringLiteral("WL  %1 / %2").arg(int(m_wc)).arg(int(m_ww)),
            QStringLiteral("Zoom  %1%").arg(int(m_zoom * 100)) },
          QRect(halfW + pad, 6, halfW - pad * 2, 36), Qt::AlignRight | Qt::AlignTop, base);

    // bottom-left: image index
    block({ QStringLiteral("Im  %1 / %2").arg(idx).arg(total) },
          QRect(pad, r.height() - 26, halfW - pad * 2, 20), Qt::AlignLeft | Qt::AlignBottom, base);

    // bottom-right: dimensions (+ diff indicator)
    QStringList br;
    if (m_diff) br << QStringLiteral("DIFF");
    br << QStringLiteral("%1 × %2 px").arg(m_vol->cols()).arg(m_vol->rows());
    block(br, QRect(halfW + pad, r.height() - 26, halfW - pad * 2, 20),
          Qt::AlignRight | Qt::AlignBottom, base);

    // orientation edge markers (anatomical directions)
    const char *top, *bot, *lft, *rgt;
    if (m_plane == 0)      { top = "A"; bot = "P"; lft = "R"; rgt = "L"; }
    else if (m_plane == 1) { top = "A"; bot = "P"; lft = "S"; rgt = "I"; }
    else                   { top = "S"; bot = "I"; lft = "R"; rgt = "L"; }
    const QColor ori(54, 196, 221);
    auto mark = [&](const QString &s, const QRect &box, int align) {
        p.setPen(QColor(0, 0, 0, 150)); p.drawText(box.translated(1, 1), align, s);
        p.setPen(ori); p.drawText(box, align, s);
    };
    QFont of = f; of.setPointSize(11); p.setFont(of);
    mark(QString::fromLatin1(top), QRect(0, 2, r.width(), 20), Qt::AlignTop | Qt::AlignHCenter);
    mark(QString::fromLatin1(bot), QRect(0, r.height() - 22, r.width(), 20), Qt::AlignBottom | Qt::AlignHCenter);
    mark(QString::fromLatin1(lft), QRect(2, 0, 40, r.height()), Qt::AlignLeft | Qt::AlignVCenter);
    mark(QString::fromLatin1(rgt), QRect(r.width() - 42, 0, 40, r.height()), Qt::AlignRight | Qt::AlignVCenter);
    p.setFont(f);

    // cursor HU readout (bottom-center, above orientation marker)
    if (m_hoverValid) {
        const float hu = huAtImage(m_hoverImg.x(), m_hoverImg.y());
        const QString ht = std::isnan(hu) ? QStringLiteral("HU  --")
                                          : QStringLiteral("HU  %1").arg(int(hu));
        QRect hr(0, r.height() - 44, r.width(), 20);
        p.setPen(QColor(0, 0, 0, 150));
        p.drawText(hr.translated(1, 1), Qt::AlignBottom | Qt::AlignHCenter, ht);
        p.setPen(QColor(255, 230, 120));
        p.drawText(hr, Qt::AlignBottom | Qt::AlignHCenter, ht);
    }

    // ImageJ-style calibrated scale bar
    if (m_scaleBar && m_vol && !m_vol->isEmpty()) {
        const float sx = (m_plane == 1) ? m_vol->spacingY()
                       : (m_plane == 2) ? m_vol->spacingX()
                                        : m_vol->spacingX();
        const float targetPx = 120.0f;
        float rawMm = targetPx * sx / m_scale;
        // round to a "nice" length (1,2,5 × 10^k)
        const float mag = std::pow(10.0f, std::floor(std::log10(rawMm)));
        const float mant = rawMm / mag;
        const float niceMm = (mant <= 1.0f) ? 1.0f : (mant <= 2.0f) ? 2.0f
                                             : (mant <= 5.0f) ? 5.0f : 10.0f;
        const float barMm = niceMm * mag;
        const float barPx = barMm / sx * m_scale;
        const int x0 = 14;
        const int y0 = r.height() - 34;
        QPen sb(Qt::white, 2);
        p.setPen(sb);
        p.drawLine(x0, y0, x0 + int(barPx), y0);
        p.drawLine(x0, y0 - 4, x0, y0 + 4);
        p.drawLine(x0 + int(barPx), y0 - 4, x0 + int(barPx), y0 + 4);
        QFont sf = f; sf.setPointSize(10);
        p.setFont(sf);
        p.setPen(Qt::white);
        const QString lbl = QString::number(barMm, 'f', barMm < 10 ? 1 : 0) + QStringLiteral(" mm");
        p.drawText(QRect(x0, y0 - 20, int(barPx) + 4, 16), Qt::AlignLeft | Qt::AlignBottom, lbl);
    }
}

void SrImageView::drawAnnotations(QPainter &p)
{
    auto drawOne = [&](const SrAnnotation &a, bool draft) {
        QPen pen(draft ? QColor(255, 200, 0) : QColor(80, 255, 120), 1.5 / m_scale);
        p.setPen(pen);
        QFont f = p.font();
        f.setPointSizeF(9.0f / m_scale);
        p.setFont(f);
        if (a.type == ToolDistance && a.pts.size() >= 2) {
            p.drawLine(a.pts[0], a.pts[1]);
            for (const auto &q : a.pts) p.drawEllipse(q, 2.0 / m_scale, 2.0 / m_scale);
            p.setPen(QColor(255, 255, 150));
            p.drawText(a.pts[1], annotationText(a));
        } else if (a.type == ToolArrow && a.pts.size() >= 2) {
            p.drawLine(a.pts[0], a.pts[1]);
            const QPointF d = a.pts[1] - a.pts[0];
            const qreal L = std::hypot(d.x(), d.y());
            if (L > 1) {
                const QPointF u = d / L;
                const qreal h = 10.0 / m_scale;
                const QPointF n(-u.y(), u.x());
                p.drawLine(a.pts[1], a.pts[1] - u * h + n * h * 0.5);
                p.drawLine(a.pts[1], a.pts[1] - u * h - n * h * 0.5);
            }
        } else if (a.type == ToolBox && a.pts.size() >= 2) {
            p.drawRect(QRectF(a.pts[0], a.pts[1]).normalized());
            p.setPen(QColor(255, 255, 150));
            p.drawText(a.pts[1], annotationText(a));
        } else if (a.type == ToolAngle && a.pts.size() >= 2) {
            for (int i = 0; i + 1 < a.pts.size(); ++i) p.drawLine(a.pts[i], a.pts[i + 1]);
            for (const auto &q : a.pts) p.drawEllipse(q, 2.0 / m_scale, 2.0 / m_scale);
            if (a.pts.size() == 3) {
                p.setPen(QColor(255, 255, 150));
                p.drawText(a.pts[0], annotationText(a));
            }
        } else if (a.type == ToolEllipse && a.pts.size() >= 2) {
            const QRectF box = QRectF(a.pts[0], a.pts[1]).normalized();
            p.drawEllipse(box);
        } else if (a.type == ToolText && a.pts.size() >= 1 && !a.text.isEmpty()) {
            p.setPen(QColor(255, 255, 150));
            p.drawEllipse(a.pts[0], 2.5 / m_scale, 2.5 / m_scale);
            p.drawText(a.pts[0], a.text);
        }
    };
    for (const auto &a : m_anns.value(key())) drawOne(a, false);
    if (m_drag == Draw && m_draft.type != 0) drawOne(m_draft, true);
}

QString SrImageView::annotationText(const SrAnnotation &a) const
{
    if (!m_vol) return {};
    const float sx = m_plane == 1 ? m_vol->spacingY() : m_vol->spacingX();
    const float sy = m_vol->spacingY();
    if (a.type == ToolDistance && a.pts.size() >= 2) {
        const qreal dx = (a.pts[1].x() - a.pts[0].x()) * sx;
        const qreal dy = (a.pts[1].y() - a.pts[0].y()) * sy;
        return QStringLiteral(" %1 mm").arg(std::hypot(dx, dy), 0, 'f', 1);
    }
    if (a.type == ToolBox && a.pts.size() >= 2) {
        const qreal w = std::abs(a.pts[1].x() - a.pts[0].x()) * sx;
        const qreal h = std::abs(a.pts[1].y() - a.pts[0].y()) * sy;
        return QStringLiteral(" %1×%2 mm").arg(w, 0, 'f', 1).arg(h, 0, 'f', 1);
    }
    if (a.type == ToolAngle && a.pts.size() == 3) {
        const QPointF a1 = a.pts[1] - a.pts[0];
        const QPointF a2 = a.pts[2] - a.pts[0];
        const qreal d = a1.x() * a2.x() + a1.y() * a2.y();
        const qreal m = std::hypot(a1.x(), a1.y()) * std::hypot(a2.x(), a2.y());
        if (m < 1e-6) return {};
        const qreal ang = std::acos(std::clamp(d / m, -1.0, 1.0)) * 180.0 / M_PI;
        return QStringLiteral(" %1°").arg(ang, 0, 'f', 1);
    }
    return {};
}

void SrImageView::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier)
        emit zoomDelta(e->angleDelta().y() > 0 ? 1.1f : 1.0f / 1.1f);
    else if (e->angleDelta().y() != 0)
        emit sliceStep(m_plane, e->angleDelta().y() > 0 ? 1 : -1);
}

void SrImageView::mousePressEvent(QMouseEvent *e)
{
    m_dragPos = e->position();
    m_pressPos = e->position();
    if (m_tool == ToolNone) {
        if (e->button() == Qt::LeftButton) m_drag = WL;
        else if (e->button() == Qt::RightButton || e->button() == Qt::MiddleButton) m_drag = Pan;
        else m_drag = None;
        return;
    }
    if (e->button() != Qt::LeftButton) {
        m_drag = Pan;
        return;
    }
    const QPointF ip = widgetToImage(e->position());
    if (m_tool == ToolText) {
        int width = 0, height = 0;
        sliceDims(width, height);
        if (ip.x() < 0 || ip.y() < 0 || ip.x() >= width || ip.y() >= height) {
            m_drag = None;
            return;
        }
        bool ok = false;
        const QString text = QInputDialog::getText(
            this, QStringLiteral("文字标注"), QStringLiteral("请输入标注文字："),
            QLineEdit::Normal, QString(), &ok).trimmed();
        if (ok && !text.isEmpty()) {
            SrAnnotation annotation;
            annotation.type = ToolText;
            annotation.pts << ip;
            annotation.text = text;
            m_anns[key()].append(annotation);
            update();
        }
        m_drag = None;
        return;
    }
    if (m_tool == ToolAngle) {
        if (m_angleStage == 0) { m_draft.type = ToolAngle; m_draft.pts.clear(); m_draft.pts << ip; m_angleStage = 1; m_drag = Draw; }
        else if (m_angleStage == 1) { m_draft.pts << ip; m_angleStage = 2; }
        else { m_draft.pts << ip; m_anns[key()].append(m_draft); m_draft = SrAnnotation(); m_angleStage = 0; m_drag = None; }
        update();
    } else {
        m_draft.type = m_tool;
        m_draft.pts.clear();
        m_draft.pts << ip << ip;
        m_drag = Draw;
    }
}

void SrImageView::mouseMoveEvent(QMouseEvent *e)
{
    m_hoverImg = widgetToImage(e->position());
    m_hoverValid = true;
    const float hu = huAtImage(m_hoverImg.x(), m_hoverImg.y());
    emit hoverInfo(std::isnan(hu) ? QStringLiteral("HU --")
                                  : QStringLiteral("HU %1  (%2,%3)").arg(int(hu)).arg(int(m_hoverImg.x())).arg(int(m_hoverImg.y())));

    if (m_drag == None) { update(); return; }
    const QPointF cur = e->position();
    if (m_drag == WL) {
        const QPointF d = cur - m_dragPos;
        m_dragPos = cur;
        emit wlDelta(int(-d.y() * 2.0f), int(d.x() * 4.0f));
    } else if (m_drag == Pan) {
        const QPointF d = cur - m_dragPos;
        m_dragPos = cur;
        emit panDelta(d);
    } else if (m_drag == Draw) {
        if (m_tool == ToolAngle) {
            if (m_angleStage >= 1 && !m_draft.pts.isEmpty()) {
                while (m_draft.pts.size() <= m_angleStage) m_draft.pts << QPointF();
                m_draft.pts[m_angleStage] = widgetToImage(e->position());
            }
        } else if (!m_draft.pts.isEmpty()) {
            m_draft.pts[1] = widgetToImage(e->position());
        }
        update();
    }
}

void SrImageView::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_drag == Draw && m_tool != ToolAngle) {
        if (!m_draft.pts.isEmpty() && m_draft.pts.size() >= 2) {
            const QPointF dd = m_draft.pts[1] - m_draft.pts[0];
            if (std::hypot(dd.x(), dd.y()) > 2.0) m_anns[key()].append(m_draft);
        }
        m_draft = SrAnnotation();
    } else if (m_drag == WL && e->button() == Qt::LeftButton
               && QLineF(e->position(), m_pressPos).length() < 4.0) {
        emit cursorRequested(m_plane, widgetToImage(e->position()));
    }
    m_drag = None;
    update();
}

void SrImageView::mouseDoubleClickEvent(QMouseEvent *)
{
    emit focusRequested(m_plane);
}

void SrImageView::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu menu(this);
    QAction *open = menu.addAction(QStringLiteral("在独立窗口打开"));
    if (menu.exec(e->globalPos()) == open) emit detachRequested(m_plane);
}

void SrImageView::leaveEvent(QEvent *)
{
    m_hoverValid = false;
    emit hoverInfo(QString());
    update();
}

} // namespace medical
