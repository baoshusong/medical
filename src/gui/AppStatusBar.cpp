#include "gui/AppStatusBar.h"
#include <QLabel>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QWidget>

namespace medical {

AppStatusBar::AppStatusBar(QObject *parent) : QObject(parent), m_bar(new QStatusBar)
{
    m_bar->setSizeGripEnabled(false);

    // Left section (permanent widgets)
    m_pacs = makeLabel(QStringLiteral("statusPacs"), false);
    m_engine = makeLabel(QStringLiteral("statusEngine"), false);
    m_infer = makeLabel(QStringLiteral("statusInfer"), false);
    m_panel = makeLabel(QStringLiteral("statusPanel"), false);
    m_problems = makeLabel(QStringLiteral("statusProblems"), false);

    // ImageJ-style live cursor readout (x, y, value) on the far left
    m_coord = new QLabel(m_bar);
    m_coord->setObjectName(QStringLiteral("statusCoord"));
    m_coord->setContentsMargins(0, 0, 0, 0);
    m_bar->addWidget(m_coord, 0);

    // Right section (permanent widgets)
    m_db = makeLabel(QStringLiteral("statusDb"), true);
    m_sliceInfo = makeLabel(QStringLiteral("statusSliceInfo"), true);
    m_study = makeLabel(QStringLiteral("statusStudy"), true);
    m_frame = makeLabel(QStringLiteral("statusFrame"), true);

    setPacs(QString());
    setEngine(QString(), QString());
    setInfer(QString());
    setDb(QString());
    setStudy(QString());
    setFrame(0, 0);
    setActivePanel(QString());
    setSliceInfo(QString());
    setProblemCount(0);
}

QLabel *AppStatusBar::makeLabel(const QString &objectName, bool permanent)
{
    auto *label = new QLabel(m_bar);
    label->setObjectName(objectName);
    label->setContentsMargins(0, 0, 0, 0);
    if (permanent)
        m_bar->addPermanentWidget(label);
    else
        m_bar->addWidget(label, 1);
    return label;
}

void AppStatusBar::setPacs(const QString &s)
{
    m_pacs->setText(s.isEmpty() ? QStringLiteral("PACS: --") : QStringLiteral("PACS: %1").arg(s));
}

void AppStatusBar::setEngine(const QString &name, const QString &device)
{
    // 状态栏只显示计算设备型号 (CPU/GPU), 不显示模型/引擎名称
    Q_UNUSED(name);
    m_engineName = name;
    m_engineDevice = device;
    if (device.isEmpty())
        m_engine->setText(QStringLiteral("引擎: --"));
    else
        m_engine->setText(QStringLiteral("引擎: %1").arg(device));
}

void AppStatusBar::setInfer(const QString &s)
{
    m_infer->setText(s.isEmpty() ? QStringLiteral("就绪") : s);
}

void AppStatusBar::setDb(const QString &s)
{
    m_db->setText(s.isEmpty() ? QStringLiteral("DB: --") : s);
}

void AppStatusBar::setStudy(const QString &s)
{
    m_study->setText(s.isEmpty() ? QStringLiteral("无检查") : s);
}

void AppStatusBar::setFrame(int idx, int total)
{
    if (total <= 0)
        m_frame->setText(QStringLiteral("帧: --"));
    else
        m_frame->setText(QStringLiteral("帧: %1/%2").arg(idx + 1).arg(total));
}

void AppStatusBar::setActivePanel(const QString &name)
{
    m_panel->setText(name.isEmpty() ? QString() : name);
    m_panel->setVisible(!name.isEmpty());
}

void AppStatusBar::setSliceInfo(const QString &info)
{
    m_sliceInfo->setText(info.isEmpty() ? QStringLiteral("轴位") : info);
}

void AppStatusBar::setProblemCount(int count)
{
    if (count <= 0)
        m_problems->setVisible(false);
    else {
        m_problems->setText(QStringLiteral("⚠ %1").arg(count));
        m_problems->setVisible(true);
    }
}

void AppStatusBar::setCoordValue(const QString &readout)
{
    m_coord->setText(readout.isEmpty() ? QStringLiteral("x=--  y=--  值=--") : readout);
}

} // namespace medical