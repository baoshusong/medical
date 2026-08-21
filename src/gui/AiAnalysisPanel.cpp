#include "gui/AiAnalysisPanel.h"

#include "ai/AiModelDiscovery.h"
#include "ai/AiPipeline.h"
#include "core/DicomFrame.h"
#include "sr/HuNormalize.h"
#include <QComboBox>
#include <QElapsedTimer>
#include <QVector>
#include <QFrame>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

namespace medical {

namespace {
QLabel *makeValueLabel(QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setObjectName(QStringLiteral("aiValue"));
    label->setWordWrap(true);
    return label;
}

QLabel *makeSectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setProperty("role", "section");
    return label;
}

QFrame *makePanel(QWidget *parent, const QString &objectName)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName(objectName);
    return frame;
}
}

AiAnalysisPanel::AiAnalysisPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("aiAnalysisPanel"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    auto *header = makePanel(this, QStringLiteral("aiWorkspaceHeader"));
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 8, 12, 8);
    auto *headingBox = new QVBoxLayout;
    headingBox->setSpacing(1);
    auto *heading = new QLabel(QStringLiteral("AI 分析工作区"), header);
    heading->setObjectName(QStringLiteral("aiWorkspaceTitle"));
    auto *subtitle = new QLabel(QStringLiteral("检查上下文、模型状态与可追溯分析活动"), header);
    subtitle->setProperty("role", "muted");
    headingBox->addWidget(heading);
    headingBox->addWidget(subtitle);
    headerLayout->addLayout(headingBox, 1);
    m_workspaceStatus = new QLabel(QStringLiteral("等待导入影像"), header);
    m_workspaceStatus->setProperty("role", "status");
    headerLayout->addWidget(m_workspaceStatus);
    root->addWidget(header);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("aiWorkspaceSplitter"));
    splitter->setChildrenCollapsible(false);

    auto *left = makePanel(splitter, QStringLiteral("aiResourceSidebar"));
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(10, 10, 10, 10);
    leftLayout->setSpacing(8);
    leftLayout->addWidget(makeSectionTitle(QStringLiteral("分析资源"), left));

    auto *contextBox = new QGroupBox(QStringLiteral("检查上下文"), left);
    auto *contextForm = new QFormLayout(contextBox);
    contextForm->setContentsMargins(8, 12, 8, 8);
    contextForm->setSpacing(6);
    m_patient = makeValueLabel(contextBox);
    m_studyInfo = makeValueLabel(contextBox);
    contextForm->addRow(QStringLiteral("患者"), m_patient);
    contextForm->addRow(QStringLiteral("检查"), m_studyInfo);
    leftLayout->addWidget(contextBox);

    auto *dataBox = new QGroupBox(QStringLiteral("影像数据"), left);
    auto *dataForm = new QFormLayout(dataBox);
    dataForm->setContentsMargins(8, 12, 8, 8);
    dataForm->setSpacing(6);
    m_geometry = makeValueLabel(dataBox);
    m_dataSummary = makeValueLabel(dataBox);
    dataForm->addRow(QStringLiteral("几何"), m_geometry);
    dataForm->addRow(QStringLiteral("状态"), m_dataSummary);
    leftLayout->addWidget(dataBox);

    auto *engineBox = new QGroupBox(QStringLiteral("分析引擎"), left);
    auto *engineLayout = new QVBoxLayout(engineBox);
    engineLayout->setContentsMargins(8, 12, 8, 8);
    m_provider = new QComboBox(engineBox);
    populateProviders();
    connect(m_provider, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { updateProviderState(); });
    engineLayout->addWidget(m_provider);
    m_modalityStatus = new QLabel(engineBox);
    m_modalityStatus->setObjectName(QStringLiteral("aiModalityStatus"));
    m_modalityStatus->setWordWrap(true);
    engineLayout->addWidget(m_modalityStatus);
    m_engine = new QLabel(engineBox);
    m_engine->setWordWrap(true);
    m_engine->setProperty("role", "muted");
    engineLayout->addWidget(m_engine);
    leftLayout->addWidget(engineBox);
    leftLayout->addStretch(1);

    auto *center = makePanel(splitter, QStringLiteral("aiWorkspaceEditor"));
    auto *centerLayout = new QVBoxLayout(center);
    centerLayout->setContentsMargins(14, 12, 14, 12);
    centerLayout->setSpacing(10);
    auto *centerTitle = new QLabel(QStringLiteral("分析概览  ·  当前检查"), center);
    centerTitle->setObjectName(QStringLiteral("aiEditorTitle"));
    centerLayout->addWidget(centerTitle);
    auto *intro = new QLabel(
        QStringLiteral("这里显示来自当前 DICOM 检查的可验证信息。选择已配置的分析模型后，\n"
                       "才可以启动推理；当前版本不会根据影像生成未经验证的诊断结论。"), center);
    intro->setWordWrap(true);
    intro->setProperty("role", "muted");
    centerLayout->addWidget(intro);
    centerLayout->addWidget(makeSectionTitle(QStringLiteral("分析任务"), center));
    m_tasks = new QListWidget(center);
    m_tasks->setObjectName(QStringLiteral("aiTaskList"));
    m_tasks->addItem(QStringLiteral("肺部结节检测    ·    模型未配置"));
    m_tasks->addItem(QStringLiteral("器官/病灶分割  ·    模型未配置"));
    m_tasks->addItem(QStringLiteral("影像定量摘要    ·    待接入分析引擎"));
    m_tasks->setEnabled(false);
    centerLayout->addWidget(m_tasks, 1);
    auto *notice = new QLabel(QStringLiteral("提示：SwinIR-Med 4× 是面内超分重建模型，不作为诊断分析模型使用。"), center);
    notice->setObjectName(QStringLiteral("aiAnalysisNotice"));
    notice->setWordWrap(true);
    centerLayout->addWidget(notice);

    auto *right = makePanel(splitter, QStringLiteral("aiResultSidebar"));
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(10, 10, 10, 10);
    rightLayout->setSpacing(8);
    rightLayout->addWidget(makeSectionTitle(QStringLiteral("结果 / 活动"), right));
    m_resultStatus = new QLabel(QStringLiteral("暂无可验证分析结果"), right);
    m_resultStatus->setObjectName(QStringLiteral("aiResultStatus"));
    m_resultStatus->setWordWrap(true);
    rightLayout->addWidget(m_resultStatus);
    m_run = new QPushButton(QStringLiteral("运行分析"), right);
    m_run->setProperty("role", "primary");
    m_run->setEnabled(false);
    m_run->setToolTip(QStringLiteral("需要先配置真实分析模型"));
    connect(m_run, &QPushButton::clicked, this, &AiAnalysisPanel::onRunAnalysis);
    rightLayout->addWidget(m_run);
    rightLayout->addWidget(makeSectionTitle(QStringLiteral("活动日志"), right));
    m_log = new QPlainTextEdit(right);
    m_log->setObjectName(QStringLiteral("aiActivityLog"));
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(100);
    rightLayout->addWidget(m_log, 1);

    splitter->addWidget(left);
    splitter->addWidget(center);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({250, 620, 300});
    root->addWidget(splitter, 1);

    m_pipeline = new AiPipeline(this);

    updateSummary();
    updateProviderState();
}

