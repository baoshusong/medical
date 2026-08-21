#include "gui/SrViewer.h"
#include "gui/SrImageView.h"
#include "gui/ImageJToolBar.h"
#include "gui/ResultsWindow.h"

#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSizePolicy>
#include <QToolBar>
#include <QActionGroup>
#include <QTimer>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>
#include <algorithm>

namespace medical {

SrViewer::SrViewer(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);
    setObjectName(QStringLiteral("srViewer"));

    // ── VS Code-style compact toolbar (replaces GroupBox) ──
    auto *toolbarFrame = new QFrame(this);
    toolbarFrame->setObjectName(QStringLiteral("viewerToolbar"));
    auto *toolbarLayout = new QVBoxLayout(toolbarFrame);
    toolbarLayout->setContentsMargins(8, 5, 8, 5);
    toolbarLayout->setSpacing(3);

    // Row 1: Window presets + WC/WW
    auto *windowRow = new QHBoxLayout;
    windowRow->setSpacing(4);
    m_preset = new QComboBox;
    m_preset->addItem(QStringLiteral("肺窗"), 0);
    m_preset->addItem(QStringLiteral("纵隔窗"), 1);
    m_preset->addItem(QStringLiteral("骨窗"), 2);
    m_preset->addItem(QStringLiteral("脑窗"), 3);
    m_preset->addItem(QStringLiteral("自定义"), 4);
    m_wcInput = new QSpinBox;
    m_wcInput->setRange(-2000, 4000);
    m_wcInput->setValue(int(m_wc));
    m_wcInput->setPrefix(QStringLiteral("WC "));
    m_wcInput->setFixedWidth(88);
    m_wwInput = new QSpinBox;
    m_wwInput->setRange(1, 4000);
    m_wwInput->setValue(int(m_ww));
    m_wwInput->setPrefix(QStringLiteral("WW "));
    m_wwInput->setFixedWidth(88);
    m_preset->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    windowRow->addWidget(m_preset, 1);
    windowRow->addWidget(m_wcInput);
    windowRow->addWidget(m_wwInput);
    toolbarLayout->addLayout(windowRow);

    // Navigation, interaction tools and quick actions share one compact line
    // so the diagnostic viewport retains the majority of the workspace height.
    auto *operationRow = new QHBoxLayout;
    operationRow->setSpacing(3);
    m_navigationPlane = new QComboBox;
    m_navigationPlane->addItem(QStringLiteral("轴位"), Axial);
    m_navigationPlane->addItem(QStringLiteral("矢状位"), Sagittal);
    m_navigationPlane->addItem(QStringLiteral("冠状位"), Coronal);
    m_navigationPlane->setFixedWidth(90);
    m_firstSlice = new QPushButton(QStringLiteral("⏮"));
    m_firstSlice->setProperty("role", "small");
    m_firstSlice->setFixedWidth(32);
    m_previousSlice = new QPushButton(QStringLiteral("◀"));
    m_previousSlice->setProperty("role", "small");
    m_previousSlice->setFixedWidth(32);
    m_sliceSlider = new QSlider(Qt::Horizontal);
    m_sliceSlider->setObjectName(QStringLiteral("viewerSliceSlider"));
    m_sliceInput = new QSpinBox;
    m_sliceInput->setPrefix(QStringLiteral("层 "));
    m_sliceInput->setFixedWidth(72);
    m_nextSlice = new QPushButton(QStringLiteral("▶"));
    m_nextSlice->setProperty("role", "small");
    m_nextSlice->setFixedWidth(32);
    m_lastSlice = new QPushButton(QStringLiteral("⏭"));
    m_lastSlice->setProperty("role", "small");
    m_lastSlice->setFixedWidth(32);
    operationRow->addWidget(m_navigationPlane);
    operationRow->addWidget(m_firstSlice);
    operationRow->addWidget(m_previousSlice);
    operationRow->addWidget(m_sliceSlider, 1);
    operationRow->addWidget(m_sliceInput);
    operationRow->addWidget(m_nextSlice);
    operationRow->addWidget(m_lastSlice);
    operationRow->addWidget(m_tool);
    operationRow->addWidget(m_reset);
    operationRow->addWidget(m_crosshairToggle);
    operationRow->addWidget(m_clear);
    operationRow->addWidget(m_restore);

    // Interaction tool selector + quick actions
    auto *actionRow = new QHBoxLayout;
    actionRow->setSpacing(3);
    m_tool = new QComboBox;
    m_tool->addItem(QStringLiteral("调窗/平移"), ToolNone);
    m_tool->addItem(QStringLiteral("距离测量"), ToolDistance);
    m_tool->addItem(QStringLiteral("角度测量"), ToolAngle);
    m_tool->addItem(QStringLiteral("箭头标注"), ToolArrow);
    m_tool->addItem(QStringLiteral("方框标注"), ToolBox);
    m_tool->addItem(QStringLiteral("椭圆 ROI"), ToolEllipse);
    m_tool->addItem(QStringLiteral("文字标注"), ToolText);
    m_tool->setFixedWidth(118);
    m_tool->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    m_reset = new QPushButton(QStringLiteral("复位"));
    m_reset->setProperty("role", "small");
    m_crosshairToggle = new QPushButton(QStringLiteral("十字线"));
    m_crosshairToggle->setCheckable(true);
    m_crosshairToggle->setChecked(m_crosshairVisible);
    m_crosshairToggle->setProperty("role", "small");
    m_clear = new QPushButton(QStringLiteral("清标注"));
    m_clear->setProperty("role", "small");
    m_restore = new QPushButton(QStringLiteral("三平面"));
    m_restore->setProperty("role", "small");
    m_restore->setVisible(false);
    operationRow->addLayout(actionRow);
    toolbarLayout->addLayout(operationRow);

