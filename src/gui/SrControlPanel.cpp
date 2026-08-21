#include "gui/SrControlPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QProgressBar>
#include <QFrame>
#include <QButtonGroup>

namespace medical {

SrControlPanel::SrControlPanel(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);
    setObjectName(QStringLiteral("srControlPanel"));

    // ── Section: Input ──
    auto *inputLabel = new QLabel(QStringLiteral("输 入"));
    inputLabel->setProperty("role", "section");
    root->addWidget(inputLabel);

    m_import = new QPushButton(QStringLiteral("导入 DICOM / IMA 序列"));
    m_import->setProperty("role", "primary");
    m_import->setCursor(Qt::PointingHandCursor);
    connect(m_import, &QPushButton::clicked, this, &SrControlPanel::importRequested);
    root->addWidget(m_import);

    // ── Section: Parameters ──
    auto *paramLabel = new QLabel(QStringLiteral("参 数"));
    paramLabel->setProperty("role", "section");
    root->addWidget(paramLabel);

    auto *prow = new QHBoxLayout;
    prow->setSpacing(6);
    auto *upscaleLabel = new QLabel(QStringLiteral("超分倍数"));
    upscaleLabel->setProperty("role", "muted");
    prow->addWidget(upscaleLabel);
    m_upscale = new QComboBox;
    m_upscale->addItem(QStringLiteral("4x (SwinIR-Med)"), 4);
    m_upscale->setEnabled(false);
    prow->addWidget(m_upscale, 1);
    root->addLayout(prow);

    auto *srow = new QHBoxLayout;
    srow->setSpacing(6);
    auto *strideLabel = new QLabel(QStringLiteral("质量/速度"));
    strideLabel->setProperty("role", "muted");
    srow->addWidget(strideLabel);
    m_stride = new QComboBox;
    m_stride->addItem(QStringLiteral("全质量"), 1);
    m_stride->addItem(QStringLiteral("快速预览 4x"), 4);
    m_stride->addItem(QStringLiteral("极速预览 8x"), 8);
    srow->addWidget(m_stride, 1);
    root->addLayout(srow);

    // 计算设备选择: CPU / GPU 两个互斥按钮 (选 GPU 但无 GPU 时引擎自动回退 CPU)
    auto *drow = new QHBoxLayout;
    drow->setSpacing(6);
    auto *deviceLabel = new QLabel(QStringLiteral("计算设备"));
    deviceLabel->setProperty("role", "muted");
    drow->addWidget(deviceLabel);

    const QString devBtnStyle = QStringLiteral(
        "QPushButton{ border:1px solid #cbd5e1; border-radius:4px; padding:5px 8px;"
        " background:#ffffff; color:#111111; }"
        "QPushButton:checked{ background:#3366cc; color:#ffffff; border-color:#3366cc;"
        " font-weight:600; }");
    m_cpuBtn = new QPushButton(QStringLiteral("CPU"));
    m_gpuBtn = new QPushButton(QStringLiteral("GPU"));
    m_cpuBtn->setCheckable(true);
    m_gpuBtn->setCheckable(true);
    m_cpuBtn->setCursor(Qt::PointingHandCursor);
    m_gpuBtn->setCursor(Qt::PointingHandCursor);
    m_cpuBtn->setStyleSheet(devBtnStyle);
    m_gpuBtn->setStyleSheet(devBtnStyle);

    m_deviceGroup = new QButtonGroup(this);
    m_deviceGroup->addButton(m_cpuBtn, 0); // 0 -> CPU
    m_deviceGroup->addButton(m_gpuBtn, 1); // 1 -> GPU
    m_deviceGroup->setExclusive(true);
    m_gpuBtn->setChecked(true); // 默认 GPU；无 GPU 时引擎回退 CPU

    // 计算设备切换时立即通知面板刷新"引擎:"标签 (显示 CPU/GPU 型号), 无需等到重建结束
    connect(m_deviceGroup, QOverload<int>::of(&QButtonGroup::idClicked), this,
            [this](int id) { emit deviceChanged(id == 0 ? QStringLiteral("cpu") : QStringLiteral("gpu")); });

