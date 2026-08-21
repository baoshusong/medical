#pragma once

#include <QWidget>

class QStackedWidget;
class QListWidget;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QButtonGroup;
class QToolButton;
class QWidget;

namespace medical {

// VS Code-style collapsible bottom panel with tabs for Problems, Output, and Progress.
class BottomPanel : public QWidget
{
    Q_OBJECT
public:
    explicit BottomPanel(QWidget *parent = nullptr);

    QListWidget *problemsList() const { return m_problemsList; }
    QPlainTextEdit *outputLog() const { return m_outputLog; }
    QProgressBar *progressBar() const { return m_progressBar; }

    void addProblem(const QString &text);
    void clearProblems();
    void appendOutput(const QString &text);
    void clearOutput();

    void showProblems();
    void showOutput();
    void showProgress();

    void setVisible(bool visible);
    void toggle();
    bool isPanelVisible() const { return m_visible; }

signals:
    void visibilityChanged(bool visible);

private:
    void showPanel(int index);

    QWidget *m_titleBar = nullptr;
    QButtonGroup *m_tabGroup = nullptr;
    QToolButton *m_closeBtn = nullptr;
    QStackedWidget *m_stack = nullptr;
    QListWidget *m_problemsList = nullptr;
    QPlainTextEdit *m_outputLog = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QWidget *m_progressArea = nullptr;

    bool m_visible = true;
    bool m_maximized = false;
    int m_savedHeight = 150;
};

} // namespace medical