void AiAnalysisPanel::populateProviders()
{
    if (!m_provider) return;
    m_provider->clear();
    const QList<AiModelInfo> models = discoverAnalysisModels();
    if (models.isEmpty()) {
        m_provider->addItem(QStringLiteral("传统定量摘要（无需模型）"), QStringLiteral("builtin-quant"));
    } else {
        for (const AiModelInfo &m : models) {
            const QString label = QStringLiteral("%1  (分析, %2 MB)")
                .arg(m.name).arg(m.sizeBytes / (1024 * 1024));
            m_provider->addItem(label, m.path);
        }
    }
}

QString AiAnalysisPanel::valueOrDash(const QString &value) const
{
    return value.trimmed().isEmpty() ? QStringLiteral("—") : value;
}

void AiAnalysisPanel::setSource(const DicomVolume &volume)
{
    m_volume = volume;
    updateSummary();
    appendLog(m_volume.isEmpty() ? QStringLiteral("未载入可分析的体数据")
                                 : QStringLiteral("已更新检查体数据：%1 层").arg(m_volume.depth()));
}

void AiAnalysisPanel::setStudy(const Study &study)
{
    m_study = study;
    updateSummary();
}

void AiAnalysisPanel::updateSummary()
{
    const auto &p = m_study.patient;
    m_patient->setText(QStringLiteral("%1  ·  %2  ·  %3")
        .arg(valueOrDash(p.name), valueOrDash(p.sex), valueOrDash(p.patientId)));
    m_studyInfo->setText(QStringLiteral("%1  %2\n检查号：%3\n序列：%4  ·  帧：%5")
        .arg(valueOrDash(m_study.modality), valueOrDash(m_study.description),
             valueOrDash(m_study.accessionNumber))
        .arg(m_study.seriesCount).arg(m_study.frameCount));

    if (m_volume.isEmpty()) {
        m_geometry->setText(QStringLiteral("暂无体数据"));
        m_dataSummary->setText(QStringLiteral("请先导入 DICOM/IMA 序列"));
        m_workspaceStatus->setText(QStringLiteral("等待导入影像"));
    } else {
        m_geometry->setText(QStringLiteral("%1 × %2 × %3\nX/Y：%4 × %5 mm")
            .arg(m_volume.cols()).arg(m_volume.rows()).arg(m_volume.depth())
            .arg(m_volume.spacingX(), 0, 'f', 3).arg(m_volume.spacingY(), 0, 'f', 3));
        m_dataSummary->setText(QStringLiteral("已载入，可供查看与后续模型使用"));
        m_workspaceStatus->setText(QStringLiteral("数据就绪  ·  未配置分析模型"));
    }
    m_engine->setText(QStringLiteral("当前：未配置诊断/检测/分割模型\n\n已有 SwinIR-Med 4× 仅用于面内超分重建。"));
    updateProviderState();
}

