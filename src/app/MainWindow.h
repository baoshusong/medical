#pragma once

#include <QCloseEvent>
#include <QMainWindow>
#include <QString>
#include <memory>

#include "core/Study.h"
#include "sr/DicomVolume.h"

class QStackedWidget;

namespace medical {

class AiAnalysisPanel;
class AppStatusBar;
class BottomPanel;
class BrightnessContrastDialog;
class EditorTabBar;
class HistogramDialog;
class ImageJToolBar;
class PatientBanner;
class PrimarySidebar;
class ReconPanel;
class SrViewer;
class ViewerPanel;

// ImageJ 风格集成工作站控制器:
//  - 顶部为紧凑的常驻工具条
//  - 中央为患者信息、模块标签、上下文侧栏与影像工作区
//  - 底部为可折叠输出/进度面板和持久状态栏
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *e) override;

private slots:
    void onImport();
    void onExportDicom();
    void onExportPng();
    void pushSourceToPanels();
    void loadDemoVolume();
    void openModule(int index);
    void onHistogram();
    void onBrightnessContrast();
    void onAbout();
    void ensureLog();
    void onTabClicked(int index);

private:
    QString resolveModelPath();
    QString resolveInterSliceModelPath();
    SrViewer *currentSrViewer() const;
    ReconPanel *activeReconPanel() const;
    void wireConnections();
    void buildWindowMenu();
    void updateContextSidebar();

    ImageJToolBar *m_ijToolBar = nullptr;
    PatientBanner *m_patientBanner = nullptr;
    EditorTabBar *m_editorTabs = nullptr;
    PrimarySidebar *m_sidebar = nullptr;
    QStackedWidget *m_workspace = nullptr;
    AppStatusBar *m_status = nullptr;
    QWidget *m_dicomContext = nullptr;
    ViewerPanel *m_view  = nullptr;
    ReconPanel *m_sr    = nullptr;
    ReconPanel *m_inter = nullptr;
    AiAnalysisPanel *m_ai  = nullptr;

    BottomPanel *m_bottomPanel = nullptr;
    BrightnessContrastDialog *m_bcDlg = nullptr;
    HistogramDialog *m_histDlg = nullptr;

    DicomVolume m_before;
    Study m_study;
    QString m_modelPath;
    int m_activeModule = 0;
};

} // namespace medical