    // Quick display actions and concise live readout
    auto *extraRow = new QHBoxLayout;
    extraRow->setSpacing(3);
    auto *zoomInBtn = new QPushButton(QStringLiteral("放大"));
    zoomInBtn->setProperty("role", "small");
    auto *zoomOutBtn = new QPushButton(QStringLiteral("缩小"));
    zoomOutBtn->setProperty("role", "small");
    auto *cineBtn = new QPushButton(QStringLiteral("播放翻层"));
    cineBtn->setProperty("role", "small");
    auto *infoBtn = new QPushButton(QStringLiteral("影像信息"));
    infoBtn->setProperty("role", "small");
    m_info = new QLabel;
    m_info->setProperty("role", "muted");
    m_hu = new QLabel(QStringLiteral("HU --"));
    m_hu->setFixedWidth(150);
    m_hu->setProperty("role", "status");
    extraRow->addWidget(zoomInBtn);
    extraRow->addWidget(zoomOutBtn);
    extraRow->addWidget(cineBtn);
    extraRow->addWidget(infoBtn);
    extraRow->addStretch();
    extraRow->addWidget(m_info, 1);
    extraRow->addWidget(m_hu);
    toolbarLayout->addLayout(extraRow);

    // Connect extra buttons
    connect(zoomInBtn, &QPushButton::clicked, this, &SrViewer::zoomIn);
    connect(zoomOutBtn, &QPushButton::clicked, this, &SrViewer::zoomOut);
    connect(cineBtn, &QPushButton::clicked, this, [this, cineBtn] {
        toggleCine();
        cineBtn->setText(m_cinePlaying ? QStringLiteral("暂停翻层") : QStringLiteral("播放翻层"));
    });
    connect(infoBtn, &QPushButton::clicked, this, &SrViewer::showImageInfo);

    root->addWidget(toolbarFrame);

    // Keep the comparison page image-first: detailed controls remain available
    // above, while the two diagnostic rows share the remaining height equally.
    auto *beforeTitle = new QLabel(QStringLiteral("原始 DICOM / 重建前（轴位 · 矢状位 · 冠状位）"));
    beforeTitle->setProperty("role", "section");
    root->addWidget(beforeTitle);
    auto *beforeRow = new QGridLayout;
    beforeRow->setContentsMargins(0, 2, 0, 2);
    beforeRow->setHorizontalSpacing(4);
    beforeRow->setVerticalSpacing(4);
    for (int plane = Axial; plane <= Coronal; ++plane) {
        auto *view = new SrImageView(this);
        view->setLabel(QStringLiteral("重建前 · %1").arg(planeName(plane)));
        m_beforeViews[plane] = view;
        auto *frame = new QFrame(this);
        frame->setObjectName(QStringLiteral("viewportFrame"));
        auto *frameLayout = new QVBoxLayout(frame);
        frameLayout->setContentsMargins(2, 2, 2, 2);
        auto *vtitle = new QLabel(QStringLiteral("重建前 · %1").arg(planeName(plane)));
        vtitle->setObjectName(QStringLiteral("viewportTitle"));
        frameLayout->addWidget(vtitle);
        frameLayout->addWidget(view, 1);
        m_beforeTitleLabels[plane] = vtitle;
        beforeRow->addWidget(frame, 0, plane);
        connectView(view, false);
    }
    for (int plane = Axial; plane <= Coronal; ++plane)
        beforeRow->setColumnStretch(plane, 1);
    root->addLayout(beforeRow, 1);
    root->setStretch(2, 1);

    m_afterTitle = new QLabel(QStringLiteral("重建结果（轴位 · 矢状位 · 冠状位）"));
    m_afterTitle->setProperty("role", "section");
    root->addWidget(m_afterTitle);
    auto *afterRow = new QGridLayout;
    afterRow->setContentsMargins(0, 2, 0, 2);
    afterRow->setHorizontalSpacing(4);
    afterRow->setVerticalSpacing(4);
    for (int plane = Axial; plane <= Coronal; ++plane) {
        auto *view = new SrImageView(this);
        view->setLabel(QStringLiteral("重建后 · %1\n等待执行重建").arg(planeName(plane)));
        m_afterViews[plane] = view;
        auto *frame = new QFrame(this);
        frame->setObjectName(QStringLiteral("viewportFrame"));
        auto *frameLayout = new QVBoxLayout(frame);
        frameLayout->setContentsMargins(2, 2, 2, 2);
        auto *vtitle = new QLabel(QStringLiteral("重建后 · %1").arg(planeName(plane)));
        vtitle->setObjectName(QStringLiteral("viewportTitle"));
        vtitle->setProperty("role", "after");
        frameLayout->addWidget(vtitle);
        frameLayout->addWidget(view, 1);
        m_afterTitleLabels[plane] = vtitle;
        afterRow->addWidget(frame, 0, plane);
        connectView(view, true);
    }
    for (int plane = Axial; plane <= Coronal; ++plane)
        afterRow->setColumnStretch(plane, 1);
    root->addLayout(afterRow, 1);
    root->setStretch(4, 1);

    auto *hint = new QLabel(QStringLiteral("滚轮翻层 · Ctrl+滚轮缩放 · 左键拖动调窗/单击联动 · 右键拖动平移 · 双击聚焦 · 右键菜单独立查看"));
    hint->setWordWrap(true);
    hint->setMaximumHeight(26);
    hint->setProperty("role", "muted");
    root->addWidget(hint);

