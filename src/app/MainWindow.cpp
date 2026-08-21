#include "app/MainWindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QImage>
#include <QImageWriter>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <memory>

#include "core/DicomFrame.h"
#include "core/Study.h"
#include "dicom/IDicomLoader.h"
#include "gui/AiAnalysisPanel.h"
#include "gui/AppStatusBar.h"
#include "gui/BottomPanel.h"
#include "gui/BrightnessContrastDialog.h"
#include "gui/EditorTabBar.h"
#include "gui/HistogramDialog.h"
#include "gui/ImageJToolBar.h"
#include "gui/PatientBanner.h"
#include "gui/PrimarySidebar.h"
#include "gui/ReconPanel.h"
#include "gui/ResultsWindow.h"
#include "gui/SrControlPanel.h"
#include "gui/SrViewer.h"
#include "gui/ViewerPanel.h"
#include "sr/BaseReconstructor.h"
#include "sr/DicomVolume.h"
#include "sr/HuNormalize.h"
#include "sr/InPlaneReconstructor.h"
#include "sr/InterSliceReconstructor.h"
#include "storage/DicomExporter.h"
#include "utils/Logger.h"

namespace medical {

namespace {

QWidget *makeDicomContextPanel(QWidget *parent, ImageJToolBar *toolBar,
                               const std::function<void()> &import,
                               const std::function<void()> &exportPng)
{
    auto *panel = new QWidget(parent);
    panel->setObjectName(QStringLiteral("dicomContextPanel"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("DICOM 查看"), panel);
    title->setProperty("role", "section");
    layout->addWidget(title);

    auto *hint = new QLabel(QStringLiteral("通过顶部工具条选择标注工具。影像区支持滚轮翻层、Ctrl+滚轮缩放，以及鼠标调窗和测量。"), panel);
    hint->setProperty("role", "muted");
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *importButton = new QPushButton(QStringLiteral("导入 DICOM / IMA 序列"), panel);
    importButton->setProperty("role", "primary");
    QObject::connect(importButton, &QPushButton::clicked, panel, import);
    layout->addWidget(importButton);

    auto *exportButton = new QPushButton(QStringLiteral("导出当前层面 PNG"), panel);
    QObject::connect(exportButton, &QPushButton::clicked, panel, exportPng);
    layout->addWidget(exportButton);

    auto *toolLabel = new QLabel(QStringLiteral("工具通过顶部 ImageJ 工具条作用于当前影像视图。"), panel);
    toolLabel->setProperty("role", "status");
    toolLabel->setWordWrap(true);
    layout->addWidget(toolLabel);
    layout->addStretch(1);

    Q_UNUSED(toolBar);
    return panel;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("AI 医学影像工作站"));
    setMinimumSize(1100, 700);
    resize(1440, 940);

    // 内置 Demo 体数据 (合成 ellipsoid 体)
    {
        const int D = 96, H = 256, W = 256;
        m_before.allocate(D, H, W, 0.7f, 0.7f, 1.0f);
        const double cx = W / 2.0, cy = H / 2.0, cz = D / 2.0;
        const double rx = W * 0.30, ry = H * 0.22, rz = D * 0.42;
        for (int z = 0; z < D; ++z)
            for (int y = 0; y < H; ++y)
                for (int x = 0; x < W; ++x) {
                    const double nx = (x - cx) / rx, ny = (y - cy) / ry, nz = (z - cz) / rz;
                    const double r = nx * nx + ny * ny + nz * nz;
                    const float hu = r <= 1.0f ? 40.0f : -1000.0f;
                    m_before.setVoxel(z, y, x, hu::huToUnit(hu));
                }
    }
    m_study = {};
    m_study.studyUid = QStringLiteral("DEMO-0001");
    m_study.modality = QStringLiteral("CT");
    m_study.description = QStringLiteral("胸部CT平扫");
    m_study.dateTime = QDateTime(QDate(2026, 1, 1), QTime(0, 0));
    m_study.seriesCount = 1;
    m_study.frameCount = m_before.depth();
    m_study.patient.name = QStringLiteral("匿名被试");

    m_view  = new ViewerPanel(this);
    m_sr    = new ReconPanel(std::make_unique<InPlaneReconstructor>(), QStringLiteral("面内超分重建"), this);
    m_inter = new ReconPanel(std::make_unique<InterSliceReconstructor>(), QStringLiteral("层间超分重建"), this);
    m_ai    = new AiAnalysisPanel(this);

    const QString mp = resolveModelPath();
    m_modelPath = mp;
    m_sr->setModelPath(mp);
    m_inter->setModelPath(resolveInterSliceModelPath());

    auto *shell = new QWidget(this);
    shell->setObjectName(QStringLiteral("workstationShell"));
    auto *shellLayout = new QVBoxLayout(shell);
    shellLayout->setContentsMargins(0, 0, 0, 0);
    shellLayout->setSpacing(0);

    m_ijToolBar = new ImageJToolBar(shell);
    shellLayout->addWidget(m_ijToolBar);

    m_patientBanner = new PatientBanner(shell);
    shellLayout->addWidget(m_patientBanner);

    m_editorTabs = new EditorTabBar(shell);
    m_editorTabs->addTab(QIcon(), QStringLiteral("DICOM 查看器"));
    m_editorTabs->addTab(QIcon(), QStringLiteral("面内超分"));
    m_editorTabs->addTab(QIcon(), QStringLiteral("层间超分"));
    m_editorTabs->addTab(QIcon(), QStringLiteral("AI 分析"));
    shellLayout->addWidget(m_editorTabs);

    auto *workspaceRow = new QSplitter(Qt::Horizontal, shell);
    workspaceRow->setObjectName(QStringLiteral("workstationSplitter"));
    workspaceRow->setChildrenCollapsible(false);
    m_sidebar = new PrimarySidebar(workspaceRow);
    m_workspace = new QStackedWidget(workspaceRow);
    m_workspace->setObjectName(QStringLiteral("workstationWorkspace"));
    m_workspace->addWidget(m_view);
    m_workspace->addWidget(m_sr);
    m_workspace->addWidget(m_inter);
    m_workspace->addWidget(m_ai);

    m_dicomContext = makeDicomContextPanel(m_sidebar, m_ijToolBar,
        [this] { onImport(); }, [this] { onExportPng(); });
    m_sidebar->registerView(0, m_dicomContext, QStringLiteral("DICOM 工具"));
    m_sidebar->registerView(1, m_sr->controlPanel(), QStringLiteral("面内超分控制"));
    m_sidebar->registerView(2, m_inter->controlPanel(), QStringLiteral("层间超分控制"));

    workspaceRow->addWidget(m_sidebar);
    workspaceRow->addWidget(m_workspace);
    workspaceRow->setStretchFactor(0, 0);
    workspaceRow->setStretchFactor(1, 1);
    workspaceRow->setSizes({300, 1140});
    shellLayout->addWidget(workspaceRow, 1);

    m_bottomPanel = new BottomPanel(shell);
    m_bottomPanel->setVisible(false);
    shellLayout->addWidget(m_bottomPanel);

    setCentralWidget(shell);

    m_status = new AppStatusBar(this);
    m_status->statusBar()->setObjectName(QStringLiteral("statusBar"));
    setStatusBar(m_status->statusBar());

    m_bcDlg  = new BrightnessContrastDialog([this]() { return currentSrViewer(); }, this);
    m_histDlg = new HistogramDialog([this]() { return currentSrViewer(); }, this);

    buildWindowMenu();
    wireConnections();
    openModule(0);
    QTimer::singleShot(50, this, &MainWindow::loadDemoVolume);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    e->accept();
}

void MainWindow::wireConnections()
{
    auto selectTool = [this](int tool) {
        m_ijToolBar->setActiveTool(tool);
        if (auto *v = currentSrViewer()) v->setActiveTool(tool);
    };
    connect(m_ijToolBar, &ImageJToolBar::toolRequested, this, selectTool);
    connect(m_ijToolBar, &ImageJToolBar::zoomInRequested, this, [this]() {
        if (auto *v = currentSrViewer()) v->zoomIn();
    });
    connect(m_ijToolBar, &ImageJToolBar::zoomOutRequested, this, [this]() {
        if (auto *v = currentSrViewer()) v->zoomOut();
    });
    connect(m_editorTabs, &EditorTabBar::tabClicked, this, &MainWindow::onTabClicked);

    connect(m_view,  &ViewerPanel::importRequested, this, &MainWindow::onImport);
    connect(m_view,  &ViewerPanel::exportPngRequested, this, &MainWindow::onExportPng);
    connect(m_sr,    &ReconPanel::importRequested, this, &MainWindow::onImport);
    connect(m_inter, &ReconPanel::importRequested, this, &MainWindow::onImport);

    connect(m_sr,    &ReconPanel::exportDicomRequested, this, &MainWindow::onExportDicom);
    connect(m_inter, &ReconPanel::exportDicomRequested, this, &MainWindow::onExportDicom);
    connect(m_sr,    &ReconPanel::exportPngRequested, this, &MainWindow::onExportPng);
    connect(m_inter, &ReconPanel::exportPngRequested, this, &MainWindow::onExportPng);

    auto progress = [this](int d, int t) {
        ensureLog();
        m_bottomPanel->showProgress();
        m_bottomPanel->progressBar()->setMaximum(t);
        m_bottomPanel->progressBar()->setValue(d);
        m_status->setInfer(QStringLiteral("重建中 %1/%2").arg(d).arg(t));
    };
    connect(m_sr,    &ReconPanel::progress, this, progress);
    connect(m_inter, &ReconPanel::progress, this, progress);

    auto reportFinished = [this](ReconPanel *panel, const SRStats &stats) {
        Q_UNUSED(panel);
        ensureLog();
        m_bottomPanel->showOutput();
        m_bottomPanel->appendOutput(QStringLiteral("重建完成: 输入 %1 层 -> 输出 %2 层, 耗时 %3 ms")
            .arg(stats.inSlices).arg(stats.outSlices).arg(stats.elapsedMs));
        m_status->setInfer(stats.ok ? QStringLiteral("重建完成") : QStringLiteral("重建失败"));
    };
    connect(m_sr, &ReconPanel::finished, this,
            [this, reportFinished](const SRStats &stats) { reportFinished(m_sr, stats); });
    connect(m_inter, &ReconPanel::finished, this,
            [this, reportFinished](const SRStats &stats) { reportFinished(m_inter, stats); });

    auto engine = [this](const QString &name, const QString &dev) {
        m_status->setEngine(name, dev);
    };
    connect(m_sr,    &ReconPanel::engineInfoChanged, this, engine);
    connect(m_inter, &ReconPanel::engineInfoChanged, this, engine);

    for (SrViewer *viewer : {m_view->viewer(), m_sr->viewer(), m_inter->viewer()}) {
        connect(viewer, &SrViewer::cursorReadout, this, [this](const QString &readout) {
            m_status->setCoordValue(readout);
        });
    }

    const QList<QKeySequence> moduleShortcuts = {
        QKeySequence(QStringLiteral("Ctrl+1")), QKeySequence(QStringLiteral("Ctrl+2")),
        QKeySequence(QStringLiteral("Ctrl+3")), QKeySequence(QStringLiteral("Ctrl+4"))
    };
    for (int i = 0; i < moduleShortcuts.size(); ++i) {
        auto *shortcut = new QShortcut(moduleShortcuts[i], this);
        connect(shortcut, &QShortcut::activated, this, [this, i] { openModule(i); });
    }
}

void MainWindow::onTabClicked(int index)
{
    openModule(index);
}

void MainWindow::openModule(int index)
{
    if (index < 0 || index > 3) return;

    m_activeModule = index;
    m_workspace->setCurrentIndex(index);
    m_editorTabs->setCurrentTab(index);
    updateContextSidebar();

    const QString titles[] = {
        QStringLiteral("DICOM 查看器"), QStringLiteral("面内超分重建"),
        QStringLiteral("层间超分重建"), QStringLiteral("AI 分析")
    };
    m_status->setActivePanel(titles[index]);
    m_status->setInfer(QStringLiteral("就绪"));
}

void MainWindow::updateContextSidebar()
{
    const bool hasContextSidebar = m_activeModule != 3;
    m_sidebar->setVisible(hasContextSidebar);
    if (hasContextSidebar)
        m_sidebar->switchToView(m_activeModule);
}

void MainWindow::buildWindowMenu()
{
    menuBar()->clear();

    QMenu *file = menuBar()->addMenu(QStringLiteral("文件(&F)"));
    file->addAction(QStringLiteral("导入 DICOM 序列..."), this, &MainWindow::onImport,
                    QKeySequence::Open);
    file->addAction(QStringLiteral("导出当前层面 PNG..."), this, &MainWindow::onExportPng);
    file->addAction(QStringLiteral("导出重建 DICOM..."), this, &MainWindow::onExportDicom);
    file->addSeparator();
    file->addAction(QStringLiteral("退出"), qApp, &QApplication::quit);

    QMenu *image = menuBar()->addMenu(QStringLiteral("图像(&I)"));
    QMenu *presets = image->addMenu(QStringLiteral("窗位预设"));
    struct Preset { QString name; int wc, ww; };
    const Preset ps[] = {
        {QStringLiteral("软组织"), 40, 400}, {QStringLiteral("肺窗"), -600, 1500},
        {QStringLiteral("骨窗"), 500, 2000}, {QStringLiteral("脑窗"), 40, 80},
    };
    for (const Preset &p : ps)
        presets->addAction(p.name, this, [this, p] { if (auto *v = currentSrViewer()) v->setWindowLevel(p.wc, p.ww); });
    image->addSeparator();
    image->addAction(QStringLiteral("放大"), this, [this] { if (auto *v = currentSrViewer()) v->zoomIn(); });
    image->addAction(QStringLiteral("缩小"), this, [this] { if (auto *v = currentSrViewer()) v->zoomOut(); });
    image->addAction(QStringLiteral("适应窗口"), this, [this] { if (auto *v = currentSrViewer()) v->resetView(); });
    image->addSeparator();
    image->addAction(QStringLiteral("水平翻转"), this, [this] { if (auto *v = currentSrViewer()) v->flipHorizontal(); });
    image->addAction(QStringLiteral("垂直翻转"), this, [this] { if (auto *v = currentSrViewer()) v->flipVertical(); });
    image->addAction(QStringLiteral("顺时针旋转"), this, [this] { if (auto *v = currentSrViewer()) v->rotateCW(); });
    image->addAction(QStringLiteral("逆时针旋转"), this, [this] { if (auto *v = currentSrViewer()) v->rotateCCW(); });
    image->addAction(QStringLiteral("反相"), this, [this] { if (auto *v = currentSrViewer()) v->setInvert(!v->invert()); });
    image->addAction(QStringLiteral("比例尺"), this, [this] { if (auto *v = currentSrViewer()) v->setScaleBar(!v->scaleBar()); });

    QMenu *tools = menuBar()->addMenu(QStringLiteral("工具(&T)"));
    struct Tool { QString name; int id; };
    const Tool toolList[] = {
        {QStringLiteral("距离"), ImageJToolBar::ToolDistance},
        {QStringLiteral("角度"), ImageJToolBar::ToolAngle},
        {QStringLiteral("箭头"), ImageJToolBar::ToolArrow},
        {QStringLiteral("框选"), ImageJToolBar::ToolBox},
        {QStringLiteral("椭圆"), ImageJToolBar::ToolEllipse},
        {QStringLiteral("文字标注"), ImageJToolBar::ToolText},
    };
    for (const Tool &tool : toolList)
        tools->addAction(tool.name, this, [this, tool] {
            m_ijToolBar->setActiveTool(tool.id);
            if (auto *v = currentSrViewer()) v->setActiveTool(tool.id);
        });
    tools->addSeparator();
    tools->addAction(QStringLiteral("清除标注"), this, [this] { if (auto *v = currentSrViewer()) v->clearAnnotations(); });
    tools->addAction(QStringLiteral("十字定位线"), this, [this] {
        if (auto *v = currentSrViewer()) v->setCrosshairVisible(!v->crosshairVisible());
    });

    QMenu *analysis = menuBar()->addMenu(QStringLiteral("分析(&A)"));
    analysis->addAction(QStringLiteral("直方图"), this, &MainWindow::onHistogram);
    analysis->addAction(QStringLiteral("ROI 测量"), this, [this] { if (auto *v = currentSrViewer()) v->measureROI(); });
    analysis->addAction(QStringLiteral("亮度/对比度"), this, &MainWindow::onBrightnessContrast);
    analysis->addAction(QStringLiteral("运行 AI 分析"), m_ai, &AiAnalysisPanel::runAnalysis);
    analysis->addAction(QStringLiteral("清除结果窗"), this, [] { ResultsWindow::instance()->clearResults(); });

    QMenu *process = menuBar()->addMenu(QStringLiteral("处理(&P)"));
    process->addAction(QStringLiteral("超分重建"), this, [this] {
        if (auto *panel = activeReconPanel()) panel->reconstruct(panel->controlPanel()->scale());
    });
    process->addAction(QStringLiteral("取消重建"), this, [this] {
        if (auto *panel = activeReconPanel()) panel->requestCancel();
    });

    QMenu *window = menuBar()->addMenu(QStringLiteral("窗口(&W)"));
    window->addAction(QStringLiteral("DICOM 查看器"), this, [this] { openModule(0); }, QKeySequence(QStringLiteral("Ctrl+1")));
    window->addAction(QStringLiteral("面内超分"), this, [this] { openModule(1); }, QKeySequence(QStringLiteral("Ctrl+2")));
    window->addAction(QStringLiteral("层间超分"), this, [this] { openModule(2); }, QKeySequence(QStringLiteral("Ctrl+3")));
    window->addAction(QStringLiteral("AI 分析"), this, [this] { openModule(3); }, QKeySequence(QStringLiteral("Ctrl+4")));
    window->addSeparator();
    window->addAction(QStringLiteral("输出 / 进度"), this, &MainWindow::ensureLog);
    window->addAction(QStringLiteral("切换控制侧栏"), this, [this] { if (m_activeModule != 3) m_sidebar->toggle(); });

    menuBar()->addMenu(QStringLiteral("帮助(&H)"))->addAction(QStringLiteral("关于"), this, &MainWindow::onAbout);
}

void MainWindow::pushSourceToPanels()
{
    m_patientBanner->setStudy(m_study);
    m_view->setStudy(m_study);
    m_view->setSource(m_before);
    m_sr->setSource(m_before);
    m_inter->setSource(m_before);
    m_ai->setSource(m_before);
    m_ai->setStudy(m_study);
    m_view->viewer()->setWindowLevel(40, 400);
    m_view->viewer()->resetView();
    m_status->setStudy(QStringLiteral("%1 · %2").arg(m_study.patient.name, m_study.description));
    m_status->setFrame(0, m_before.depth());
}

void MainWindow::loadDemoVolume()
{
    pushSourceToPanels();
    LOG_INFO("app", QStringLiteral("内置 Demo 体数据已加载"));
    m_bottomPanel->appendOutput(QStringLiteral("[app] 内置 Demo 体数据已加载"));
}

QString MainWindow::resolveModelPath()
{
    const QString path = QApplication::applicationDirPath()
                         + QStringLiteral("/model/swinir_med_4x.onnx");
    if (!QFile::exists(path))
        LOG_ERR("sr", QStringLiteral("面内超分模型不存在: %1").arg(path));
    return path;
}

QString MainWindow::resolveInterSliceModelPath()
{
    const QString path = QApplication::applicationDirPath()
                         + QStringLiteral("/model/edsr_invsr_width4x.onnx");
    if (!QFile::exists(path))
        LOG_ERR("sr", QStringLiteral("层间超分模型不存在: %1").arg(path));
    return path;
}

SrViewer *MainWindow::currentSrViewer() const
{
    switch (m_activeModule) {
    case 0: return m_view->viewer();
    case 1: return m_sr->viewer();
    case 2: return m_inter->viewer();
    default: return nullptr;
    }
}

ReconPanel *MainWindow::activeReconPanel() const
{
    if (m_activeModule == 1) return m_sr;
    if (m_activeModule == 2) return m_inter;
    return nullptr;
}

void MainWindow::onImport()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择 DICOM 序列目录"), QString());
    if (dir.isEmpty()) return;

