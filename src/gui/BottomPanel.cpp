#include "gui/BottomPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QStackedWidget>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QButtonGroup>
#include <QSizePolicy>
#include <QLabel>

namespace medical {

BottomPanel::BottomPanel(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("bottomPanel"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Title bar with tab buttons ──
    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName(QStringLiteral("panelTitleBar"));
    m_titleBar->setFixedHeight(30);
    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(8, 0, 4, 0);
    titleLayout->setSpacing(0);

    m_tabGroup = new QButtonGroup(this);
    m_tabGroup->setExclusive(true);

    const struct { QString label; } tabs[] = {
        { QStringLiteral("问题")  },
        { QStringLiteral("输出")  },
        { QStringLiteral("进度")  }
    };
    for (int i = 0; i < 3; ++i) {
        auto *tabBtn = new QPushButton(tabs[i].label, m_titleBar);
        tabBtn->setObjectName(QStringLiteral("panelTab"));
        tabBtn->setCheckable(true);
        tabBtn->setFocusPolicy(Qt::NoFocus);
        tabBtn->setCursor(Qt::PointingHandCursor);
        m_tabGroup->addButton(tabBtn, i);
        titleLayout->addWidget(tabBtn);
        connect(tabBtn, &QPushButton::clicked, this, [this, i] {
            showPanel(i);
        });
    }

    titleLayout->addStretch(1);

    // Close button
    m_closeBtn = new QToolButton(m_titleBar);
    m_closeBtn->setObjectName(QStringLiteral("panelAction"));
    m_closeBtn->setText(QStringLiteral("\u2715"));
    m_closeBtn->setFixedSize(24, 24);
    m_closeBtn->setFocusPolicy(Qt::NoFocus);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setToolTip(QStringLiteral("关闭面板"));
    m_closeBtn->setAutoRaise(true);
    connect(m_closeBtn, &QToolButton::clicked, this, [this] { setVisible(false); });
    titleLayout->addWidget(m_closeBtn);

    root->addWidget(m_titleBar);

    // ── Stacked content ──
    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(QStringLiteral("panelContent"));

    // Problems tab
    m_problemsList = new QListWidget(m_stack);
    m_problemsList->setObjectName(QStringLiteral("problemsList"));
    m_problemsList->setFrameShape(QFrame::NoFrame);
    m_stack->addWidget(m_problemsList);

    // Output tab
    m_outputLog = new QPlainTextEdit(m_stack);
    m_outputLog->setObjectName(QStringLiteral("outputLog"));
    m_outputLog->setReadOnly(true);
    m_outputLog->setMaximumBlockCount(500);
    m_outputLog->setFrameShape(QFrame::NoFrame);
    m_stack->addWidget(m_outputLog);

    // Progress tab
    m_progressArea = new QWidget(m_stack);
    m_progressArea->setObjectName(QStringLiteral("panelContent"));
    auto *progressLayout = new QVBoxLayout(m_progressArea);
    progressLayout->setContentsMargins(16, 16, 16, 16);
    progressLayout->setSpacing(8);
    auto *progressLabel = new QLabel(QStringLiteral("重建进度"), m_progressArea);
    progressLabel->setStyleSheet(QStringLiteral("color: #666666; font-size: 12px;"));
    m_progressBar = new QProgressBar(m_progressArea);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setMinimumHeight(6);
    m_progressBar->setTextVisible(false);
    progressLayout->addWidget(progressLabel);
    progressLayout->addWidget(m_progressBar);
    progressLayout->addStretch(1);
    m_stack->addWidget(m_progressArea);

    root->addWidget(m_stack, 1);

    // Select first tab
    showPanel(0);
}

void BottomPanel::addProblem(const QString &text)
{
    m_problemsList->addItem(text);
}

void BottomPanel::clearProblems()
{
    m_problemsList->clear();
}

void BottomPanel::appendOutput(const QString &text)
{
    m_outputLog->appendPlainText(text);
}

void BottomPanel::clearOutput()
{
    m_outputLog->clear();
}

void BottomPanel::showPanel(int index)
{
    if (index < 0 || index >= m_stack->count()) return;
    m_stack->setCurrentIndex(index);
    auto *btn = m_tabGroup->button(index);
    if (btn) btn->setChecked(true);
}

void BottomPanel::showProblems()   { showPanel(0); }
void BottomPanel::showOutput()     { showPanel(1); }
void BottomPanel::showProgress()   { showPanel(2); }

void BottomPanel::setVisible(bool visible)
{
    m_visible = visible;
    if (visible) {
        if (m_savedHeight < 50) m_savedHeight = 150;
        setMaximumHeight(m_savedHeight);
    } else {
        m_savedHeight = qMax(height(), 100);
    }
    emit visibilityChanged(visible);
    QWidget::setVisible(visible);
}

void BottomPanel::toggle()
{
    setVisible(!m_visible);
}

} // namespace medical