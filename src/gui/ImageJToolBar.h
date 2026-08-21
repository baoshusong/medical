#pragma once

#include <QWidget>
#include <QColor>

class QToolButton;
class QButtonGroup;

namespace medical {

// ImageJ-style horizontal annotation toolbar.
//  Left   : drawing & analysis tools (select / rect / ellipse / line /
//           angle / arrow / hand / picker) + zoom + text
//  Right  : ImageJ signature foreground/background color chips.
class ImageJToolBar : public QWidget
{
    Q_OBJECT
public:
    enum ToolId { ToolNone = -1, ToolDistance = 0, ToolAngle = 1, ToolArrow = 2, ToolBox = 3, ToolEllipse = 4, ToolText = 5 };

    explicit ImageJToolBar(QWidget *parent = nullptr);

    void setActiveTool(int tool);

signals:
    void toolRequested(int tool);
    void zoomInRequested();
    void zoomOutRequested();

private:
    QToolButton *makeToolBtn(const QString &icon, const QString &tip, bool checkable);
    QWidget *makeSep();

    void swapColors();
    void updateChips();

    QButtonGroup *m_toolGroup = nullptr;
    QToolButton *m_fgChip = nullptr;
    QToolButton *m_bgChip = nullptr;
    QColor m_fgColor = QColor(QStringLiteral("#ffe678"));
    QColor m_bgColor = QColor(QStringLiteral("#1a1a1a"));
};

} // namespace medical
