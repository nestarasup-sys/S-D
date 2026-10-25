#include "editor_controller.hpp"
#include "thumbnail_provider.hpp"
#include "workspace_controller.hpp"
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
    WorkspaceController workspace(&controller,
                                  smokeDirectory.isEmpty() ? QString{} : smokeDirectory + "/workspace");
    int projectArgument = args.indexOf("--project");
    if (projectArgument >= 0 && projectArgument + 1 < args.size())
        controller.open(QUrl::fromLocalFile(QFileInfo(args[projectArgument + 1]).absoluteFilePath()));
    int frameArgument = args.indexOf("--frame");
    if (frameArgument >= 0 && frameArgument + 1 < args.size())
        controller.setFrame(args[frameArgument + 1].toInt());
    int selectionArgument = args.indexOf("--select");
    if (selectionArgument >= 0 && selectionArgument + 1 < args.size())
        controller.selectNode(args[selectionArgument + 1]);
    if (args.contains("--extract-video-audio-smoke"))
        controller.addAudioFromSelectedVideo();
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("editor", &controller);
    engine.rootContext()->setContextProperty("workspace", &workspace);
    engine.addImageProvider("studio", new ThumbnailProvider);
    int pageArgument = args.indexOf("--page");
    if (pageArgument >= 0 && pageArgument + 1 < args.size())
        workspace.navigate(args[pageArgument + 1]);
    else if (!smokeDirectory.isEmpty())
        workspace.navigate("editor");
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
        QTimer::singleShot(1500, &app, [&engine, &controller, &workspace, directory] {
            auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
            bool ok = window && window->grabWindow().save(directory + "/desktop.png");
            auto rootObject = engine.rootObjects().value(0);
            auto captionList = rootObject ? rootObject->findChild<QObject *>("captionWordAlignmentList") : nullptr;
            auto timelinePanel = rootObject ? rootObject->findChild<QObject *>("timelinePanelRoot") : nullptr;
            auto videoAudioButton = rootObject ? rootObject->findChild<QObject *>("addVideoAudioButton") : nullptr;
            auto canvasPicker = rootObject ? rootObject->findChild<QObject *>("newProjectCanvasPicker") : nullptr;
            const auto audioTracks = controller.audioTracks();
            QFile project(directory + "/demo.studio.json");
            ok = ok && project.open(QIODevice::WriteOnly);
            if (ok)
                project.write(studio::serialize(controller.project()));
            QFile runtime(directory + "/runtime.json");
            if (runtime.open(QIODevice::WriteOnly))
                runtime.write(QJsonDocument(QJsonObject{{"frame", controller.frame()},
                                                        {"playing", controller.playing()},
                                                        {"hasAudio", controller.hasAudio()},
                                                        {"page", workspace.page()},
                                                        {"selectedId", controller.selectedId()},
                                                        {"selectedCaptionWordCount",
                                                         controller.selected()["captionWords"].toList().size()},
                                                        {"captionTimingControlsVisible",
                                                         captionList && captionList->property("visible").toBool()},
                                                        {"captionTimingControlCount",
                                                         captionList ? captionList->property("count").toInt() : -1},
                                                        {"selectedCaptionTrackAvailable",
                                                         timelinePanel && timelinePanel->property("selectedCaptionTrackAvailable").toBool()},
                                                        {"selectedTrackIndex",
                                                         timelinePanel ? timelinePanel->property("selectedTrackIndex").toInt() : -1},
                                                        {"videoAudioActionVisible",
                                                         videoAudioButton && videoAudioButton->property("visible").toBool()},
                                                        {"newProjectCanvasOptions",
                                                         canvasPicker ? canvasPicker->property("count").toInt() : -1},
                                                        {"audioTrackCount", audioTracks.size()},
                                                        {"projects", workspace.projectCount()},
                                                        {"assets", workspace.assetCount()},
                                                        {"categories", workspace.categories().size()}})
                                  .toJson());
            QCoreApplication::exit(ok ? 0 : 2);
        });
    }
    return app.exec();
}
