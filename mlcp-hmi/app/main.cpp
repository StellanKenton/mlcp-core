#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>

#include "bridge/fanSettingsViewModel.h"
#include "bridge/lifecycleViewModel.h"
#include "system/appcontrol/appcontrol.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    mlcp::hmi::system::AppControl appControl;
    appControl.startup();
    mlcp::hmi::bridge::FanSettingsViewModel fanSettingsViewModel(appControl);
    mlcp::hmi::bridge::LifecycleViewModel lifecycleViewModel(appControl);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("fanSettingsViewModel", &fanSettingsViewModel);
    engine.rootContext()->setContextProperty("lifecycleViewModel", &lifecycleViewModel);
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    const int result = app.exec();
    appControl.shutdown();

    return result;
}
