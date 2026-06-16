#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "AudioAnalyzer.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("Music Visualizer");
    app.setApplicationDisplayName("Music Visualizer");

    qmlRegisterType<AudioAnalyzer>("MusicVisualizer", 1, 0, "AudioAnalyzer");

    QQmlApplicationEngine engine;

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app,    [](){ QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("MusicVisualizer", "Main");

    return app.exec();
}
