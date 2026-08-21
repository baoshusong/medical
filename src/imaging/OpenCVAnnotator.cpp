#include "imaging/OpenCVAnnotator.h"

#ifdef USE_OPENCV
#  include <opencv2/imgproc.hpp>
#  include <opencv2/imgcodecs.hpp>
#endif

namespace medical {

QImage OpenCVAnnotator::overlay(const DicomFrame &frame, const QList<Annotation> &ann)
{
#ifdef USE_OPENCV
    // TODO: cv::Mat(frame) → 遍历 ann 用 cv::rectangle/cv::putText/cv::arrowedLine
    // 叠加检测框(置信度)、测量线、箭头，转回 QImage 返回。
    Q_UNUSED(ann)
    return frame.image;
#else
    Q_UNUSED(ann)
    return frame.image; // 无 OpenCV 时返回原图，由 DicomViewer 自绘标注
#endif
}

} // namespace medical
