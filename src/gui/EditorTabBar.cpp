#include "gui/EditorTabBar.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSizePolicy>

namespace medical {

EditorTabBar::EditorTabBar(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("editorTabBar"));
    setFixedHeight(42);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);
    m_layout->addStretch(1);
}

int EditorTabBar::addTab(const QIcon &icon, const QString &title)
{
    const int index = m_tabs.size();
    auto *tabBtn = new QPushButton(title, this);
    tabBtn->setObjectName(QStringLiteral("editorTab"));
    tabBtn->setIcon(icon);
    tabBtn->setIconSize(QSize(16, 16));
    tabBtn->setCheckable(true);
    tabBtn->setFocusPolicy(Qt::NoFocus);
    tabBtn->setCursor(Qt::PointingHandCursor);
    tabBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    tabBtn->setMinimumWidth(132);
    tabBtn->setMaximumWidth(210);
    tabBtn->setFixedHeight(42);

    connect(tabBtn, &QPushButton::clicked, this, [this, index] {
        emit tabClicked(index);
    });

    m_group->addButton(tabBtn, index);
    m_layout->insertWidget(m_layout->count() - 1, tabBtn);
    m_tabs.append({tabBtn});

    if (index == 0) tabBtn->setChecked(true);
    updateTabStyles();
    return index;
}

void EditorTabBar::setCurrentTab(int index)
{
    if (index < 0 || index >= m_tabs.size()) return;
    m_currentIndex = index;
    if (m_tabs[index].button) m_tabs[index].button->setChecked(true);
    updateTabStyles();
}

int EditorTabBar::currentTab() const
{
    return m_group->checkedId();
}

int EditorTabBar::count() const
{
    return m_tabs.size();
}

void EditorTabBar::updateTabStyles()
{
    // Styling is handled by QSS.
}

} // namespace medical