    auto loader = IDicomLoader::create();
    Study study;
    if (!loader->load(dir, study)) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("DICOM 加载失败"));
        return;
    }
    const QVector<DicomFrame> frames = loader->frames();
    if (frames.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("未读取到切片"));
        return;
    }
    DicomVolume vol;
    vol.fromFrames(frames);
    m_before = vol;
    m_study = study;
    pushSourceToPanels();
    ensureLog();
    m_bottomPanel->showOutput();
    m_bottomPanel->appendOutput(QStringLiteral("[app] 导入完成: %1 帧").arg(frames.size()));
    LOG_INFO("app", QStringLiteral("导入完成, 体素 %1x%2x%3").arg(vol.cols()).arg(vol.rows()).arg(vol.depth()));
}

void MainWindow::onExportDicom()
{
    ReconPanel *panel = activeReconPanel();
    if (!panel || panel->isRunning()) return;
    const DicomVolume &after = panel->result();
    if (after.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先执行超分重建"));
        return;
    }
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择导出目录"), QString());
    if (dir.isEmpty()) return;
    DicomExporter::exportDicomSeries(after, m_study, dir, panel->controlPanel()->scale());
    ensureLog();
    m_bottomPanel->showOutput();
    m_bottomPanel->appendOutput(QStringLiteral("[导出] DICOM 序列 -> %1").arg(dir));
}

void MainWindow::onExportPng()
{
    SrViewer *viewer = currentSrViewer();
    if (!viewer) return;
    const SrViewer::SliceContext ctx = viewer->activeContext();
    if (!ctx.volume) return;
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出 PNG"), QString(), QStringLiteral("PNG (*.png)"));
    if (path.isEmpty()) return;
    DicomExporter::exportPngPlane(*ctx.volume, ctx.plane, ctx.slice, viewer->windowCenter(), viewer->windowWidth(), path);
}

void MainWindow::onHistogram()
{
    if (!currentSrViewer()) return;
    m_histDlg->recompute();
    m_histDlg->show();
    m_histDlg->raise();
}

void MainWindow::onBrightnessContrast()
{
    if (!currentSrViewer()) return;
    m_bcDlg->syncFromViewer();
    m_bcDlg->show();
    m_bcDlg->raise();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, QStringLiteral("关于"),
        QStringLiteral("AI 医学影像工作站\nImageJ 风格集成影像查看与分析工具。"));
}

void MainWindow::ensureLog()
{
    if (m_bottomPanel && !m_bottomPanel->isVisible())
        m_bottomPanel->setVisible(true);
}

} // namespace medical