    connect(m_preset, &QComboBox::currentIndexChanged, this, &SrViewer::onPresetChanged);
    connect(m_wcInput, qOverload<int>(&QSpinBox::valueChanged), this, &SrViewer::onWindowCenterChanged);
    connect(m_wwInput, qOverload<int>(&QSpinBox::valueChanged), this, &SrViewer::onWindowWidthChanged);
    connect(m_navigationPlane, qOverload<int>(&QComboBox::currentIndexChanged), this, &SrViewer::onNavigationPlaneChanged);
    connect(m_sliceSlider, &QSlider::valueChanged, this, &SrViewer::onNavigationSliceChanged);
    connect(m_sliceInput, qOverload<int>(&QSpinBox::valueChanged), this, &SrViewer::onNavigationSliceChanged);
    connect(m_firstSlice, &QPushButton::clicked, this, &SrViewer::firstSlice);
    connect(m_previousSlice, &QPushButton::clicked, this, &SrViewer::previousSlice);
    connect(m_nextSlice, &QPushButton::clicked, this, &SrViewer::nextSlice);
    connect(m_lastSlice, &QPushButton::clicked, this, &SrViewer::lastSlice);
    connect(m_tool, &QComboBox::currentIndexChanged, this, [this](int) {
        const int tool = m_tool->currentData().toInt();
        for (auto *view : m_beforeViews) view->setTool(tool);
        for (auto *view : m_afterViews) view->setTool(tool);
        for (const auto &detached : m_detachedViews) detached.view->setTool(tool);
    });
    connect(m_reset, &QPushButton::clicked, this, &SrViewer::onResetView);
    connect(m_crosshairToggle, &QPushButton::toggled, this, &SrViewer::onCrosshairToggled);
    connect(m_restore, &QPushButton::clicked, this, &SrViewer::onRestoreGrid);
    connect(m_clear, &QPushButton::clicked, this, [this] {
        for (auto *view : m_beforeViews) view->clearAnnotations();
        for (auto *view : m_afterViews) view->clearAnnotations();
        for (const auto &detached : m_detachedViews) detached.view->clearAnnotations();
    });

    updateVisibility();
    updateSliceControls();
    m_cineTimer = new QTimer(this);
    connect(m_cineTimer, &QTimer::timeout, this, &SrViewer::onCineTick);
}

void SrViewer::connectView(SrImageView *view, bool after)
{
    connect(view, &SrImageView::sliceStep, this, [this, after](int plane, int delta) {
        m_activePlane = plane;
        m_activeAfter = after;
        onSliceStep(plane, delta);
    });
    connect(view, &SrImageView::cursorRequested, this, [this, after](int plane, QPointF imagePos) {
        m_activePlane = plane;
        m_activeAfter = after;
        onCursorRequested(plane, imagePos);
    });
    connect(view, &SrImageView::wlDelta, this, &SrViewer::onWLDelta);
    connect(view, &SrImageView::panDelta, this, &SrViewer::onPanDelta);
    connect(view, &SrImageView::zoomDelta, this, &SrViewer::onZoomDelta);
    connect(view, &SrImageView::focusRequested, this, [this, after](int plane) { focusPlane(plane, after); });
    connect(view, &SrImageView::detachRequested, this, [this, after](int plane) { openDetached(plane, after); });
    connect(view, &SrImageView::hoverInfo, this, [this](const QString &t) {
        if (m_hu) m_hu->setText(t);
        if (t.isEmpty()) {
            emit cursorReadout(QStringLiteral("x=--  y=--  值=--"));
            return;
        }
        // t is "HU <val>  (<x>,<y>)" — reformat to ImageJ-style coordinates+value
        const int p = t.indexOf('(');
        if (p >= 0) {
            const QString val = t.mid(3, p - 3).trimmed();
            QString co = t.mid(p + 1);
            co.chop(1);
            const QStringList xy = co.split(',');
            if (xy.size() == 2) {
                emit cursorReadout(QStringLiteral("x=%1  y=%2  值=%3")
                                   .arg(xy.at(0).trimmed()).arg(xy.at(1).trimmed()).arg(val));
                return;
            }
        }
        emit cursorReadout(t);
    });
}

void SrViewer::setCompareMode(bool on)
{
    m_compare = on;
    updateVisibility();
    pushParams();
}

void SrViewer::setCrosshairVisible(bool visible)
{
    if (m_crosshairVisible == visible) return;
    m_crosshairVisible = visible;
    const QSignalBlocker block(m_crosshairToggle);
    m_crosshairToggle->setChecked(visible);
    applyCrosshairVisibility();
    emit crosshairVisibilityChanged(visible);
}

void SrViewer::onCrosshairToggled(bool visible)
{
    setCrosshairVisible(visible);
}

void SrViewer::applyCrosshairVisibility()
{
    for (auto *view : m_beforeViews) view->setCrosshairVisible(m_crosshairVisible);
    for (auto *view : m_afterViews) view->setCrosshairVisible(m_crosshairVisible);
    for (const auto &detached : m_detachedViews) {
        if (detached.view) detached.view->setCrosshairVisible(m_crosshairVisible);
    }
}

void SrViewer::setVolumes(const DicomVolume &before, const DicomVolume &after)
{
    ++m_volumeRevision;
    m_before = before;
    m_after = after;
    for (auto *view : m_beforeViews) view->clearAnnotations();
    for (auto *view : m_afterViews) view->clearAnnotations();
    for (const auto &detached : m_detachedViews) detached.view->clearAnnotations();
    m_cursor.x = qMax(0, m_before.cols() / 2);
    m_cursor.y = qMax(0, m_before.rows() / 2);
    m_cursor.z = qMax(0, m_before.depth() / 2);
    for (auto *view : m_beforeViews) view->setSource(&m_before, m_volumeRevision);
    for (auto *view : m_afterViews) view->setSource(&m_after, m_volumeRevision);
    updateSliceControls();
    pushParams();
}

