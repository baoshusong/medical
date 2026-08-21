#pragma once

#include <QObject>
#include <QString>

class QStatusBar;
class QLabel;

namespace medical {

// VS Code-style bottom status bar with left (study info, panel, problems) and
// right (frame position, slice plane, engine) sections.
class AppStatusBar : public QObject
{
    Q_OBJECT
public:
    explicit AppStatusBar(QObject *parent = nullptr);

    QStatusBar *statusBar() const { return m_bar; }

    void setPacs(const QString &s);
    void setEngine(const QString &name, const QString &device);
    void setInfer(const QString &s);
    void setDb(const QString &s);
    void setStudy(const QString &s);
    void setFrame(int idx, int total);
    void setActivePanel(const QString &name);
    void setSliceInfo(const QString &info);
    void setProblemCount(int count);
    void setCoordValue(const QString &readout);   // ImageJ-style "x=.. y=.. 值=.."

private:
    QLabel *makeLabel(const QString &objectName, bool permanent);

    QStatusBar *m_bar = nullptr;
    QLabel *m_pacs = nullptr;
    QLabel *m_engine = nullptr;
    QLabel *m_infer = nullptr;
    QLabel *m_db = nullptr;
    QLabel *m_study = nullptr;
    QLabel *m_frame = nullptr;
    QLabel *m_panel = nullptr;
    QLabel *m_sliceInfo = nullptr;
    QLabel *m_problems = nullptr;
    QLabel *m_coord = nullptr;
    QString m_engineName, m_engineDevice;
};

} // namespace medical