    drow->addWidget(m_cpuBtn);
    drow->addWidget(m_gpuBtn, 1);
    root->addLayout(drow);

    // ── Section: Reconstruct ──
    auto *reconLabel = new QLabel(QStringLiteral("重 建"));
    reconLabel->setProperty("role", "section");
    root->addWidget(reconLabel);

    m_recon = new QPushButton(QStringLiteral("一键超分重建"));
    m_recon->setProperty("role", "primary");
    m_recon->setCursor(Qt::PointingHandCursor);
    m_recon->setMinimumHeight(34);
    connect(m_recon, &QPushButton::clicked, this, [this]{
        emit reconstructRequested(m_upscale->currentData().toInt());
    });
    root->addWidget(m_recon);

    m_cancel = new QPushButton(QStringLiteral("取消重建"));
    m_cancel->setProperty("role", "danger");
    m_cancel->setEnabled(false);
    connect(m_cancel, &QPushButton::clicked, this, &SrControlPanel::cancelRequested);
    root->addWidget(m_cancel);

    m_prog = new QProgressBar;
    m_prog->setRange(0, 100);
    m_prog->setValue(0);
    m_prog->setFixedHeight(6);
    m_prog->setTextVisible(false);
    root->addWidget(m_prog);

    // Separator
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QStringLiteral("QFrame { color: #c0c0c0; }"));
    root->addWidget(sep);

    // ── Section: Export ──
    auto *expLabel = new QLabel(QStringLiteral("导 出"));
    expLabel->setProperty("role", "section");
    root->addWidget(expLabel);

    m_expDcm = new QPushButton(QStringLiteral("导出 DICOM 3.0"));
    m_expDcm->setCursor(Qt::PointingHandCursor);
    m_expPng = new QPushButton(QStringLiteral("导出 PNG / JPG"));
    m_expPng->setCursor(Qt::PointingHandCursor);
    connect(m_expDcm, &QPushButton::clicked, this, &SrControlPanel::exportDicomRequested);
    connect(m_expPng, &QPushButton::clicked, this, &SrControlPanel::exportPngRequested);
    root->addWidget(m_expDcm);
    root->addWidget(m_expPng);

    root->addStretch(1);

    // ── Engine info ──
    m_engine = new QLabel;
    m_engine->setProperty("role", "muted");
    m_engine->setWordWrap(true);
    root->addWidget(m_engine);

    // ── Status ──
    m_status = new QLabel;
    m_status->setWordWrap(true);
    m_status->setProperty("role", "status");
    m_status->setMinimumHeight(36);
    root->addWidget(m_status);
}

void SrControlPanel::setEngineInfo(const QString &name, const QString &device)
{
    // 标签只显示计算设备型号 (CPU/GPU), 不显示模型/引擎名称
    Q_UNUSED(name);
    m_engine->setText(device.isEmpty() ? QStringLiteral("引擎: --")
                                       : QStringLiteral("引擎: %1").arg(device));
}

void SrControlPanel::setProgress(int done, int total)
{
    m_prog->setMaximum(qMax(1, total));
    m_prog->setValue(done);
    setStatus(QStringLiteral("重建中... %1/%2 层").arg(done).arg(total));
}

void SrControlPanel::setReconstructEnabled(bool on) { m_recon->setEnabled(on); }
void SrControlPanel::setCancelEnabled(bool on) { m_cancel->setEnabled(on); }

void SrControlPanel::setReconstructLabel(const QString &s) { m_recon->setText(s); }

void SrControlPanel::setStatus(const QString &s) { m_status->setText(s); }

int SrControlPanel::stride() const { return m_stride ? m_stride->currentData().toInt() : 1; }
int SrControlPanel::scale()  const { return m_upscale ? m_upscale->currentData().toInt() : 4; }
QString SrControlPanel::devicePreference() const
{
    if (!m_deviceGroup) return QStringLiteral("gpu");
    return m_deviceGroup->checkedId() == 0 ? QStringLiteral("cpu") : QStringLiteral("gpu");
}

} // namespace medical