SrViewer::ExportState SrViewer::exportState() const
{
    const bool after = m_activeAfter && !m_after.isEmpty();
    const DicomVolume &volume = after ? m_after : m_before;
    return { &volume, m_activePlane, sliceForVolume(m_activePlane, volume, after), m_wc, m_ww };
}

int SrViewer::axisExtent(const DicomVolume &volume, int plane) const
{
    if (volume.isEmpty()) return 0;
    switch (plane) {
    case Axial: return volume.depth();
    case Sagittal: return volume.cols();
    case Coronal: return volume.rows();
    default: return 0;
    }
}

int SrViewer::sourceIndexForPlane(int plane) const
{
    switch (plane) {
    case Axial: return m_cursor.z;
    case Sagittal: return m_cursor.x;
    case Coronal: return m_cursor.y;
    default: return 0;
    }
}

int SrViewer::mapIndexToVolume(int index, int sourceExtent, int targetExtent) const
{
    if (targetExtent <= 1 || sourceExtent <= 1) return 0;
    return qBound(0, qRound(double(index) * (targetExtent - 1) / (sourceExtent - 1)), targetExtent - 1);
}

int SrViewer::sliceForVolume(int plane, const DicomVolume &volume, bool after) const
{
    const int targetExtent = axisExtent(volume, plane);
    if (targetExtent <= 0) return 0;
    const int sourceExtent = axisExtent(m_before, plane);
    const int index = sourceIndexForPlane(plane);
    return after ? mapIndexToVolume(index, sourceExtent, targetExtent)
                 : qBound(0, index, targetExtent - 1);
}

QPointF SrViewer::crosshairForVolume(int plane, const DicomVolume &volume, bool after) const
{
    const auto map = [this, &volume, after](int index, int sourceExtent, int targetExtent) {
        return after ? mapIndexToVolume(index, sourceExtent, targetExtent)
                     : qBound(0, index, qMax(0, targetExtent - 1));
    };
    const int x = map(m_cursor.x, m_before.cols(), volume.cols());
    const int y = map(m_cursor.y, m_before.rows(), volume.rows());
    const int z = map(m_cursor.z, m_before.depth(), volume.depth());
    switch (plane) {
    case Axial: return {x, y};
    case Sagittal: return {y, z};
    case Coronal: return {x, z};
    default: return {};
    }
}

void SrViewer::pushParams()
{
    for (int plane = Axial; plane <= Coronal; ++plane) {
        m_beforeViews[plane]->setParams(plane, sliceForVolume(plane, m_before, false), m_wc, m_ww,
                                        m_zoom, m_pan, crosshairForVolume(plane, m_before, false), false);
        m_afterViews[plane]->setParams(plane, sliceForVolume(plane, m_after, true), m_wc, m_ww,
                                       m_zoom, m_pan, crosshairForVolume(plane, m_after, true), false);
    }
    updateDetachedViews();
    applyCrosshairVisibility();
    if (!m_before.isEmpty()) {
        m_info->setText(QStringLiteral("X:%1/%2  Y:%3/%4  Z:%5/%6  WC=%7 WW=%8")
            .arg(m_cursor.x + 1).arg(m_before.cols())
            .arg(m_cursor.y + 1).arg(m_before.rows())
            .arg(m_cursor.z + 1).arg(m_before.depth())
            .arg(int(m_wc)).arg(int(m_ww)));
    } else {
        m_info->clear();
    }
    for (int plane = Axial; plane <= Coronal; ++plane) {
        if (m_beforeTitleLabels[plane]) m_beforeTitleLabels[plane]->setText(formatViewportTitle(plane, false));
        if (m_afterTitleLabels[plane])  m_afterTitleLabels[plane]->setText(formatViewportTitle(plane, true));
    }
    updateSliceControls();
}

QString SrViewer::formatViewportTitle(int plane, bool after) const
{
    const DicomVolume &vol = after ? m_after : m_before;
    const QString tag = after ? QStringLiteral("重建后") : QStringLiteral("重建前");
    if (vol.isEmpty())
        return QStringLiteral("%1 · %2  （等待载入）").arg(tag).arg(planeName(plane));
    int w = 0, h = 0;
    switch (plane) {
        case Sagittal: w = vol.rows();  h = vol.depth();  break;  // 矢状：行×层
        case Coronal:  w = vol.cols();  h = vol.depth();  break;  // 冠状：列×层
        default:       w = vol.cols();  h = vol.rows();   break;  // 轴位：列×行
    }
    return QStringLiteral("%1 · %2   %3×%4   缩放 ×%5%")
        .arg(tag).arg(planeName(plane)).arg(w).arg(h).arg(int(m_zoom * 100));
}

void SrViewer::updateVisibility()
{
    const bool focused = m_focusedPlane >= Axial && m_focusedPlane <= Coronal;
    for (int plane = Axial; plane <= Coronal; ++plane) {
        const bool showBefore = !focused || (!m_focusedAfter && plane == m_focusedPlane);
        const bool showAfter = m_compare && (!focused || (m_focusedAfter && plane == m_focusedPlane));
        m_beforeViews[plane]->setVisible(showBefore);
        if (auto *frame = qobject_cast<QFrame*>(m_beforeViews[plane]->parentWidget()))
            frame->setVisible(showBefore);
        m_afterViews[plane]->setVisible(showAfter);
        if (auto *frame = qobject_cast<QFrame*>(m_afterViews[plane]->parentWidget()))
            frame->setVisible(showAfter);
    }
    m_afterTitle->setVisible(m_compare && !focused);
    m_restore->setVisible(focused);
}

