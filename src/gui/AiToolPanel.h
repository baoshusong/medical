#pragma once

#include <QWidget>

class QButtonGroup;
class QRadioButton;
class QSlider;
class QComboBox;
class QPushButton;
class QLabel;

namespace medical {

// 左侧 AI 标注工具面板 (对应 1.png 左栏)。
class AiToolPanel : public QWidget
{
    Q_OBJECT
public:
    enum Tool { AutoSegment, NoduleDetect, Measure, ArrowAnnotate };

    explicit AiToolPanel(QWidget *parent = nullptr);

    QString currentTask() const;
    QString currentModel() const;
    int     threshold() const;
    Tool     currentTool() const { return m_tool; }

signals:
    void toolChanged(int tool);
    void thresholdChanged(int value);
    void runAiRequested();

private slots:
    void onToolSelected(int id);

private:
    QButtonGroup *m_tools = nullptr;
    QRadioButton *m_seg = nullptr, *m_nod = nullptr, *m_meas = nullptr, *m_arr = nullptr;
    QSlider      *m_thresh = nullptr;
    QComboBox    *m_models = nullptr;
    QPushButton  *m_run = nullptr;
    QLabel       *m_threshVal = nullptr;
    Tool          m_tool = NoduleDetect;
};

} // namespace medical
