#include "app/MainWindow.h"
#include "gui/UiStyle.h"
#include "sr/BaseReconstructor.h"
#include "utils/Logger.h"

#include <QApplication>
#include <QIcon>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication::setOrganizationName(QStringLiteral("AiMedical"));
    QApplication::setApplicationName(QStringLiteral("AiMedicalWorkstation"));
    QApplication::setApplicationDisplayName(QStringLiteral("AI 医学影像工作站"));

    QApplication app(argc, argv);

    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/ai_medical_icon.ico")));

    medical::ui::applyWorkstationTheme(app);

    qRegisterMetaType<medical::SRStats>("medical::SRStats");

    medical::Logger::instance().setLevel(medical::Logger::Info);
    LOG_INFO("main", "startup");

    medical::MainWindow w;
    w.show();
    w.raise();
    w.activateWindow();
    return app.exec();
}
