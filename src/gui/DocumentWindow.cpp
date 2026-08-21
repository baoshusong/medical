#include "gui/DocumentWindow.h"

#include <QWidget>

namespace medical {

DocumentWindow::DocumentWindow(int moduleIndex, QWidget *content, QWidget *parent)
    : QMainWindow(parent)
    , m_module(moduleIndex)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    if (content)
        setCentralWidget(content);
    setMinimumSize(420, 320);
}

void DocumentWindow::closeEvent(QCloseEvent *e)
{
    // 把内容面板交还主控制器 (MainWindow), 避免随窗口销毁而丢失 (面板需复用)
    if (QWidget *c = centralWidget())
        c->setParent(parentWidget());
    emit closed(this);
    QMainWindow::closeEvent(e);
}

void DocumentWindow::focusInEvent(QFocusEvent *e)
{
    emit activated(this);
    QMainWindow::focusInEvent(e);
}

} // namespace medical