void AiAnalysisPanel::updateProviderState()
{
    if (!m_provider || !m_engine || !m_run || !m_modalityStatus) return;

    const QString data = m_provider->currentData().toString();
    const QString modality = m_study.modality.trimmed().toUpper();
    const bool hasData = !m_volume.isEmpty();

    m_engine->setText(QStringLiteral("引擎：%1").arg(
        data == QStringLiteral("builtin-quant") || data.isEmpty()
            ? QStringLiteral("传统算法（定量摘要）")
            : QStringLiteral("ONNX Runtime（分析模型）")));

    if (data == QStringLiteral("builtin-quant") || data.isEmpty()) {
        m_modalityStatus->setText(QStringLiteral("模态：%1\n状态：可用 — 传统定量摘要，无需模型。")
            .arg(modality.isEmpty() ? QStringLiteral("未知") : modality));
        m_run->setEnabled(hasData);
        m_run->setToolTip(hasData ? QStringLiteral("运行传统定量摘要") : QStringLiteral("请先导入影像"));
    } else {
        if (!hasOnnxRuntime()) {
            m_modalityStatus->setText(QStringLiteral("模态：%1\n状态：检测到分析模型，但运行时未启用 ONNX（构建需 USE_ONNXRUNTIME）。")
                .arg(modality.isEmpty() ? QStringLiteral("未知") : modality));
            m_run->setEnabled(false);
            m_run->setToolTip(QStringLiteral("需以 USE_ONNXRUNTIME=ON 重新构建"));
        } else {
            m_modalityStatus->setText(QStringLiteral("模态：%1\n状态：可用 — 检测到分析模型，将执行真实推理。")
                .arg(modality.isEmpty() ? QStringLiteral("未知") : modality));
            m_run->setEnabled(hasData);
            m_run->setToolTip(hasData ? QStringLiteral("运行分析模型推理") : QStringLiteral("请先导入影像"));
        }
    }
    appendLog(QStringLiteral("Provider 已切换：%1").arg(m_provider->currentText()));
}

void AiAnalysisPanel::appendLog(const QString &text)
{
    if (m_log) m_log->appendPlainText(text);
}

void AiAnalysisPanel::runAnalysis()
{
    onRunAnalysis();
}

void AiAnalysisPanel::onRunAnalysis()
{
    if (m_volume.isEmpty()) {
        appendLog(QStringLiteral("分析未启动：请先导入影像。"));
        return;
    }
    const QString data = m_provider ? m_provider->currentData().toString() : QString();
    if (data == QStringLiteral("builtin-quant") || data.isEmpty())
        computeQuantitativeSummary();   // 传统算法，无需模型
    else
        runModelInference(data);        // 真实 ONNX 推理
}

