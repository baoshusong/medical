#include "gui/AiToolPanel.h"
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QRadioButton>
#include <QSlider>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

namespace medical {

AiToolPanel::AiToolPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(10);

    auto *title = new QLabel(QStringLiteral("AI 标注工具"));
    title->setStyleSheet(QStringLiteral("font-weight:600; padding:2px;"));
    root->addWidget(title);

    auto *g = new QGroupBox(QStringLiteral("工具"));
    auto *gl = new QVBoxLayout(g);
    m_seg   = new QRadioButton(QStringLiteral("自动分割"));
    m_nod   = new QRadioButton(QStringLiteral("结节检测"));
    m_meas  = new QRadioButton(QStringLiteral("测量"));
    m_arr   = new QRadioButton(QStringLiteral("箭头标注"));
    m_nod->setChecked(true);
    gl->addWidget(m_seg); gl->addWidget(m_nod); gl->addWidget(m_meas); gl->addWidget(m_arr);

    m_tools = new QButtonGroup(this);
    m_tools->addButton(m_seg, AutoSegment);
    m_tools->addButton(m_nod, NoduleDetect);
    m_tools->addButton(m_meas, Measure);
    m_tools->addButton(m_arr, ArrowAnnotate);
    connect(m_tools, &QButtonGroup::idClicked, this, &AiToolPanel::onToolSelected);
    root->addWidget(g);

    auto *param = new QGroupBox(QStringLiteral("参数"));
    auto *pl = new QVBoxLayout(param);
    auto *tl = new QLabel(QStringLiteral("分割阈值"));
    m_thresh = new QSlider(Qt::Horizontal);
    m_thresh->setRange(0, 100); m_thresh->setValue(50);
    m_threshVal = new QLabel(QStringLiteral("50"));
    auto *thr = new QHBoxLayout;
    thr->addWidget(m_thresh); thr->addWidget(m_threshVal);
    connect(m_thresh, &QSlider::valueChanged, this, [this](int v){
        m_threshVal->setNum(v);
        emit thresholdChanged(v);
    });
    pl->addWidget(tl); pl->addLayout(thr);

    auto *ml = new QLabel(QStringLiteral("模型"));
    m_models = new QComboBox;
    m_models->addItem(QStringLiteral("LungNet-CT-v3"));
    m_models->addItem(QStringLiteral("Lung-XRay-v2"));
    m_models->addItem(QStringLiteral("Nodule-Seg-Net"));
    pl->addWidget(ml); pl->addWidget(m_models);
    root->addWidget(param);

    m_run = new QPushButton(QStringLiteral("▶ 运行 AI"));
    m_run->setStyleSheet(QStringLiteral("padding:6px; font-weight:600;"));
    connect(m_run, &QPushButton::clicked, this, &AiToolPanel::runAiRequested);
    root->addWidget(m_run);

    root->addStretch(1);
}

void AiToolPanel::onToolSelected(int id)
{
    m_tool = static_cast<Tool>(id);
    emit toolChanged(static_cast<int>(m_tool));
}

QString AiToolPanel::currentTask() const
{
    switch (m_tool) {
    case AutoSegment:    return QStringLiteral("segmentation");
    case NoduleDetect:   return QStringLiteral("detection");
    case Measure:        return QStringLiteral("measure");
    case ArrowAnnotate:  return QStringLiteral("annotate");
    }
    return QStringLiteral("detection");
}

QString AiToolPanel::currentModel() const
{
    return m_models->currentText();
}

int AiToolPanel::threshold() const { return m_thresh->value(); }

} // namespace medical
