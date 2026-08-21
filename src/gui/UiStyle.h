#pragma once

#include <QString>
#include <QApplication>

namespace medical::ui {

// ═══════════════════════════════════════════════════════════════════════
//  ImageJ Classic Light Theme  (Java Swing "Metal" style)
//
//  Reference: ImageJ1 classic interface
//    · White workstation surfaces with dark controls
//    · Black image viewport (medical image reading)
//    · Clean gray borders (#c0c0c0 -> #a0a0a0)
//    · Classic blue accent (#3366cc)
//    · ImageJ signature yellow cursor readout (status bar, monospace)
//
//  Palette:
//    Root       #ffffff  (white workstation surface)
//    Panel      #ffffff  (white panel)
//    Raised     #f8fafc  (subtle interaction state)
//    Control    #ffffff  (white control)
//    Input      #ffffff  (input bg)
//    Border     #c0c0c0  (primary border)
//    Soft       #dcdcdc  (soft border)
//    Text       #333333  (primary text)
//    Secondary  #666666  (secondary text)
//    Label      #888888  (label text)
//    Faint      #aaaaaa  (faint text)
//    Accent     #3366cc  (classic blue)
//    AccentFill #e8f0fe  (selected bg)
//    Viewport   #000000  (image viewport)
// ═══════════════════════════════════════════════════════════════════════
inline QString uiStyleSheet()
{
    return QStringLiteral(
        // ---- Base ----
        "QWidget {"
        "  background: #ffffff;"
        "  color: #111111;"
        "  font-family: \"Segoe UI\", \"Microsoft YaHei UI\", \"PingFang SC\", \"Source Han Sans SC\", sans-serif;"
        "  font-size: 12px;"
        "  outline: none;"
        "}"
        "* { border: none; }"
        "QLabel { background: transparent; }"

        // ---- Roles ----
        "QLabel[role=\"section\"] {"
        "  color: #3366cc; font-size: 11px; font-weight: 600;"
        "  letter-spacing: 0.08em; background: transparent; padding: 2px 0;"
        "}"
        "QLabel[role=\"subtitle\"] { color: #555555; font-size: 12px; background: transparent; }"
        "QLabel[role=\"muted\"]    { color: #888888; font-size: 11px; background: transparent; line-height: 1.5; }"
        "QLabel[role=\"status\"]   { color: #555555; font-size: 11px; background: transparent; }"
        "QLabel[role=\"mono\"] {"
        "  color: #444444;"
        "  font-family: \"Cascadia Code\", \"Consolas\", \"SF Mono\", monospace;"
        "  font-size: 11px; background: transparent;"
        "}"

        // ---- Title bar (brand) ----
        "QWidget#titleBar {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ffffff, stop:1 #e8e8e8);"
        "  border-bottom: 1px solid #c0c0c0;"
        "}"
        "QLabel#appTitle { color: #222222; font-size: 14px; font-weight: 700; letter-spacing: 0.04em; }"
        "QLabel#appSubtitle { color: #888888; font-size: 11px; letter-spacing: 0.06em; }"
        "QWidget#titleAccent { background: #3366cc; border-radius: 2px; min-width: 3px; max-width: 3px; }"
        "QWidget#appCentral, QWidget#workstationShell, QStackedWidget#workstationWorkspace,"
        "QWidget#viewerPanel, QWidget#srViewer, QWidget#reconViewer, QWidget#dicomContextPanel,"
        "QStackedWidget#sidebarContent, QStackedWidget#panelContent, QWidget#srControlPanel,"
        "QWidget#reconControlPanel, QWidget#aiAnalysisPanel, QFrame#aiWorkspaceHeader,"
        "QFrame#aiResourceSidebar, QFrame#aiWorkspaceEditor, QFrame#aiResultSidebar {"
        "  background: #ffffff;"
        "}"
        "QWidget#appCentral { border: 1px solid #d1d5db; }"

        "QPushButton#titleMenuBtn {"
        "  background: transparent; color: #444444; border: none; border-radius: 0;"
        "  padding: 0 12px; min-height: 36px; font-size: 12px;"
        "}"
        "QPushButton#titleMenuBtn:hover { background: #d8d8d8; color: #222222; }"
        "QPushButton#titleMenuBtn:pressed { background: #c0c0c0; }"
        "QPushButton#titleMenuBtn:checked,"
        "QPushButton#titleMenuBtn:open { background: #c0c0c0; color: #222222; }"

        "QPushButton#titleSysBtn {"
        "  background: transparent; color: #666666; border: none; border-radius: 0;"
        "  min-width: 40px; min-height: 36px; font-size: 14px; padding: 0;"
        "}"
        "QPushButton#titleSysBtn:hover { background: #d8d8d8; color: #222222; }"
        "QPushButton#titleSysBtnClose:hover { background: #c0392b; color: #ffffff; }"

        // ---- Patient / Study banner ----
        "QWidget#patientBanner { background: #ffffff; border-bottom: 1px solid #dcdcdc; }"
        "QLabel#bannerLabel {"
        "  color: #888888; font-size: 9px; font-weight: 600;"
        "  letter-spacing: 0.12em; text-transform: uppercase; background: transparent;"
        "}"
        "QLabel#bannerValue { color: #333333; font-size: 12px; font-weight: 500; background: transparent; }"
        "QLabel#bannerValue[accent=\"true\"] { color: #3366cc; font-weight: 700; }"
        "QLabel#bannerEmpty { color: #aaaaaa; font-size: 12px; font-style: italic; background: transparent; }"
        "QFrame#bannerSep { background: #dcdcdc; min-width: 1px; max-width: 1px; margin: 6px 0; }"

        // ---- ImageJ horizontal Tool Bar ----
        "QWidget#ijToolBar {"
        "  background: #ffffff;"
        "  border-bottom: 1px solid #d1d5db;"
        "  padding: 3px 4px;"
        "}"
        "QToolButton#ijToolBtn {"
        "  background: #ffffff;"
        "  border: 1px solid #d1d5db; border-radius: 3px;"
        "  color: #111111; min-width: 28px; max-width: 28px;"
        "  min-height: 28px; max-height: 28px; padding: 3px;"
        "}"
        "QToolButton#ijToolBtn:hover {"
        "  background: #f3f4f6; border: 1px solid #94a3b8; color: #000000;"
        "}"
        "QToolButton#ijToolBtn:pressed { background: #e5e7eb; border: 1px solid #94a3b8; }"
        "QToolButton#ijToolBtn:checked {"
        "  background: #dbeafe; border: 1px solid #2563eb; color: #111111;"
        "}"
        "QWidget#ijSep {"
        "  background: #d1d5db;"
        "  min-width: 1px; max-width: 1px; margin: 6px 4px;"
        "}"
        "QWidget#ijChipWrap { background: transparent; }"

        // ---- Primary sidebar ----
        "QWidget#primarySidebar, QStackedWidget#sidebarContent { background: #ffffff; border-right: 1px solid #d1d5db; }"
        "QWidget#sidebarTitleBar { background: #ffffff; border-bottom: 1px solid #d1d5db; }"
        "QLabel#sidebarTitle { color: #333333; font-size: 12px; font-weight: 600;"
        "  letter-spacing: 0.05em; background: transparent; }"
        "QToolButton#sidebarCollapse {"
        "  background: transparent; color: #888888; font-size: 14px; border: none; padding: 2px 6px;"
        "}"
        "QToolButton#sidebarCollapse:hover { background: #d8d8d8; color: #222222; }"

        // ---- Editor tab bar ----
        "QWidget#editorTabBar { background: #ffffff; border-bottom: 1px solid #cbd5e1; }"
        "QPushButton#editorTab {"
        "  background: #f8fafc; border-right: 1px solid #d1d5db;"
        "  border-top: 1px solid transparent; padding: 0 22px; color: #4b5563;"
        "  font-size: 12px; font-weight: 500;"
        "}"
        "QPushButton#editorTab:hover { background: #eef2f7; color: #111827; }"
        "QPushButton#editorTab:checked {"
        "  background: #ffffff; color: #1d4ed8; font-weight: 700;"
        "  border-top: 3px solid #2563eb; border-bottom: 1px solid #ffffff;"
        "}"

        // ---- Viewer toolbar (in-context) ----
        "QWidget#viewerToolbar { background: #ffffff; border-bottom: 1px solid #e5e7eb; }"
        "QFrame#viewerSep { background: #d1d5db; min-width: 1px; max-width: 1px; margin: 5px 2px; }"
        "QLabel#viewerToolLabel {"
        "  color: #888888; font-size: 9px; font-weight: 600;"
        "  letter-spacing: 0.12em; text-transform: uppercase; background: transparent; padding: 0 4px;"
        "}"

        // ---- Viewport frame (kept dark for diagnostic CT reading) ----
        "QFrame#viewportFrame { background: #000000; border: 1px solid #999999; border-radius: 3px; }"
        "QLabel#viewportTitle {"
        "  background: #1a1a1a; color: #cccccc;"
        "  font-family: \"Cascadia Code\", \"Consolas\", \"SF Mono\", monospace;"
        "  font-size: 11px; padding: 3px 8px; border-bottom: 1px solid #444444;"
        "}"
        "QLabel#viewportTitle[role=\"after\"] { color: #ffcc00; }"

        // ---- Bottom panel ----
        "QWidget#bottomPanel, QStackedWidget#panelContent { background: #ffffff; border-top: 1px solid #d1d5db; }"
        "QWidget#panelTitleBar { background: #ffffff; border-bottom: 1px solid #d1d5db; }"
        "QPushButton#panelTab {"
        "  color: #4b5563; font-size: 11px; letter-spacing: 0.04em;"
        "  background: transparent; padding: 0 10px; border-right: 1px solid #e5e7eb;"
        "}"
        "QPushButton#panelTab:checked {"
        "  color: #111111; background: #ffffff; border-bottom: 2px solid #2563eb;"
        "}"
        "QToolButton#panelAction {"
        "  background: #ffffff; color: #333333; border: 1px solid #d1d5db; border-radius: 3px;"
        "  padding: 3px 6px; font-size: 11px;"
        "}"
        "QToolButton#panelAction:hover { background: #f3f4f6; border: 1px solid #94a3b8; color: #111111; }"
        "QProgressBar {"
        "  background: #ffffff; border: 1px solid #cbd5e1; border-radius: 4px;"
        "  height: 10px; text-align: center; color: #4b5563; font-size: 10px;"
        "}"
        "QProgressBar::chunk {"
        "  background: #2563eb; border-radius: 3px;"
        "}"

        // ---- Push buttons (generic) ----
        "QPushButton {"
        "  background: #ffffff; color: #111111; border: 1px solid #cbd5e1; border-radius: 4px;"
        "  padding: 6px 12px; font-size: 12px;"
        "}"
        "QPushButton:hover { background: #f8fafc; border: 1px solid #94a3b8; color: #000000; }"
        "QPushButton:pressed { background: #e5e7eb; border: 1px solid #94a3b8; }"
        "QPushButton:disabled { background: #f3f4f6; color: #9ca3af; border: 1px solid #e5e7eb; }"
        "QPushButton[role=\"primary\"] {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #4488dd, stop:1 #3366cc);"
        "  border: 1px solid #2255aa; color: #ffffff; font-weight: 600;"
        "}"
        "QPushButton[role=\"primary\"]:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #5599ee, stop:1 #4477dd);"
        "}"
        "QPushButton[role=\"small\"] { padding: 3px 9px; font-size: 11px; }"
        "QPushButton[role=\"primary\"][role=\"small\"] { font-weight: 600; }"
        "QPushButton[role=\"danger\"] {"
        "  background: #f8f0f0; border: 1px solid #d0a0a0; color: #c0392b;"
        "}"
        "QPushButton[role=\"danger\"]:hover {"
        "  background: #fce8e8; border: 1px solid #c08080; color: #a02020;"
        "}"

        // ---- Inputs ----
        "QComboBox, QSpinBox, QDoubleSpinBox {"
        "  background: #ffffff; color: #333333; border: 1px solid #b0b0b0;"
        "  border-radius: 4px; padding: 4px 8px; min-height: 20px;"
        "}"
        "QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover { border: 1px solid #888888; }"
        "QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid #3366cc; }"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding; subcontrol-position: top right;"
        "  width: 18px; border: none;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #ffffff; color: #333333; border: 1px solid #b0b0b0;"
        "  selection-background-color: #c8daf0; selection-color: #1a3a5c;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button,"
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
        "  width: 16px; background: #e8e8e8; border: none;"
        "}"
        "QSpinBox::up-button:hover, QSpinBox::down-button:hover,"
        "QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {"
        "  background: #d0d0d0;"
        "}"

        // ---- Sliders ----
        "QSlider::groove:horizontal {"
        "  background: #d0d0d0; border: 1px solid #b0b0b0;"
        "  height: 4px; border-radius: 2px;"
        "}"
        "QSlider::sub-page:horizontal { background: #3366cc; border-radius: 2px; }"
        "QSlider::handle:horizontal {"
        "  background: #ffffff; border: 1px solid #888888;"
        "  width: 12px; height: 12px; margin: -5px 0; border-radius: 6px;"
        "}"
        "QSlider::handle:horizontal:hover { background: #f0f0f0; border: 1px solid #3366cc; }"

        // ---- Scroll areas & bars ----
        "QScrollArea { background: #ffffff; border: none; }"
        "QScrollBar:vertical { background: #ffffff; width: 10px; border: none; }"
        "QScrollBar::handle:vertical {"
        "  background: #c0c0c0; border-radius: 5px; min-height: 24px;"
        "}"
        "QScrollBar::handle:vertical:hover { background: #a0a0a0; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar:horizontal { background: #ffffff; height: 10px; border: none; }"
        "QScrollBar::handle:horizontal {"
        "  background: #c0c0c0; border-radius: 5px; min-width: 24px;"
        "}"
        "QScrollBar::handle:horizontal:hover { background: #a0a0a0; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"

        // ---- Group box ----
        "QGroupBox {"
        "  background: #ffffff; border: 1px solid #c0c0c0; border-radius: 5px;"
        "  margin-top: 14px; padding: 12px 10px 10px 10px;"
        "  color: #333333; font-size: 12px; font-weight: 600;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; subcontrol-position: top left;"
        "  left: 10px; padding: 0 6px;"
        "  color: #888888; font-size: 10px; font-weight: 600;"
        "  letter-spacing: 0.1em; text-transform: uppercase;"
        "}"

        // ---- Lists (series navigator) ----
        "QListWidget {"
        "  background: #ffffff; border: 1px solid #c0c0c0; border-radius: 4px;"
        "  color: #444444; outline: none;"
        "}"
        "QListWidget::item {"
        "  padding: 7px 9px; border-bottom: 1px solid #e8e8e8; color: #555555;"
        "}"
        "QListWidget::item:hover { background: #f0f0f0; }"
        "QListWidget::item:selected {"
        "  background: #c8daf0; color: #1a3a5c; border-left: 2px solid #3366cc;"
        "}"

        // ---- Plain text (output log) ----
        "QPlainTextEdit {"
        "  background: #ffffff; color: #111111; border: 1px solid #cbd5e1; border-radius: 4px;"
        "  font-family: \"Cascadia Code\", \"Consolas\", \"SF Mono\", monospace;"
        "  font-size: 11px;"
        "}"

        // ---- AI analysis readouts ----
        "QLabel#aiSection {"
        "  color: #3366cc; font-size: 11px; font-weight: 600;"
        "  letter-spacing: 0.1em; text-transform: uppercase; background: transparent;"
        "}"
        "QLabel#aiValue {"
        "  color: #222222; font-size: 20px; font-weight: 700;"
        "  background: transparent; padding: 2px 0;"
        "}"
        "QLabel#aiValue[severity=\"normal\"] { color: #27ae60; }"
        "QLabel#aiValue[severity=\"warn\"]   { color: #e67e22; }"
        "QLabel#aiValue[severity=\"high\"]   { color: #c0392b; }"

        // ---- Status bar ----
        "QStatusBar#statusBar {"
        "  background: #ffffff;"
        "  border-top: 1px solid #d1d5db;"
        "  color: #4b5563; font-size: 11px; padding: 0 2px;"
        "}"
        "QStatusBar#statusBar::item { border: none; }"
        "QLabel#statusPacs, QLabel#statusEngine, QLabel#statusInfer, QLabel#statusPanel,"
        "QLabel#statusProblems, QLabel#statusDb, QLabel#statusSliceInfo, QLabel#statusStudy, QLabel#statusFrame {"
        "  color: #4b5563; font-size: 11px; background: transparent;"
        "}"
        "QLabel#statusItem { color: #4b5563; font-size: 11px; background: transparent; }"
        "QLabel#statusItem[accent=\"true\"] {"
        "  color: #444444; border-left: 1px solid #d0d0d0; padding-left: 10px;"
        "}"
        "QLabel#statusCoord {"
        "  color: #996600;"
        "  font-family: \"Cascadia Code\", \"Consolas\", \"SF Mono\", monospace;"
        "  font-size: 11px; background: transparent;"
        "  padding: 0 10px 0 2px; border-right: 1px solid #d0d0d0;"
        "}"
        "QFrame#statusSep {"
        "  background: #d0d0d0; min-width: 1px; max-width: 1px; margin: 5px 6px;"
        "}"
        "QPushButton#statusBtn {"
        "  background: transparent; color: #666666; border: none;"
        "  border-radius: 3px; padding: 3px 9px; font-size: 11px;"
        "}"
        "QPushButton#statusBtn:hover { background: #e0e0e0; color: #222222; }"

        // ---- Menu bar / menus ----
        "QMenuBar {"
        "  background: #ffffff; color: #111111;"
        "  border-bottom: 1px solid #d1d5db; padding: 1px;"
        "}"
        "QMenuBar::item { padding: 4px 10px; background: transparent; }"
        "QMenuBar::item:selected { background: #c8daf0; color: #1a3a5c; }"
        "QMenu {"
        "  background: #ffffff; color: #333333;"
        "  border: 1px solid #b0b0b0; padding: 4px;"
        "}"
        "QMenu::item { padding: 5px 22px 5px 12px; }"
        "QMenu::item:selected { background: #c8daf0; color: #1a3a5c; }"
        "QMenu::separator { height: 1px; background: #dcdcdc; margin: 4px 6px; }"

        // ---- Tooltip ----
        "QToolTip {"
        "  background: #ffffdd; color: #333333;"
        "  border: 1px solid #888888; border-radius: 4px;"
        "  padding: 5px 8px; font-size: 11px;"
        "}"

        // ---- Table / Tree ----
        "QTableView, QTreeView {"
        "  background: #ffffff; color: #333333;"
        "  border: 1px solid #c0c0c0; border-radius: 4px;"
        "  gridline-color: #e0e0e0;"
        "  selection-background-color: #c8daf0; selection-color: #1a3a5c;"
        "}"
        "QHeaderView::section {"
        "  background: #ffffff; color: #111111;"
        "  border: 1px solid #d0d0d0; padding: 4px 8px; font-size: 11px; font-weight: 600;"
        "}"

        // ---- Splitter ----
        "QSplitter::handle { background: #e5e7eb; }"
        "QSplitter::handle:horizontal { width: 1px; }"
        "QSplitter::handle:vertical { height: 1px; }"

        // ---- Tab widget ----
        "QTabWidget::pane {"
        "  background: #ffffff; border: 1px solid #c0c0c0; border-radius: 4px;"
        "}"
        "QTabBar::tab {"
        "  background: #f8fafc; color: #4b5563;"
        "  border: 1px solid #d1d5db; border-bottom: none;"
        "  padding: 5px 12px; border-top-left-radius: 4px; border-top-right-radius: 4px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #ffffff; color: #222222; font-weight: 600;"
        "}"
        "QTabBar::tab:hover { background: #f0f0f0; }"
    );
}

inline void applyWorkstationTheme(QApplication &app)
{
    app.setStyleSheet(uiStyleSheet());
}

} // namespace medical::ui