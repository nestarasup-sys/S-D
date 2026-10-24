#include "editor_controller.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName("StudioDarkPrototype");
    app.setApplicationName("Studio Dark");
    QQuickStyle::setStyle("Basic");
    const auto args = app.arguments();
    int index = args.indexOf("--smoke");
    const auto smokeDirectory =
        index >= 0 && index + 1 < args.size() ? QDir(args[index + 1]).absolutePath() : QString{};
    EditorController controller(nullptr, smokeDirectory.isEmpty() ? QString{} : smokeDirectory + "/recovery");
    int projectArgument = args.indexOf("--project");
    if (projectArgument >= 0 && projectArgument + 1 < args.size())
        controller.open(QUrl::fromLocalFile(QFileInfo(args[projectArgument + 1]).absoluteFilePath()));
    int frameArgument = args.indexOf("--frame");
    if (frameArgument >= 0 && frameArgument + 1 < args.size())
        controller.setFrame(args[frameArgument + 1].toInt());
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("editor", &controller);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
        Qt::QueuedConnection);
    engine.loadFromModule("Studio.Dark", "Main");
    if (args.contains("--play-smoke"))
        QTimer::singleShot(100, &controller, &EditorController::togglePlayback);
    // Deterministic smoke mode: exercises QML creation and captures the actual window.
    if (!smokeDirectory.isEmpty()) {
        const auto directory = smokeDirectory;
        QDir().mkpath(directory);
        QTimer::singleShot(1500, &app, [&engine, &controller, directory] {
            auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
            bool ok = window && window->grabWindow().save(directory + "/desktop.png");
            QFile project(directory + "/demo.studio.json");
            ok = ok && project.open(QIODevice::WriteOnly);
            if (ok)
                project.write(studio::serialize(controller.project()));
            QFile runtime(directory + "/runtime.json");
            if (runtime.open(QIODevice::WriteOnly))
                runtime.write(QJsonDocument(QJsonObject{{"frame", controller.frame()},
                                                        {"playing", controller.playing()},
                                                        {"hasAudio", controller.hasAudio()}})
                                  .toJson());
            QCoreApplication::exit(ok ? 0 : 2);
        });
    }
    return app.exec();
}
