#include "gui/ReconPanel.h"
#include "gui/SrControlPanel.h"
#include "gui/SrViewer.h"
#include "utils/Logger.h"

#ifdef USE_ONNXRUNTIME
#include "sr/OrtProviders.h"
#endif

#include <QTimer>
#include <QVBoxLayout>
#include <QElapsedTimer>

namespace medical {

ReconPanel::ReconPanel(std::unique_ptr<BaseReconstructor> recon,
                       const QString &reconLabel, QWidget *parent)
    : QWidget(parent), m_recon(std::move(recon))
{
    // SrControlPanel is created as a child of ReconPanel but NOT added to layout.
    // It will be reparented to the sidebar by MainWindow via controlPanel().
    m_panel = new SrControlPanel(this);
    m_panel->setObjectName(QStringLiteral("reconControlPanel"));

    // SrViewer fills the entire panel area
    m_viewer = new SrViewer(this);
    m_viewer->setObjectName(QStringLiteral("reconViewer"));
    m_viewer->setCompareMode(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(0);
    root->addWidget(m_viewer, 1);

    m_panel->setReconstructLabel(reconLabel);
    // 构造时即根据当前选择的计算设备刷新引擎标签 (默认 GPU, 立即显示 GPU 型号).

    // Control panel → this panel
    connect(m_panel, &SrControlPanel::importRequested,    this, &ReconPanel::importRequested);
    connect(m_panel, &SrControlPanel::exportDicomRequested, this, &ReconPanel::exportDicomRequested);
    connect(m_panel, &SrControlPanel::exportPngRequested,  this, &ReconPanel::exportPngRequested);
    // 计算设备 (CPU/GPU) 切换后, 立即刷新"引擎:"标签显示对应设备型号
    connect(m_panel, &SrControlPanel::deviceChanged, this, &ReconPanel::refreshEngineInfo);
    connect(m_panel, &SrControlPanel::reconstructRequested, this, [this](int up){
        if (m_before.isEmpty()) {
            m_panel->setStatus(QStringLiteral("请先 ① 导入 DICOM 序列。"));
            return;
        }
        if (m_jobActive || m_recon->isRunning()) return;
        reconstruct(up);
    });
    connect(m_panel, &SrControlPanel::cancelRequested, this, &ReconPanel::requestCancel);

    // Reconstructor → this panel (forward + status). Coalesce worker progress
    // to at most 20 Hz so hundreds of queued UI updates cannot delay completion.
    m_progressTimer.setSingleShot(true);
    const auto publishProgress = [this]() {
        m_panel->setProgress(m_pendingProgressDone, m_pendingProgressTotal);
        emit progress(m_pendingProgressDone, m_pendingProgressTotal);
        m_progressClock.restart();
    };
    connect(&m_progressTimer, &QTimer::timeout, this, publishProgress);
    connect(m_recon.get(), &BaseReconstructor::progress, this, [this, publishProgress](int done, int total) {
        m_pendingProgressDone = done;
        m_pendingProgressTotal = total;
        const bool finalUpdate = total > 0 && done >= total;
        if (finalUpdate || !m_progressClock.isValid() || m_progressClock.elapsed() >= 50) {
            m_progressTimer.stop();
            publishProgress();
        } else if (!m_progressTimer.isActive()) {
            m_progressTimer.start(50 - int(m_progressClock.elapsed()));
        }
    });
    connect(m_recon.get(), &BaseReconstructor::finished, this, &ReconPanel::onFinished);

    // 构造末尾即按当前选择的计算设备刷新引擎标签 (默认 GPU, 立即显示设备型号)
    refreshEngineInfo(m_panel->devicePreference());
}

void ReconPanel::refreshEngineInfo(const QString &devicePref)
{
    // 设备串: 即时查询 CPU/GPU 型号, 不真正创建推理会话
    QString dev;
#ifdef USE_ONNXRUNTIME
    dev = (devicePref == QStringLiteral("cpu"))
              ? describeCpuDevice()
              : describeGpuDevice();
#else
    dev = (devicePref == QStringLiteral("cpu")) ? QStringLiteral("CPU") : QStringLiteral("GPU");
#endif

    // 仅展示设备型号 (CPU/GPU), 不再显示模型/引擎名称
    m_engineDevice = dev;
    m_panel->setEngineInfo(QString(), dev);
    emit engineInfoChanged(QString(), dev);   // 同步主窗口底部状态栏
}

void ReconPanel::setSource(const DicomVolume &before)
{
    if (m_jobActive) {
        m_panel->setStatus(QStringLiteral("重建进行中，取消或等待完成后再导入新序列。"));
        return;
    }
    ++m_sourceRevision;
    m_before = before;
    m_after  = DicomVolume();
    m_viewer->setVolumes(m_before, m_after);
    m_panel->setReconstructEnabled(true);
    m_panel->setStatus(QStringLiteral("已载入: %1层 %2×%3。点 ② 执行重建。")
                          .arg(m_before.depth()).arg(m_before.rows()).arg(m_before.cols()));
}

void ReconPanel::reconstruct(int upscale)
{
    Q_UNUSED(upscale);
    if (m_before.isEmpty() || !m_recon || m_jobActive) return;
    m_jobActive = true;
    m_jobSourceRevision = m_sourceRevision;
    m_upscale = m_recon->configuredUpscale();
    m_recon->setStride(m_panel->stride());
    m_panel->setReconstructEnabled(false);
    m_panel->setCancelEnabled(true);
    m_panel->setStatus(m_modelPath.isEmpty()
        ? QStringLiteral("正在 %1× 双三次面内超分重建…").arg(m_upscale)
        : QStringLiteral("正在 %1× SwinIR-Med (ONNX) 超分重建… 后台执行中").arg(m_upscale));
    m_recon->reconstructAsync(m_before, m_modelPath, m_panel->devicePreference());
}

void ReconPanel::requestCancel()
{
    if (!m_jobActive) return;
    m_panel->setCancelEnabled(false);
    m_panel->setStatus(QStringLiteral("正在取消重建，请稍候当前推理结束…"));
    m_recon->requestCancel();
}

void ReconPanel::onFinished(const SRStats &stats)
{
    m_progressTimer.stop();
    if (m_pendingProgressTotal > 0) {
        m_panel->setProgress(m_pendingProgressDone, m_pendingProgressTotal);
        emit progress(m_pendingProgressDone, m_pendingProgressTotal);
    }
    const bool currentJob = m_jobActive && m_jobSourceRevision == m_sourceRevision;
    if (!currentJob) return;
    m_jobActive = false;
    // 重建结束后刷新底部"引擎："标签, 显示本次实际使用的引擎与设备
    // (含 GPU 名称, 如 "CUDA:0 (NVIDIA GeForce RTX 5060 Laptop GPU)").
    // 注意: 构造时只能显示占位引擎(设备为"-"); 真实引擎在 worker 线程 run() 中创建,
    // 其设备串已通过 stats.engineDevice 带回到主线程, 必须在此处刷新才会显示.
    m_engineName    = stats.engineName;    // 供后续设备切换时保持真实引擎名
    m_engineDevice  = stats.engineDevice;
    m_panel->setEngineInfo(stats.engineName, stats.engineDevice);
    emit engineInfoChanged(stats.engineName, stats.engineDevice);  // 同步底部状态栏
    m_panel->setReconstructEnabled(true);
    m_panel->setCancelEnabled(false);
    if (stats.cancelled) {
        m_panel->setStatus(QStringLiteral("重建已取消。"));
        emit finished(stats);
        return;
    }
    m_panel->setProgress(stats.ok ? 1 : 0, 1);
    if (!stats.ok) {
        m_panel->setStatus(QStringLiteral("重建未完成: %1").arg(
            stats.error.isEmpty() ? QStringLiteral("未知错误") : stats.error));
        emit finished(stats);
        return;
    }
    m_after = m_recon->result();
    if (m_after.isEmpty()) {
        m_panel->setStatus(QStringLiteral("重建未完成: 未生成有效结果。"));
        emit finished(stats);
        return;
    }
    m_viewer->setVolumes(m_before, m_after);
    m_panel->setStatus(QStringLiteral("完成: %1 层 %2×%3 → %4×%5 (×%6), 耗时 %7 ms, %8")
                          .arg(stats.inSlices)
                          .arg(m_before.cols()).arg(m_before.rows())
                          .arg(m_after.cols()).arg(m_after.rows())
                          .arg(stats.upscale).arg(stats.elapsedMs)
                          .arg(stats.engineName));
    emit finished(stats);
}

} // namespace medical