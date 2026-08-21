#include "gui/ImageJToolBar.h"

#include <QToolButton>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QFrame>
#include <QAbstractButton>
#include <QIcon>
#include <QList>
#include <QSizePolicy>

namespace medical {

ImageJToolBar::ImageJToolBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("ijToolBar"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto *main = new QHBoxLayout(this);
    main->setContentsMargins(6, 3, 6, 3);
    main->setSpacing(3);

    m_toolGroup = new QButtonGroup(this);
    m_toolGroup->setExclusive(true);

    // ── ROI / analysis tools ──
    struct Tool { QString icon, tip; int id; };
    const QList<Tool> tools = {
        {QStringLiteral(":/icons/cmd_select.svg"),  QStringLiteral("选择 / 平移 / 调窗"), ToolNone},
        {QStringLiteral(":/icons/cmd_box.svg"),     QStringLiteral("矩形 ROI"), ToolBox},
        {QStringLiteral(":/icons/cmd_ellipse.svg"), QStringLiteral("椭圆 ROI"), ToolEllipse},
        {QStringLiteral(":/icons/cmd_ruler.svg"),   QStringLiteral("直线 / 距离"), ToolDistance},
        {QStringLiteral(":/icons/cmd_angle.svg"),   QStringLiteral("角度"), ToolAngle},
        {QStringLiteral(":/icons/cmd_arrow.svg"),   QStringLiteral("箭头"), ToolArrow},
        {QStringLiteral(":/icons/cmd_hand.svg"),    QStringLiteral("抓手 / 平移"), ToolNone},
        {QStringLiteral(":/icons/cmd_picker.svg"),  QStringLiteral("吸管 / 取像素值"), ToolNone},
        {QStringLiteral(":/icons/cmd_text.svg"),    QStringLiteral("文字标注"), ToolText},
    };
    for (const auto &t : tools) {
        QToolButton *b = makeToolBtn(t.icon, t.tip, true);
        m_toolGroup->addButton(b, t.id);
        connect(b, &QToolButton::clicked, this, [this, id = t.id] { emit toolRequested(id); });
        main->addWidget(b);
    }
    main->addWidget(makeSep());

    // ── Zoom ──
    QToolButton *zin = makeToolBtn(QStringLiteral(":/icons/cmd_zoom.svg"), QStringLiteral("放大"), false);
    connect(zin, &QToolButton::clicked, this, &ImageJToolBar::zoomInRequested);
    QToolButton *zout = makeToolBtn(QStringLiteral(":/icons/cmd_zoom_out.svg"), QStringLiteral("缩小"), false);
    connect(zout, &QToolButton::clicked, this, &ImageJToolBar::zoomOutRequested);
    main->addWidget(zin);
    main->addWidget(zout);
    main->addWidget(makeSep());

    main->addStretch(1);

    // ── ImageJ foreground / background color chips ──
    QWidget *chipWrap = new QWidget;
    chipWrap->setObjectName(QStringLiteral("ijChipWrap"));
    auto *cl = new QHBoxLayout(chipWrap);
    cl->setContentsMargins(0, 2, 0, 2);
    cl->setSpacing(3);
    m_fgChip = new QToolButton;
    m_fgChip->setObjectName(QStringLiteral("ijFgChip"));
    m_fgChip->setFixedSize(15, 15);
    m_bgChip = new QToolButton;
    m_bgChip->setObjectName(QStringLiteral("ijBgChip"));
    m_bgChip->setFixedSize(15, 15);
    cl->addWidget(m_fgChip);
    cl->addWidget(m_bgChip);
    main->addWidget(chipWrap, 0, Qt::AlignVCenter);
    updateChips();
    connect(m_fgChip, &QToolButton::clicked, this, &ImageJToolBar::swapColors);
    connect(m_bgChip, &QToolButton::clicked, this, &ImageJToolBar::swapColors);

    // default tool = select
    if (!m_toolGroup->buttons().isEmpty())
        m_toolGroup->buttons().first()->setChecked(true);
}

QToolButton *ImageJToolBar::makeToolBtn(const QString &icon, const QString &tip, bool checkable)
{
    auto *b = new QToolButton(this);
    b->setObjectName(QStringLiteral("ijToolBtn"));
    b->setIcon(QIcon(icon));
    b->setIconSize(QSize(20, 20));
    b->setFixedSize(28, 28);
    b->setToolTip(tip);
    b->setCheckable(checkable);
    return b;
}

QWidget *ImageJToolBar::makeSep()
{
    auto *s = new QFrame;
    s->setObjectName(QStringLiteral("ijSep"));
    s->setFrameShape(QFrame::VLine);
    s->setFixedWidth(2);
    return s;
}

void ImageJToolBar::setActiveTool(int tool)
{
    QAbstractButton *b = m_toolGroup->button(tool);
    if (b) b->setChecked(true);
}

void ImageJToolBar::swapColors()
{
    qSwap(m_fgColor, m_bgColor);
    updateChips();
}

void ImageJToolBar::updateChips()
{
    m_fgChip->setStyleSheet(QStringLiteral("QToolButton#ijFgChip{background:%1;border:2px solid #888888;padding:0;}")
                                .arg(m_fgColor.name()));
    m_bgChip->setStyleSheet(QStringLiteral("QToolButton#ijBgChip{background:%1;border:1px solid #888888;padding:0;}")
                                .arg(m_bgColor.name()));
}

} // namespace medical
