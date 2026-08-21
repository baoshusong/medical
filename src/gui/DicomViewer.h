#pragma once

#include "core/DicomFrame.h"
#include "core/Study.h"
#include "utils/WindowLevel.h"
#include <QWidget>
#include <QVector>

namespace medical {

struct AiResult;

// 中间 DICOM 主视口 (对应 1.png 中栏)。
// 自绘实现，支持：窗宽窗位(左键拖拽)、缩放(滚轮)、平移(右键拖拽)、序列翻页、
// 检测框叠加。USE_VTK 开启后由 VtkSceneFactory 提供 QVTKOpenGLNativeWidget 替代。
class DicomViewer : public QWidget
{
    Q_OBJECT
public:
    explicit DicomViewer(QWidget *parent = nullptr);

    void load(const QVector<DicomFrame> &frames, const Study &study);

    DicomFrame currentFrame() const;
    int frameIndex() const { return m_index; }
    int frameCount() const { return m_frames.size(); }

    void prevFrame();
    void nextFrame();
    void applyWindow(const WindowLevel &wl);
    void showDetections(const QList<Annotation> &anns);
    void setTool(int tool) { m_tool = static_cast<int>(tool); update(); }
    void setThreshold(int v) { m_threshold = v; update(); }

signals:
    void frameChanged(int index);

protected:
    void paintEvent(QPaintEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private:
    void renderCurrent();

    QVector<DicomFrame> m_frames;
    Study  m_study;
    int    m_index = 0;
    WindowLevel m_window;
    float  m_zoom = 1.0f;
    QPointF m_pan;
    int    m_tool = 1; // AiToolPanel::NoduleDetect
    int    m_threshold = 50;

    QImage m_canvas; // 当前已按窗位渲染的图

    enum DragMode { None, Windowing, Panning } m_drag = None;
    QPoint m_dragStart;
    WindowLevel m_dragStartWindow;
    QPointF m_dragStartPan;

    QList<Annotation> m_detections;
};

} // namespace medical