void SrViewer::updateWindowControls(bool customPreset)
{
    const QSignalBlocker blockWc(m_wcInput);
    const QSignalBlocker blockWw(m_wwInput);
    const QSignalBlocker blockPreset(m_preset);
    m_wcInput->setValue(qRound(m_wc));
    m_wwInput->setValue(qRound(m_ww));
    if (customPreset) m_preset->setCurrentIndex(4);
}

void SrViewer::updateSliceControls()
{
    const int extent = axisExtent(m_before, m_activePlane);
    const bool enabled = extent > 0;
    const int index = enabled ? qBound(0, activeSliceIndex(), extent - 1) : 0;
    const QSignalBlocker blockPlane(m_navigationPlane);
    const QSignalBlocker blockSlider(m_sliceSlider);
    const QSignalBlocker blockInput(m_sliceInput);
    m_navigationPlane->setCurrentIndex(m_activePlane);
    m_sliceSlider->setEnabled(enabled);
    m_sliceInput->setEnabled(enabled);
    m_firstSlice->setEnabled(enabled);
    m_previousSlice->setEnabled(enabled && index > 0);
    m_nextSlice->setEnabled(enabled && index + 1 < extent);
    m_lastSlice->setEnabled(enabled);
    m_sliceSlider->setRange(0, qMax(0, extent - 1));
    m_sliceSlider->setValue(index);
    m_sliceInput->setRange(1, qMax(1, extent));
    m_sliceInput->setValue(index + 1);
    m_sliceInput->setToolTip(enabled ? QStringLiteral("当前 %1 / %2 层").arg(index + 1).arg(extent)
                                      : QStringLiteral("未载入影像"));
}

void SrViewer::setActivePlane(int plane)
{
    if (plane < Axial || plane > Coronal) return;
    m_activePlane = plane;
    updateSliceControls();
}

int SrViewer::activeSliceIndex() const
{
    return sourceIndexForPlane(m_activePlane);
}

void SrViewer::setActiveSliceIndex(int index)
{
    const int extent = axisExtent(m_before, m_activePlane);
    if (extent <= 0) return;
    int *coordinate = m_activePlane == Axial ? &m_cursor.z : m_activePlane == Sagittal ? &m_cursor.x : &m_cursor.y;
    *coordinate = qBound(0, index, extent - 1);
    pushParams();
}

void SrViewer::onNavigationPlaneChanged(int index)
{
    setActivePlane(m_navigationPlane->itemData(index).toInt());
}

void SrViewer::onNavigationSliceChanged(int index)
{
    if (sender() == m_sliceInput) setActiveSliceIndex(index - 1);
    else setActiveSliceIndex(index);
}

void SrViewer::firstSlice() { setActiveSliceIndex(0); }
void SrViewer::previousSlice() { setActiveSliceIndex(activeSliceIndex() - 1); }
void SrViewer::nextSlice() { setActiveSliceIndex(activeSliceIndex() + 1); }
void SrViewer::lastSlice() { setActiveSliceIndex(axisExtent(m_before, m_activePlane) - 1); }

void SrViewer::resetView() { onResetView(); }
void SrViewer::restoreGrid() { onRestoreGrid(); }
void SrViewer::setWindow(float wc, float ww, bool customPreset)
{
    m_wc = qBound(-2000.0f, wc, 4000.0f);
    m_ww = qBound(1.0f, ww, 4000.0f);
    updateWindowControls(customPreset);
    pushParams();
}

void SrViewer::setWindowLevel(float wc, float ww)
{
    setWindow(wc, ww, true);
}

// ── ImageJ-style display transforms & overlays ──────────────────────────
static void applyToViews(const std::array<SrImageView *, 3> &views,
                         const std::function<void(SrImageView *)> &fn)
{
    for (auto *v : views) fn(v);
}

void SrViewer::flipHorizontal()
{
    const bool nv = !m_beforeViews[Axial]->flipHorizontal();
    applyToViews(m_beforeViews, [nv](SrImageView *v) { v->setFlipHorizontal(nv); });
    applyToViews(m_afterViews, [nv](SrImageView *v) { v->setFlipHorizontal(nv); });
}
void SrViewer::flipVertical()
{
    const bool nv = !m_beforeViews[Axial]->flipVertical();
    applyToViews(m_beforeViews, [nv](SrImageView *v) { v->setFlipVertical(nv); });
    applyToViews(m_afterViews, [nv](SrImageView *v) { v->setFlipVertical(nv); });
}
void SrViewer::rotateCW()
{
    const int nr = (m_beforeViews[Axial]->rotate90() + 1) % 4;
    applyToViews(m_beforeViews, [nr](SrImageView *v) { v->setRotate90(nr); });
    applyToViews(m_afterViews, [nr](SrImageView *v) { v->setRotate90(nr); });
}
void SrViewer::rotateCCW()
{
    const int nr = (m_beforeViews[Axial]->rotate90() + 3) % 4;
    applyToViews(m_beforeViews, [nr](SrImageView *v) { v->setRotate90(nr); });
    applyToViews(m_afterViews, [nr](SrImageView *v) { v->setRotate90(nr); });
}
void SrViewer::setInvert(bool on)
{
    m_invert = on;
    applyToViews(m_beforeViews, [on](SrImageView *v) { v->setInvert(on); });
    applyToViews(m_afterViews, [on](SrImageView *v) { v->setInvert(on); });
}
void SrViewer::setScaleBar(bool on)
{
    m_scaleBar = on;
    applyToViews(m_beforeViews, [on](SrImageView *v) { v->setScaleBar(on); });
    applyToViews(m_afterViews, [on](SrImageView *v) { v->setScaleBar(on); });
}

