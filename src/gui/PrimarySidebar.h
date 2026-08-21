#pragma once

#include <QWidget>
#include <QMap>

class QLabel;
class QStackedWidget;
class QToolButton;

namespace medical {

// VS Code-style primary sidebar that appears between the activity bar
// and the editor area. Each view registers its own content widget.
class PrimarySidebar : public QWidget
{
    Q_OBJECT
public:
    explicit PrimarySidebar(QWidget *parent = nullptr);

    // Register a sidebar content widget for a given view index.
    // The sidebar will display this widget when switchToView(viewIndex) is called.
    void registerView(int viewIndex, QWidget *content, const QString &title);

    // Switch to the sidebar content for the given view
    void switchToView(int viewIndex);

    // Show / hide the sidebar
    void setVisible(bool visible);
    void toggle();
    bool isSidebarVisible() const { return m_visible; }

signals:
    void visibilityChanged(bool visible);

private:
    QWidget *m_titleBar = nullptr;
    QLabel *m_titleLabel = nullptr;
    QToolButton *m_collapseBtn = nullptr;
    QStackedWidget *m_contentStack = nullptr;
    QMap<int, int> m_viewIndexToStackIndex;
    QMap<int, QString> m_viewTitles;
    bool m_visible = true;
    int m_savedWidth = 300;
};

} // namespace medical