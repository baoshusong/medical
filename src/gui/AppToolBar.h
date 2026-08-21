#pragma once

#include <QToolBar>
class QAction;

namespace medical {

// 顶部工具条：文件 / 窗宽窗位 / 翻帧 / AI 按钮 (对应 1.png 顶部)。
class AppToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit AppToolBar(QWidget *parent = nullptr);

    QAction *actionOpen()   const { return m_open; }
    QAction *actionExport() const { return m_export; }
    QAction *actionPrev()   const { return m_prev; }
    QAction *actionNext()   const { return m_next; }

signals:
    void openRequested();
    void exportRequested();
    void prevFrame();
    void nextFrame();
    void runAi();

private:
    QAction *m_open = nullptr, *m_export = nullptr, *m_prev = nullptr, *m_next = nullptr, *m_ai = nullptr;
};

} // namespace medical
