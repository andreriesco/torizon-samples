#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <iostream>
#include "HardwareAccelerationService.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Create hardware acceleration service
    HardwareAccelerationService hardwareService;

    // Print detailed hardware acceleration log
    std::cout << "\n=== C++ QML Hardware Acceleration Detailed Log ===" << std::endl;
    std::cout << "Project: cppQMLHardAccTest" << std::endl;
    std::cout << "Framework: Qt6 + QML + C++" << std::endl;
    std::cout << "Hardware Accelerated: " << (hardwareService.isHardwareAccelerated() ? "Yes" : "No") << std::endl;
    std::cout << "\n" << hardwareService.detailedReport().toStdString() << std::endl;
    std::cout << "=== End of Detailed Log ===" << std::endl << std::endl;

    QQmlApplicationEngine engine;
    
    // Register the service with QML context
    engine.rootContext()->setContextProperty("hardwareService", &hardwareService);
    
    const QUrl url("qrc:/cppQMLHATest/QML/main.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    std::cout << "Hardware Acceleration Test - Qt/QML with Torizon!" << std::endl;

    return app.exec();
}
