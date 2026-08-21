#include "gui/AppToolBar.h"
#include <QAction>

namespace medical {

AppToolBar::AppToolBar(QWidget *parent)
    : QToolBar(QStringLiteral("工具"), parent)
{
    setMovable(false);
    setIconSize({ 20, 20 });

    m_open = addAction(QStringLiteral("打开检查"));
    m_open->setShortcut(QKeySequence::Open);
    connect(m_open, &QAction::triggered, this, &AppToolBar::openRequested);

    addSeparator();
    // 预设窗位由“查看”菜单触发；这里提供翻帧与 AI。
    m_prev = addAction(QStringLiteral("◀ 上一帧"));
    m_next = addAction(QStringLiteral("下一帧 ▶"));
    m_prev->setShortcut(Qt::Key_Left);
    m_next->setShortcut(Qt::Key_Right);
    connect(m_prev, &QAction::triggered, this, &AppToolBar::prevFrame);
    connect(m_next, &QAction::triggered, this, &AppToolBar::nextFrame);

    addSeparator();
    m_ai = addAction(QStringLiteral("🧠 运行 AI"));
    m_ai->setShortcut(QKeySequence(Qt::Key_F5));
    connect(m_ai, &QAction::triggered, this, &AppToolBar::runAi);

    addSeparator();
    m_export = addAction(QStringLiteral("导出报告"));
    connect(m_export, &QAction::triggered, this, &AppToolBar::exportRequested);
}

} // namespace medical
