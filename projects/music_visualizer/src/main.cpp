#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "AudioAnalyzer.h"
#include "LyricsParser.h"
#include "MusicLibrary.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("Music Visualizer");

    qmlRegisterType<AudioAnalyzer>("MusicVisualizer", 1, 0, "AudioAnalyzer");
    qmlRegisterType<LyricsParser> ("MusicVisualizer", 1, 0, "LyricsParser");
    qmlRegisterType<MusicLibrary> ("MusicVisualizer", 1, 0, "MusicLibrary");

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []{ QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("MusicVisualizer", "Main");
    return app.exec();
}
