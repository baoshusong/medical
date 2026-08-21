#include "gui/PrimarySidebar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QStackedWidget>
#include <QScrollArea>
#include <QSizePolicy>

namespace medical {

PrimarySidebar::PrimarySidebar(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("primarySidebar"));
    setFixedWidth(m_savedWidth);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Title bar ──
    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName(QStringLiteral("sidebarTitleBar"));
    m_titleBar->setFixedHeight(32);
    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(12, 0, 4, 0);
    titleLayout->setSpacing(4);

    m_titleLabel = new QLabel(QStringLiteral("控制面板"), m_titleBar);
    m_titleLabel->setObjectName(QStringLiteral("sidebarTitle"));

    m_collapseBtn = new QToolButton(m_titleBar);
    m_collapseBtn->setObjectName(QStringLiteral("sidebarCollapse"));
    m_collapseBtn->setText(QStringLiteral("\u25B2"));
    m_collapseBtn->setFixedSize(24, 24);
    m_collapseBtn->setToolTip(QStringLiteral("收起侧栏"));
    m_collapseBtn->setFocusPolicy(Qt::NoFocus);
    m_collapseBtn->setCursor(Qt::PointingHandCursor);
    connect(m_collapseBtn, &QToolButton::clicked, this, &PrimarySidebar::toggle);

    titleLayout->addWidget(m_titleLabel, 1);
    titleLayout->addWidget(m_collapseBtn);
    root->addWidget(m_titleBar);

    // ── Stacked content ──
    m_contentStack = new QStackedWidget(this);
    m_contentStack->setObjectName(QStringLiteral("sidebarContent"));
    m_contentStack->setFrameShape(QFrame::NoFrame);
    root->addWidget(m_contentStack, 1);
}

void PrimarySidebar::registerView(int viewIndex, QWidget *content, const QString &title)
{
    if (!content) return;

    // Add scroll area wrapper
    auto *scroll = new QScrollArea(m_contentStack);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // setWidget() reparents the content into the scroll area's viewport;
    // calling setParent(nullptr) first would briefly make it a top-level
    // window (creating a native HWND on Windows) and can crash.
    scroll->setWidget(content);

    int stackIdx = m_contentStack->addWidget(scroll);
    m_viewIndexToStackIndex[viewIndex] = stackIdx;
    m_viewTitles[viewIndex] = title;
}

void PrimarySidebar::switchToView(int viewIndex)
{
    auto it = m_viewIndexToStackIndex.find(viewIndex);
    if (it != m_viewIndexToStackIndex.end()) {
        m_contentStack->setCurrentIndex(it.value());
        m_titleLabel->setText(m_viewTitles.value(viewIndex, QStringLiteral("控制面板")));
    }
}

void PrimarySidebar::setVisible(bool visible)
{
    m_visible = visible;
    if (visible) {
        setFixedWidth(m_savedWidth);
    } else {
        m_savedWidth = qMax(width(), 200);
        setFixedWidth(0);
    }
    emit visibilityChanged(visible);
    QWidget::setVisible(visible);
}

void PrimarySidebar::toggle()
{
    setVisible(!m_visible);
}

} // namespace medical