SrViewer::SliceContext SrViewer::activeContext() const
{
    const int plane = (m_focusedPlane >= Axial && m_focusedPlane <= Coronal)
                          ? m_focusedPlane : m_activePlane;
    const bool after = m_focusedAfter && !m_after.isEmpty();
    const DicomVolume *vol = after ? &m_after : &m_before;
    const int slice = sliceForVolume(plane, *vol, after);
    return {plane, slice, vol};
}

SrImageView *SrViewer::activeView() const
{
    const int plane = (m_focusedPlane >= Axial && m_focusedPlane <= Coronal)
                          ? m_focusedPlane : m_activePlane;
    const bool after = m_focusedAfter && !m_after.isEmpty();
    return after ? m_afterViews[plane] : m_beforeViews[plane];
}

void SrViewer::measureROI()
{
    if (m_before.isEmpty() && m_after.isEmpty()) return;
    const struct { float sx, sy; } planeSpacing[3] = {
        {m_before.spacingX(), m_before.spacingY()},   // Axial:  X,Y
        {m_before.spacingY(), m_before.spacingY()},    // Sagittal: Y, Z(~=Y)
        {m_before.spacingX(), m_before.spacingY()}     // Coronal:  X, Z(~=Y)
    };

    for (int plane = Axial; plane <= Coronal; ++plane) {
        for (bool after : {false, true}) {
            SrImageView *view = after ? m_afterViews[plane] : m_beforeViews[plane];
            const DicomVolume &vol = after ? m_after : m_before;
            if (vol.isEmpty()) continue;
            const float sx = planeSpacing[plane].sx;
            const float sy = planeSpacing[plane].sy;
            for (const SrAnnotation &a : view->annotations()) {
                if (a.type != ToolBox && a.type != ToolEllipse) continue;
                if (a.pts.size() < 2) continue;
                const QPointF p0 = a.pts[0], p1 = a.pts[1];
                const int minX = int(std::floor(std::min(p0.x(), p1.x())));
                const int maxX = int(std::ceil(std::max(p0.x(), p1.x())));
                const int minY = int(std::floor(std::min(p0.y(), p1.y())));
                const int maxY = int(std::ceil(std::max(p0.y(), p1.y())));
                const double cx = (p0.x() + p1.x()) / 2.0, cy = (p0.y() + p1.y()) / 2.0;
                const double rx = std::abs(p1.x() - p0.x()) / 2.0;
                const double ry = std::abs(p1.y() - p0.y()) / 2.0;
                const bool ellipse = (a.type == ToolEllipse);

                long count = 0; double sum = 0, sumsq = 0;
                float mn = 1e30f, mx = -1e30f;
                for (int y = minY; y <= maxY; ++y) {
                    for (int x = minX; x <= maxX; ++x) {
                        if (ellipse) {
                            const double nx = rx > 0 ? (x + 0.5 - cx) / rx : 2;
                            const double ny = ry > 0 ? (y + 0.5 - cy) / ry : 2;
                            if (nx * nx + ny * ny > 1.0) continue;
                        }
                        const float hu = view->huAt(x, y);
                        if (std::isnan(hu)) continue;
                        ++count; sum += hu; sumsq += double(hu) * hu;
                        if (hu < mn) mn = hu;
                        if (hu > mx) mx = hu;
                    }
                }
                if (count == 0) continue;
                const double mean = sum / count;
                const double std = std::sqrt(std::max(0.0, sumsq / count - mean * mean));
                const double area = count * sx * sy;   // mm²
                const QString label = QStringLiteral("%1 · %2")
                    .arg(planeName(plane))
                    .arg(after ? QStringLiteral("重建后") : QStringLiteral("重建前"));
                ResultsWindow::instance()->addRow({
                    label,
                    ellipse ? QStringLiteral("椭圆") : QStringLiteral("矩形"),
                    QString::number(area, 'f', 1),
                    QString::number(int(mean)),
                    QString::number(int(std)),
                    QString::number(int(mn)),
                    QString::number(int(mx)),
                    QString::number(count) });
            }
        }
    }
    ResultsWindow::instance()->show();
    ResultsWindow::instance()->raise();
}

void SrViewer::zoomIn() { onZoomDelta(1.25f); }
void SrViewer::zoomOut() { onZoomDelta(0.8f); }

void SrViewer::toggleCine()
{
    if (!m_cineTimer || axisExtent(m_before, m_activePlane) <= 1) return;
    m_cinePlaying = !m_cinePlaying;
    if (m_cinePlaying) {
        m_cineTimer->start(qMax(1, 1000 / m_cineFps));
    } else {
        m_cineTimer->stop();
    }
}

void SrViewer::onCineTick()
{
    const int extent = axisExtent(m_before, m_activePlane);
    if (extent <= 1) { m_cinePlaying = false; m_cineTimer->stop(); return; }
    if (activeSliceIndex() + 1 >= extent) firstSlice();
    else nextSlice();
}

void SrViewer::showImageInfo()
{
    const int extent = axisExtent(m_before, m_activePlane);
    const QString text = QStringLiteral("平面：%1\n层面：%2 / %3\n体数据：%4 × %5 × %6（列 × 行 × 帧）\n像素间距：%7 × %8 mm\n窗位/窗宽：%9 / %10\n缩放：%11%")
        .arg(planeName(m_activePlane))
        .arg(extent > 0 ? activeSliceIndex() + 1 : 0).arg(extent)
        .arg(m_before.cols()).arg(m_before.rows()).arg(m_before.depth())
        .arg(m_before.spacingX(), 0, 'f', 3).arg(m_before.spacingY(), 0, 'f', 3)
        .arg(int(m_wc)).arg(int(m_ww)).arg(int(m_zoom * 100));
    QMessageBox::information(this, QStringLiteral("影像信息"), text);
}

