#pragma once

#include <QWidget>

namespace medical {

// VTK 视口占位 (仅 USE_VTK 编译)。实现 2D 快速绘制 / MPR / VR。
class VtkViewer : public QWidget
{
    Q_OBJECT
public:
    explicit VtkViewer(QWidget *parent = nullptr) : QWidget(parent) {}
};

} // namespace medical
