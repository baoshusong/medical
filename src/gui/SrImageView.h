#pragma once

#include "sr/DicomVolume.h"
#include <QWidget>
#include <QImage>
#include <QFutureWatcher>
#include <QHash>
#include <QList>
#include <QPolygonF>
#include <QString>
#include <QTransform>

namespace medical {

// DICOM 查看器式交互视口: 渲染 DicomVolume 的某个 MPR 切片。
// 支持窗宽窗位、三平面定位、平移/缩放、测量/标注及实时 HU 值。
enum SrTool { ToolNone=0, ToolDistance=1, ToolAngle=2, ToolArrow=3, ToolBox=4, ToolEllipse=5, ToolText=6 };

struct SrAnnotation
{
    int type = 0;
    QVector<QPointF> pts;
    QString text;
};

class SrImageView : public QWidget
{
    Q_OBJECT
public:
    explicit SrImageView(QWidget *parent = nullptr);

    void setSource(const DicomVolume *vol, quint64 revision = 0);
    void clearAnnotations() { m_anns.clear(); m_draft = SrAnnotation(); m_angleStage=0; update(); }
    void setParams(int plane, int slice, float wc, float ww, float zoom, QPointF pan,
                   QPointF crosshair, bool diffMode);
    void setCrosshairVisible(bool visible);
    void setLabel(const QString &s) { m_label = s; update(); }
    void setTool(int t);

    // ImageJ-style display transforms & overlays
    void setFlipHorizontal(bool on) { m_flipH = on; update(); }
    void setFlipVertical(bool on)   { m_flipV = on; update(); }
    void setRotate90(int q)         { m_rot = ((q % 4) + 4) % 4; update(); }
    void setInvert(bool on);
    void setScaleBar(bool on)       { m_scaleBar = on; update(); }
    bool flipHorizontal() const { return m_flipH; }
    bool flipVertical() const   { return m_flipV; }
    int  rotate90() const       { return m_rot; }
    bool invert() const         { return m_invert; }
    bool scaleBar() const       { return m_scaleBar; }

    // read-only accessors (ROI measurement / histogram)
    int plane() const { return m_plane; }
    int slice() const { return m_slice; }
    const DicomVolume *source() const { return m_vol; }
    QList<SrAnnotation> annotations() const { return m_anns.value(key()); }
    float huAt(int ix, int iy) const { return huAtImage(ix, iy); }
    QTransform displayTransform() const;
    QPointF displayToBase(QPointF dp) const;

    QSize sizeHint() const override { return {400, 400}; }

signals:
    void sliceStep(int plane, int delta);
    void cursorRequested(int plane, QPointF imagePos);
    void wlDelta(int dCenter, int dWidth);
    void panDelta(QPointF d);
    void zoomDelta(float factor);
    void focusRequested(int plane);
    void detachRequested(int plane);
    void hoverInfo(QString text);

protected:
    void paintEvent(QPaintEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    const DicomVolume *m_vol = nullptr;
    quint64 m_sourceRevision = 0;
    int   m_plane = 0, m_slice = 0;
    float m_wc = -600, m_ww = 1500;
    float m_zoom = 1.0f;
    QPointF m_pan;
    QPointF m_crosshair;
    bool  m_crosshairVisible = true;
    bool  m_diff = false;
    QString m_label;
    int   m_tool = ToolNone;

    // ImageJ-style display state
    bool  m_flipH = false;
    bool  m_flipV = false;
    int   m_rot = 0;          // quarter-turn count (0..3)
    bool  m_invert = false;    // inverted grayscale display
    bool  m_scaleBar = false;  // show calibrated scale bar

    // 渲染缓存 (避免平移/缩放时逐体素重算)
    mutable QImage m_baseCache;
    mutable QString m_baseKey;
    mutable QImage m_dispCache;
    mutable QString m_dispKey;
    struct RenderResult {
        QImage image;
        quint64 generation = 0;
        QString key;
    };
    QFutureWatcher<RenderResult> *m_renderWatcher = nullptr;
    mutable quint64 m_renderGeneration = 0;
    mutable quint64 m_requestedGeneration = 0;
    mutable QString m_requestedBaseKey;

    enum Drag { None, WL, Pan, Draw } m_drag = None;
    QPointF m_dragPos;
    QPointF m_pressPos;

    QHash<QString, QList<SrAnnotation>> m_anns;
    SrAnnotation m_draft;
    int  m_angleStage = 0;
    QPointF m_hoverImg;
    bool m_hoverValid = false;

    float m_scale = 1.0f;
    QPointF m_origin;

    void sliceDims(int &w, int &h) const;
    int planeSliceCount() const;
    quint8 mapVoxel(float unit) const;
    void requestBaseRender();
    QImage displayImage() const;   // 带缓存的展示图 (切面+窗位 与 翻转/旋转 分层缓存)
    QString key() const;
    QPointF widgetToImage(QPointF wp) const;
    float huAtImage(float ix, float iy) const;
    void drawAnnotations(QPainter &p);
    void drawOverlay(QPainter &p) const;
    QString annotationText(const SrAnnotation &a) const;
};

} // namespace medical
