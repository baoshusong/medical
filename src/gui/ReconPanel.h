#pragma once

#include "sr/BaseReconstructor.h"
#include "sr/DicomVolume.h"
#include <QElapsedTimer>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <memory>

namespace medical {

class SrControlPanel;
class SrViewer;

// 可复用 SR 板块 = 左控制面板 + 右前后对比查看器 + 一个重建器。
// 板块2 传 InPlaneReconstructor, 板块3 传 InterSliceReconstructor。
// 源数据由外部 (MainWindow 共享导入) 经 setSource 下发; 重建结果存于本板块 m_after。
class ReconPanel : public QWidget
{
    Q_OBJECT
public:
    ReconPanel(std::unique_ptr<BaseReconstructor> recon,
               const QString &reconLabel,
               QWidget *parent = nullptr);

    void setSource(const DicomVolume &before);
    void setModelPath(const QString &p) { m_modelPath = p; }
    void reconstruct(int upscale);                 // 用面板 stride + modelPath
    bool isRunning() const { return m_jobActive; }
    void requestCancel();
    const DicomVolume &result() const { return m_after; }
    const SRStats &lastStats() const { return m_recon->lastStats(); }
    SrControlPanel *controlPanel() const { return m_panel; }
    SrViewer *viewer() const { return m_viewer; }
    // 当前"引擎:"标签的引擎名/设备 (供主窗口底部状态栏同步显示)
    QString currentEngineName() const { return m_engineName; }
    QString currentEngineDevice() const { return m_engineDevice; }

signals:
    void importRequested();
    void exportDicomRequested();
    void exportPngRequested();
    void progress(int done, int total);
    void finished(const medical::SRStats &stats);
    // 引擎信息变化 (设备选择或重建结束): 主窗口据此更新底部状态栏
    void engineInfoChanged(const QString &name, const QString &device);

private:
    void onFinished(const medical::SRStats &stats);
    // 根据当前选择的计算设备 (cpu/gpu) 即时刷新底部"引擎:"标签, 显示对应设备型号,
    // 无需等到重建结束. 引擎名优先用上次重建的真实引擎名, 否则用预期引擎名.
    void refreshEngineInfo(const QString &devicePref);

    std::unique_ptr<BaseReconstructor> m_recon;
    SrControlPanel *m_panel  = nullptr;
    SrViewer       *m_viewer = nullptr;
    DicomVolume     m_before;
    DicomVolume     m_after;
    QString         m_modelPath;
    QString         m_engineName;     // 当前引擎名 (用于底部状态栏同步)
    QString         m_engineDevice;   // 当前引擎设备 (CPU/GPU 型号)
    int             m_upscale = 4;
    bool            m_jobActive = false;
    quint64         m_sourceRevision = 0;
    quint64         m_jobSourceRevision = 0;
    QElapsedTimer   m_progressClock;
    QTimer          m_progressTimer;
    int             m_pendingProgressDone = 0;
    int             m_pendingProgressTotal = 0;
};

} // namespace medical