void SrViewer::onPresetChanged(int)
{
    switch (m_preset->currentData().toInt()) {
    case 0: setWindow(-600.0f, 1500.0f, false); break;
    case 1: setWindow(40.0f, 400.0f, false); break;
    case 2: setWindow(400.0f, 1500.0f, false); break;
    case 3: setWindow(40.0f, 80.0f, false); break;
    default: break;
    }
}

void SrViewer::setActiveTool(int tool)
{
    int mapped = ToolNone;
    switch (tool) {
    case ImageJToolBar::ToolNone:    mapped = ToolNone; break;
    case ImageJToolBar::ToolDistance: mapped = ToolDistance; break;
    case ImageJToolBar::ToolAngle:   mapped = ToolAngle; break;
    case ImageJToolBar::ToolArrow:   mapped = ToolArrow; break;
    case ImageJToolBar::ToolBox:     mapped = ToolBox; break;
    case ImageJToolBar::ToolEllipse: mapped = ToolEllipse; break;
    case ImageJToolBar::ToolText:    mapped = ToolText; break;
    default: return;
    }

    for (auto *view : m_beforeViews) view->setTool(mapped);
    for (auto *view : m_afterViews)  view->setTool(mapped);
    for (const auto &detached : m_detachedViews) {
        if (!detached.view) continue;
        detached.view->setTool(mapped);
        if (detached.toolCombo) {
            const QSignalBlocker block(detached.toolCombo);
            for (int i = 0; i < detached.toolCombo->count(); ++i) {
                if (detached.toolCombo->itemData(i).toInt() == mapped) {
                    detached.toolCombo->setCurrentIndex(i);
                    break;
                }
            }
        }
    }
    for (int i = 0; i < m_tool->count(); ++i) {
        if (m_tool->itemData(i).toInt() == mapped) {
            const QSignalBlocker block(m_tool);
            m_tool->setCurrentIndex(i);
            break;
        }
    }
}

void SrViewer::clearAnnotations()
{
    for (auto *view : m_beforeViews) view->clearAnnotations();
    for (auto *view : m_afterViews)  view->clearAnnotations();
    for (const auto &detached : m_detachedViews) detached.view->clearAnnotations();
}

void SrViewer::onWindowCenterChanged(int value)
{
    setWindow(float(value), m_ww, true);
}

void SrViewer::onWindowWidthChanged(int value)
{
    setWindow(m_wc, float(value), true);
}

void SrViewer::onSliceStep(int plane, int delta)
{
    setActivePlane(plane);
    const int extent = axisExtent(m_before, plane);
    if (extent <= 0) return;
    int *coordinate = plane == Axial ? &m_cursor.z : plane == Sagittal ? &m_cursor.x : &m_cursor.y;
    *coordinate = qBound(0, *coordinate + delta, extent - 1);
    pushParams();
}

void SrViewer::setCursorFromImage(int plane, QPointF imagePos)
{
    const int imageX = qRound(imagePos.x());
    const int imageY = qRound(imagePos.y());
    if (m_activeAfter && !m_after.isEmpty()) {
        if (plane == Axial) {
            m_cursor.x = mapIndexToVolume(imageX, m_after.cols(), m_before.cols());
            m_cursor.y = mapIndexToVolume(imageY, m_after.rows(), m_before.rows());
        } else if (plane == Sagittal) {
            m_cursor.y = mapIndexToVolume(imageX, m_after.rows(), m_before.rows());
            m_cursor.z = mapIndexToVolume(imageY, m_after.depth(), m_before.depth());
        } else {
            m_cursor.x = mapIndexToVolume(imageX, m_after.cols(), m_before.cols());
            m_cursor.z = mapIndexToVolume(imageY, m_after.depth(), m_before.depth());
        }
    } else if (plane == Axial) {
        m_cursor.x = qBound(0, imageX, qMax(0, m_before.cols() - 1));
        m_cursor.y = qBound(0, imageY, qMax(0, m_before.rows() - 1));
    } else if (plane == Sagittal) {
        m_cursor.y = qBound(0, imageX, qMax(0, m_before.rows() - 1));
        m_cursor.z = qBound(0, imageY, qMax(0, m_before.depth() - 1));
    } else {
        m_cursor.x = qBound(0, imageX, qMax(0, m_before.cols() - 1));
        m_cursor.z = qBound(0, imageY, qMax(0, m_before.depth() - 1));
    }
}

void SrViewer::onCursorRequested(int plane, QPointF imagePos)
{
    setActivePlane(plane);
    setCursorFromImage(plane, imagePos);
    pushParams();
}

void SrViewer::onWLDelta(int dCenter, int dWidth)
{
    setWindow(m_wc + dCenter, m_ww + dWidth, true);
}

void SrViewer::onPanDelta(QPointF d)
{
    m_pan += d;
    pushParams();
}

void SrViewer::onZoomDelta(float f)
{
    m_zoom = qBound(0.1f, m_zoom * f, 20.0f);
    pushParams();
}

void SrViewer::onResetView()
{
    m_zoom = 1.0f;
    m_pan = QPointF();
    pushParams();
}

void SrViewer::focusPlane(int plane, bool after)
{
    if (m_focusedPlane == plane && m_focusedAfter == after) {
        onRestoreGrid();
        return;
    }
    m_activePlane = plane;
    m_activeAfter = after;
    m_focusedPlane = plane;
    m_focusedAfter = after;
    updateVisibility();
}

