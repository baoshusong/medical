#include "imaging/VtkViewer.h"

#ifdef USE_VTK
// 真实实现：QVTKOpenGLNativeWidget + vtkRenderWindow/vtkRenderer，
// 2D 用 vtkImageActor，MPR 用 vtkImageReslice，VR 用 vtkSmartVolumeMapper。
// 此处仅占位，保证 USE_VTK 时工程可链接。
namespace medical {} // namespace
#endif
