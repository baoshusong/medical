#pragma once

#include <QWidget>

namespace medical {

struct DicomFrame;

// VTK 场景工厂：USE_VTK 时构建 2D/MPR/VR 三维重建场景，挂接 QVTKOpenGLNativeWidget。
// 关闭时返回 nullptr，主程序回退到自绘 DicomViewer。
class VtkSceneFactory
{
public:
    // 返回可嵌入 QWidget 的视口；USE_VTK 未开启时返回 nullptr。
    static QWidget *createViewer(QWidget *parent);
};

} // namespace medical