void SrViewer::onRestoreGrid()
{
    m_focusedPlane = -1;
    m_focusedAfter = false;
    updateVisibility();
}

void SrViewer::openDetached(int plane, bool after)
{
    const DicomVolume &volume = after ? m_after : m_before;
    if (volume.isEmpty()) return;

    auto *dialog = new QDialog(this, Qt::Window);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(QStringLiteral("%1 · %2").arg(after ? QStringLiteral("重建后") : QStringLiteral("重建前"), planeName(plane)));
    dialog->resize(720, 720);
    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(6, 6, 6, 6);
    auto *toolbar = new QToolBar(dialog);
    toolbar->setObjectName(QStringLiteral("detachedDicomToolbar"));
    toolbar->setMovable(false);
    toolbar->setFloatable(false);

    const QList<QPair<QString, int>> tools = {
        {QStringLiteral("调窗/平移"), ToolNone},
        {QStringLiteral("距离测量"), ToolDistance},
        {QStringLiteral("角度测量"), ToolAngle},
        {QStringLiteral("箭头标注"), ToolArrow},
        {QStringLiteral("方框标注"), ToolBox},
        {QStringLiteral("椭圆 ROI"), ToolEllipse},
        {QStringLiteral("文字标注"), ToolText}
    };

    auto *tool = new QComboBox(dialog);
    tool->setToolTip(QStringLiteral("选择当前独立窗口的操作工具"));
    for (const auto &entry : tools) tool->addItem(entry.first, entry.second);
    toolbar->addWidget(new QLabel(QStringLiteral("工具"), toolbar));
    toolbar->addWidget(tool);

    auto *prev = toolbar->addAction(QStringLiteral("上一层"));
    auto *next = toolbar->addAction(QStringLiteral("下一层"));
    auto *reset = toolbar->addAction(QStringLiteral("复位"));
    auto *zoomIn = toolbar->addAction(QStringLiteral("放大"));
    auto *zoomOut = toolbar->addAction(QStringLiteral("缩小"));
    auto *cine = toolbar->addAction(QStringLiteral("播放翻层"));
    auto *info = toolbar->addAction(QStringLiteral("影像信息"));
    auto *crosshair = toolbar->addAction(QStringLiteral("十字线"));
    crosshair->setCheckable(true);
    crosshair->setChecked(m_crosshairVisible);
    auto *clear = toolbar->addAction(QStringLiteral("清除本窗口标注"));
    layout->addWidget(toolbar);

    auto *view = new SrImageView(dialog);
    const int activeTool = m_tool ? m_tool->currentData().toInt() : ToolNone;
    for (int i = 0; i < tool->count(); ++i) {
        if (tool->itemData(i).toInt() == activeTool) {
            tool->setCurrentIndex(i);
            break;
        }
    }
    view->setTool(activeTool);
    connect(tool, qOverload<int>(&QComboBox::currentIndexChanged), dialog,
            [view, tool](int index) { view->setTool(tool->itemData(index).toInt()); });
    connect(prev, &QAction::triggered, dialog, [this, plane, after](bool) {
        m_activePlane = plane;
        m_activeAfter = after;
        previousSlice();
    });
    connect(next, &QAction::triggered, dialog, [this, plane, after](bool) {
        m_activePlane = plane;
        m_activeAfter = after;
        nextSlice();
    });
    connect(reset, &QAction::triggered, dialog, [this](bool) { resetView(); });
    connect(zoomIn, &QAction::triggered, dialog, [this](bool) { this->zoomIn(); });
    connect(zoomOut, &QAction::triggered, dialog, [this](bool) { this->zoomOut(); });
    connect(cine, &QAction::triggered, dialog, [this, cine, plane, after](bool) {
        m_activePlane = plane;
        m_activeAfter = after;
        toggleCine();
        cine->setText(m_cinePlaying ? QStringLiteral("暂停翻层") : QStringLiteral("播放翻层"));
    });
    connect(info, &QAction::triggered, dialog, [this](bool) { showImageInfo(); });
    connect(crosshair, &QAction::toggled, dialog, [this](bool visible) { setCrosshairVisible(visible); });
    connect(clear, &QAction::triggered, dialog, [view](bool) { view->clearAnnotations(); });
    view->setLabel(QStringLiteral("%1 · %2").arg(after ? QStringLiteral("重建后") : QStringLiteral("重建前"), planeName(plane)));
    layout->addWidget(view);
    connectView(view, after);
    m_detachedViews.append({view, tool, plane, after});
    connect(view, &QObject::destroyed, this, [this, view] {
        m_detachedViews.erase(std::remove_if(m_detachedViews.begin(), m_detachedViews.end(),
            [view](const DetachedView &detached) { return detached.view == view; }), m_detachedViews.end());
    });
    dialog->show();
    pushParams();
}

void SrViewer::updateDetachedViews()
{
    for (const auto &detached : m_detachedViews) {
        if (!detached.view) continue;
        const DicomVolume &volume = detached.after ? m_after : m_before;
        detached.view->setSource(&volume, m_volumeRevision);
        detached.view->setParams(detached.plane, sliceForVolume(detached.plane, volume, detached.after),
                                 m_wc, m_ww, m_zoom, m_pan,
                                 crosshairForVolume(detached.plane, volume, detached.after), false);
    }
}

QString SrViewer::planeName(int plane) const
{
    switch (plane) {
    case Axial: return QStringLiteral("轴位 Axial");
    case Sagittal: return QStringLiteral("矢状位 Sagittal");
    case Coronal: return QStringLiteral("冠状位 Coronal");
    default: return {};
    }
}

} // namespace medical