// ── 传统定量摘要：直接统计体数据，无需任何模型 ──────────────────────
void AiAnalysisPanel::computeQuantitativeSummary()
{
    QElapsedTimer t; t.start();
    const int D = m_volume.depth(), H = m_volume.rows(), W = m_volume.cols();
    const double sx = m_volume.spacingX(), sy = m_volume.spacingY(), sz = m_volume.spacingZ();
    const double voxelMm3 = sx * sy * sz;
    const qint64 N = qint64(D) * H * W;

    double sum = 0.0, vmin = 1e9, vmax = -1e9;
    qint64 lowCount = 0;           // HU < -100 (近似肺/空气)
    qint64 softCount = 0;          // HU > 0 (软组织/骨)
    m_volume.forEachUnit([&](float u) {
        const float hu = hu::unitToHu(u);
        sum += hu;
        if (hu < vmin) vmin = hu;
        if (hu > vmax) vmax = hu;
        if (hu < -100.0) ++lowCount;
        if (hu > 0.0)   ++softCount;
    });
    const double meanHu = N ? sum / double(N) : 0.0;
    const double totalCm3 = voxelMm3 * double(N) / 1000.0;
    const double lowCm3 = voxelMm3 * double(lowCount) / 1000.0;
    const double elapsed = t.elapsed() / 1000.0;

    m_tasks->setEnabled(true);
    m_tasks->clear();
    m_tasks->addItem(QStringLiteral("分析任务：定量摘要（已完成）"));
    m_tasks->addItem(QStringLiteral("体素总数：%1").arg(N));
    m_tasks->addItem(QStringLiteral("体积估算：%1 cm³").arg(totalCm3, 0, 'f', 1));
    m_tasks->addItem(QStringLiteral("HU 范围：%1 ~ %2").arg(vmin, 0, 'f', 0).arg(vmax, 0, 'f', 0));
    m_tasks->addItem(QStringLiteral("平均 HU：%1").arg(meanHu, 0, 'f', 1));
    m_tasks->addItem(QStringLiteral("低密度(<−100HU)占比：%1%").arg(100.0 * lowCount / double(N), 0, 'f', 1));
    m_tasks->addItem(QStringLiteral("低密度体积估算：%1 cm³").arg(lowCm3, 0, 'f', 1));
    m_tasks->addItem(QStringLiteral("软组织/骨(>0HU)占比：%1%").arg(100.0 * softCount / double(N), 0, 'f', 1));

    m_resultStatus->setText(QStringLiteral(
        "传统定量摘要完成（无需模型）。\n计算范围：%1 层 × %2×%3。\n"
        "结果仅供影像参考，不替代医生诊断。").arg(D).arg(H).arg(W));
    appendLog(QStringLiteral("定量摘要完成：%1 体素, 平均HU %2, 体积 %3 cm³, 用时 %4 s")
        .arg(N).arg(meanHu, 0, 'f', 1).arg(totalCm3, 0, 'f', 1).arg(elapsed, 0, 'f', 2));
}

// ── 真实 ONNX 推理：经 AiPipeline 调用 OnnxInferenceEngine ───────────
void AiAnalysisPanel::runModelInference(const QString &modelPath)
{
    if (!m_pipeline) { computeQuantitativeSummary(); return; }
    DicomFrame frame = makeRepresentativeFrame();
    appendLog(QStringLiteral("运行分析模型推理：%1").arg(modelPath));
    AiResult result = m_pipeline->run(frame, modelPath, QStringLiteral("segmentation"));
    showResult(result);
}

void AiAnalysisPanel::showResult(const AiResult &result)
{
    m_tasks->setEnabled(true);
    m_tasks->clear();
    m_tasks->addItem(QStringLiteral("分析任务：%1").arg(result.modelName));
    if (!result.detections.isEmpty()) {
        m_tasks->addItem(QStringLiteral("检出目标：%1 个").arg(result.detections.size()));
        int i = 1;
        for (const auto &d : result.detections) {
            m_tasks->addItem(QStringLiteral("  #%1 评分 %2").arg(i++).arg(d.score, 0, 'f', 2));
        }
    }
    if (result.totalPositive > 0)
        m_tasks->addItem(QStringLiteral("阳性体素：%1").arg(result.totalPositive));
    m_resultStatus->setText(result.summary.isEmpty()
        ? QStringLiteral("推理完成，无结构化结论。结果不替代医生诊断。")
        : result.summary + QStringLiteral("\n结果不替代医生诊断。"));
    appendLog(QStringLiteral("推理完成：%1 目标, 摘要: %2")
        .arg(result.detections.size()).arg(result.summary));
}

// 取中间层构造一张代表帧供单帧推理使用（保持原始 HU 信息）。
DicomFrame AiAnalysisPanel::makeRepresentativeFrame() const
{
    DicomFrame f;
    const int z = m_volume.depth() / 2;
    const int W = m_volume.cols(), H = m_volume.rows();
    f.width = W;
    f.height = H;
    f.spacingX = m_volume.spacingX();
    f.spacingY = m_volume.spacingY();
    f.sliceThickness = m_volume.spacingZ();
    QVector<ushort> raw(size_t(W) * H);
    int i = 0;
    m_volume.forEachUnitInPlane(0, z, [&](float u) {
        const float hu = hu::unitToHu(u);
        raw[i++] = quint16(qBound(0, int(hu + 1000.0f), 4095));
    });
    f.rawPixels = raw;
    return f;
}

} // namespace medical
