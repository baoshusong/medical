#pragma once

#include "sr/DicomVolume.h"
#include <QPointF>
#include <QVector>
#include <QWidget>
#include <array>
#include <QtGlobal>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QTimer;
class QVBoxLayout;

namespace medical {

class SrImageView;

// 三平面影像查看器：轴位/矢状位/冠状位联动；重建页额外显示重建后的一组三平面。
class SrViewer : public QWidget
{
    Q_OBJECT
public:
    enum Plane { Axial = 0, Sagittal = 1, Coronal = 2 };

    struct ExportState {
        const DicomVolume *volume = nullptr;
        int plane = Axial;
        int slice = 0;
        float wc = -600.0f;
        float ww = 1500.0f;
    };

    explicit SrViewer(QWidget *parent = nullptr);

    void setVolumes(const DicomVolume &before, const DicomVolume &after);
    void setCompareMode(bool on);
    void setCrosshairVisible(bool visible);
    bool crosshairVisible() const { return m_crosshairVisible; }
    void setActiveTool(int tool);
    void clearAnnotations();
    void resetView();
    void restoreGrid();
    void firstSlice();
    void previousSlice();
    void nextSlice();
    void lastSlice();
    void setWindowLevel(float wc, float ww);
    void zoomIn();
    void zoomOut();
    void toggleCine();
    bool cinePlaying() const { return m_cinePlaying; }
    void showImageInfo();
    ExportState exportState() const;

    // ── ImageJ-style display transforms & overlays ──
    void flipHorizontal();
    void flipVertical();
    void rotateCW();
    void rotateCCW();
    void setInvert(bool on);
    bool invert() const { return m_invert; }
    void setScaleBar(bool on);
    bool scaleBar() const { return m_scaleBar; }

    float windowCenter() const { return m_wc; }
    float windowWidth() const { return m_ww; }

    struct SliceContext { int plane = Axial; int slice = 0; const DicomVolume *volume = nullptr; };
    SliceContext activeContext() const;
    SrImageView *activeView() const;
    void measureROI();

signals:
    void crosshairVisibilityChanged(bool visible);
    void cursorReadout(const QString &readout);   // ImageJ-style "x=.. y=.. 值=.."

private slots:
    void onPresetChanged(int);
    void onWindowCenterChanged(int value);
    void onWindowWidthChanged(int value);
    void onSliceStep(int plane, int delta);
    void onCursorRequested(int plane, QPointF imagePos);
    void onWLDelta(int dCenter, int dWidth);
    void onPanDelta(QPointF d);
    void onZoomDelta(float f);
    void onResetView();
    void onRestoreGrid();
    void onCrosshairToggled(bool visible);
    void onNavigationPlaneChanged(int index);
    void onNavigationSliceChanged(int index);
    void onCineTick();

private:
    struct Cursor { int x = 0; int y = 0; int z = 0; };

    void buildPlaneGroup(QVBoxLayout *layout, const QString &title,
                         std::array<SrImageView *, 3> &views, bool after);
    void connectView(SrImageView *view, bool after);
    void pushParams();
    void updateVisibility();
    void applyCrosshairVisibility();
    void updateWindowControls(bool customPreset);
    void updateSliceControls();
    void setActivePlane(int plane);
    int activeSliceIndex() const;
    void setActiveSliceIndex(int index);
    void setWindow(float wc, float ww, bool customPreset);
    int sourceIndexForPlane(int plane) const;
    int mapIndexToVolume(int index, int sourceExtent, int targetExtent) const;
    int sliceForVolume(int plane, const DicomVolume &volume, bool after) const;
    QPointF crosshairForVolume(int plane, const DicomVolume &volume, bool after) const;
    int axisExtent(const DicomVolume &volume, int plane) const;
    void setCursorFromImage(int plane, QPointF imagePos);
    void focusPlane(int plane, bool after);
    QString formatViewportTitle(int plane, bool after) const;
    void openDetached(int plane, bool after);
    void updateDetachedViews();
    QString planeName(int plane) const;

    struct DetachedView {
        SrImageView *view = nullptr;
        QComboBox *toolCombo = nullptr;
        int plane = Axial;
        bool after = false;
    };

    DicomVolume m_before, m_after;
    Cursor m_cursor;

    QComboBox *m_preset = nullptr;
    QComboBox *m_tool = nullptr;
    QComboBox *m_navigationPlane = nullptr;
    QSlider *m_sliceSlider = nullptr;
    QSpinBox *m_sliceInput = nullptr;
    QSpinBox *m_wcInput = nullptr;
    QSpinBox *m_wwInput = nullptr;
    QPushButton *m_reset = nullptr;
    QPushButton *m_firstSlice = nullptr;
    QPushButton *m_previousSlice = nullptr;
    QPushButton *m_nextSlice = nullptr;
    QPushButton *m_lastSlice = nullptr;
    QPushButton *m_clear = nullptr;
    QPushButton *m_restore = nullptr;
    QPushButton *m_crosshairToggle = nullptr;
    QLabel *m_info = nullptr;
    QLabel *m_hu = nullptr;
    QLabel *m_afterTitle = nullptr;
    std::array<QLabel *, 3> m_beforeTitleLabels{};
    std::array<QLabel *, 3> m_afterTitleLabels{};
    std::array<SrImageView *, 3> m_beforeViews{};
    std::array<SrImageView *, 3> m_afterViews{};
    QVector<DetachedView> m_detachedViews;

    int m_focusedPlane = -1;
    bool m_focusedAfter = false;
    int m_activePlane = Axial;
    bool m_activeAfter = false;
    float m_wc = -600.0f;
    float m_ww = 1500.0f;
    float m_zoom = 1.0f;
    QPointF m_pan;
    bool m_compare = true;
    bool m_crosshairVisible = true;
    QTimer *m_cineTimer = nullptr;
    bool m_cinePlaying = false;
    int m_cineFps = 5;
    bool m_invert = false;
    bool m_scaleBar = false;
    quint64 m_volumeRevision = 0;
};

} // namespace medical
