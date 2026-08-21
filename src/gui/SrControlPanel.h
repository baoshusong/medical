#pragma once

#include <QWidget>

class QPushButton;
class QComboBox;
class QButtonGroup;
class QSpinBox;
class QProgressBar;
class QLabel;

namespace medical {

// 左侧 SR 控制面板 (可复用: 板块2 面内超分 / 板块3 层间超分):
//  ① 导入 DICOM 序列  ② 一键超分重建(文案可配)  ③ 导出结果
class SrControlPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SrControlPanel(QWidget *parent = nullptr);

    void setEngineInfo(const QString &name, const QString &device);
    void setProgress(int done, int total);
    void setReconstructEnabled(bool on);
    void setCancelEnabled(bool on);
    void setStatus(const QString &s);
    void setReconstructLabel(const QString &s);  // ② 按钮文案 (面内/层间)
    int  stride() const;       // 1=全质量, 4/8=快速预览(插值)
    int  scale() const;        // 放大倍数 (来自 m_upscale)
    QString devicePreference() const;  // "cpu" / "gpu" (无 GPU 时引擎自动回退 CPU)
signals:
    void importRequested();
    void reconstructRequested(int upscale);
    void cancelRequested();
    void exportDicomRequested();
    void exportPngRequested();
    void deviceChanged(const QString &pref);  // 计算设备切换: "cpu" / "gpu"

private:
    QPushButton *m_import = nullptr;
    QComboBox   *m_upscale = nullptr;
    QComboBox   *m_stride = nullptr;
    QButtonGroup *m_deviceGroup = nullptr;
    QPushButton  *m_cpuBtn = nullptr;
    QPushButton  *m_gpuBtn = nullptr;
    QPushButton *m_recon  = nullptr;
    QPushButton *m_cancel = nullptr;
    QProgressBar *m_prog  = nullptr;
    QPushButton *m_expDcm = nullptr;
    QPushButton *m_expPng = nullptr;
    QLabel      *m_engine = nullptr;
    QLabel      *m_status = nullptr;
};

} // namespace medical
