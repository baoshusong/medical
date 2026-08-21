#pragma once

#include "core/Study.h"
#include "core/DicomFrame.h"

class QImage;

namespace medical {

// OpenCV 病灶可视化/标注绘制接入点。USE_OPENCV 时用 cv::rectangle/cv::arrowedLine
// 在影像上叠加检测框与测量；关闭时由 DicomViewer 用 QPainter 自绘。
class OpenCVAnnotator
{
public:
    // 在影像上绘制标注，返回叠加后的 QImage。
    static QImage overlay(const DicomFrame &frame, const QList<Annotation> &ann);
};

} // namespace medical
