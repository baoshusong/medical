#include "imaging/VtkSceneFactory.h"

#ifdef USE_VTK
#  include "imaging/VtkViewer.h"
#  include <QVTKOpenGLNativeWidget.h>
#endif

namespace medical {

QWidget *VtkSceneFactory::createViewer(QWidget *parent)
{
#ifdef USE_VTK
    // TODO: 创建 vtkRenderWindow，挂接 QVTKOpenGLNativeWidget；
    //       MPR 用 vtkImageReslice，VR 用 vtkSmartVolumeMapper + vtkVolume。
    auto *w = new VtkViewer(parent);
    return w;
#else
    Q_UNUSED(parent)
    return nullptr; // 主程序回退到自绘 DicomViewer
#endif
}

} // namespace medical
