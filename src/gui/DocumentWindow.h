#pragma once

#include <QCloseEvent>
#include <QFocusEvent>
#include <QMainWindow>

namespace medical {

// ImageJ 风格独立文档窗口: 每个模块 (DICOM 查看 / 超分 / 层间超分 / AI 分析)
// 以独立顶层窗口呈现, 拥有自己的菜单栏。获得焦点时通知主控制器成为"活动窗口",
// 以接收工具条指令 (工具/缩放等)。关闭时把内容面板交还主控制器以便复用。
class DocumentWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit DocumentWindow(int moduleIndex, QWidget *content, QWidget *parent = nullptr);

    int moduleIndex() const { return m_module; }

signals:
    void activated(DocumentWindow *w);
    void closed(DocumentWindow *w);

protected:
    void closeEvent(QCloseEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;

private:
    int m_module = 0;
};

} // namespace medical
