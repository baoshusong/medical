#pragma once

#include "sr/DicomVolume.h"
#include "core/Study.h"
#include <QWidget>

class QLabel;

namespace medical {

class SrViewer;

// 板块1: DICOM 查看器 (纯查看, 无超分)。
// 单视口 SrViewer (MPR/窗位/测量/标注) + 顶部按钮行 (导入 / 导出当前层面 PNG)。
class ViewerPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ViewerPanel(QWidget *parent = nullptr);

    void setSource(const DicomVolume &vol);
    void setStudy(const Study &study);
    SrViewer *viewer() const { return m_viewer; }

signals:
    void importRequested();
    void exportPngRequested();

private:
    SrViewer *m_viewer = nullptr;
    QLabel *m_studyInfo = nullptr;
};

} // namespace medical
