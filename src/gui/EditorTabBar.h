#pragma once

#include <QWidget>
#include <QList>
#include <QIcon>

class QHBoxLayout;
class QPushButton;
class QButtonGroup;

namespace medical {

// Persistent four-module navigation bar above the workspace.
class EditorTabBar : public QWidget
{
    Q_OBJECT
public:
    explicit EditorTabBar(QWidget *parent = nullptr);

    int addTab(const QIcon &icon, const QString &title);
    void setCurrentTab(int index);
    int currentTab() const;
    int count() const;

signals:
    void tabClicked(int index);

private:
    struct TabData {
        QPushButton *button = nullptr;
    };

    QHBoxLayout *m_layout = nullptr;
    QButtonGroup *m_group = nullptr;
    QList<TabData> m_tabs;
    int m_currentIndex = -1;

    void updateTabStyles();
};

} // namespace medical