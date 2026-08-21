#include "BrightnessContrastDialog.h"
#include "SrViewer.h"
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QShowEvent>

namespace medical {

BrightnessContrastDialog::BrightnessContrastDialog(std::function<SrViewer *()> getViewer,
                                                   QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::WindowStaysOnTopHint)
    , m_getViewer(std::move(getViewer))
{
    setWindowTitle(QStringLiteral("亮度/对比度 (ImageJ)"));
    setMinimumWidth(320);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    m_wc = new QSlider(Qt::Horizontal, this);
    m_wc->setRange(-2000, 4000);
    m_wc->setSingleStep(1);
    m_wcVal = new QLabel(QStringLiteral("0"), this);

    m_ww = new QSlider(Qt::Horizontal, this);
    m_ww->setRange(1, 4000);
    m_ww->setSingleStep(1);
    m_wwVal = new QLabel(QStringLiteral("0"), this);

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("窗位 (WL):"), m_wc);
    form->addRow(QStringLiteral("窗宽 (WW):"), m_ww);
    layout->addLayout(form);

    auto *valRow = new QHBoxLayout;
    valRow->addWidget(new QLabel(QStringLiteral("WL ="), this));
    valRow->addWidget(m_wcVal);
    valRow->addSpacing(16);
    valRow->addWidget(new QLabel(QStringLiteral("WW ="), this));
    valRow->addWidget(m_wwVal);
    valRow->addStretch(1);
    layout->addLayout(valRow);

    auto *btns = new QHBoxLayout;
    auto *reset = new QPushButton(QStringLiteral("复位"), this);
    auto *close = new QPushButton(QStringLiteral("关闭"), this);
    btns->addWidget(reset);
    btns->addStretch(1);
    btns->addWidget(close);
    layout->addLayout(btns);

    auto apply = [this] {
        if (auto *v = m_getViewer()) {
            m_wcVal->setText(QString::number(m_wc->value()));
            m_wwVal->setText(QString::number(m_ww->value()));
            v->setWindowLevel(float(m_wc->value()), float(m_ww->value()));
        }
    };
    connect(m_wc, &QSlider::valueChanged, this, apply);
    connect(m_ww, &QSlider::valueChanged, this, apply);
    connect(reset, &QPushButton::clicked, this, [this] {
        m_wc->setValue(-600);
        m_ww->setValue(1500);
    });
    connect(close, &QPushButton::clicked, this, &QWidget::close);

    syncFromViewer();
}

void BrightnessContrastDialog::syncFromViewer()
{
    if (auto *v = m_getViewer()) {
        m_wc->setValue(qRound(v->windowCenter()));
        m_ww->setValue(qRound(v->windowWidth()));
        m_wcVal->setText(QString::number(m_wc->value()));
        m_wwVal->setText(QString::number(m_ww->value()));
    }
}

void BrightnessContrastDialog::showEvent(QShowEvent *e)
{
    syncFromViewer();
    QWidget::showEvent(e);
}

} // namespace medical
