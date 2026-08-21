#include "imaging/ItkProcessor.h"

#ifdef USE_ITK
// #include <ITKImageIOFactory.h>
// #include <itkResampleImageFilter.h>
// #include <itkLinearInterpolateImageFilter.h>
#endif

namespace medical {

DicomFrame ItkProcessor::normalize(const DicomFrame &in)
{
#ifdef USE_ITK
    // TODO: 构建 itk::Image<unsigned short,2>，ResampleImageFilter 到等距 0.7mm，
    // IntensityWindowingImageFilter 应用窗位，返回新 DicomFrame。
    return in;
#else
    // 无 ITK 时直接透传。
    return in;
#endif
}

} // namespace medical
