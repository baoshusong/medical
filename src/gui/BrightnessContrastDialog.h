#pragma once

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <functional>

namespace medical {

class SrViewer;

// ImageJ-style Brightness/Contrast window: live window-center / window-width
// sliders driving the active SrViewer.
class BrightnessContrastDialog : public QWidget
{
    Q_OBJECT
public:
    explicit BrightnessContrastDialog(std::function<SrViewer *()> getViewer,
                                      QWidget *parent = nullptr);

    void syncFromViewer();

protected:
    void showEvent(QShowEvent *e) override;

private:
    std::function<SrViewer *()> m_getViewer;
    class QSlider *m_wc = nullptr;
    class QSlider *m_ww = nullptr;
    class QLabel *m_wcVal = nullptr;
    class QLabel *m_wwVal = nullptr;
};

} // namespace medical
