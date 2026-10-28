#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <studio/composition/painter.hpp>
#include <studio/core/project.hpp>
#include <studio/media/wave.hpp>
#include <studio/production/compiler.hpp>
#include <studio/storage/project_store.hpp>

namespace {
void require(bool condition, const QString &message) {
    if (!condition)
        throw std::runtime_error(message.toStdString());
}

std::size_t changedPixels(const QImage &a, const QImage &b, QRect region) {
    require(a.size() == b.size(), "image sizes must match");
    region = region.intersected(a.rect());
    std::size_t changed = 0;
    for (int y = region.top(); y <= region.bottom(); ++y)
        for (int x = region.left(); x <= region.right(); ++x)
            if (a.pixel(x, y) != b.pixel(x, y))
                ++changed;
    return changed;
}

studio::PcmAudio speechForFrames(int frames) {
    studio::PcmAudio audio;
    audio.samples.resize(std::size_t(frames) * (studio::ProductionSampleRate / 30), .025f);
    return audio;
}

void writeStill(studio::CompositionPainter &painter, const studio::Project &project, std::int64_t frame,
                const QString &path) {
    require(painter.frame(project, frame).save(path), "save frame evidence");
}

const studio::VisualBeat &beat(const studio::Project &project, const std::string &id) {
    const auto found = std::find_if(project.visualBeats.begin(), project.visualBeats.end(),
                                    [&](const auto &item) { return item.id == id; });
    require(found != project.visualBeats.end(), QStringLiteral("compiled beat exists: ") + QString::fromStdString(id));
    return *found;
}
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    try {
        const QString sourceRoot = QStringLiteral(STUDIO_SOURCE_DIR);
        const QString outputRoot = sourceRoot + "/artifacts/visual-beats-regression-" +
                                   QDateTime::currentDateTime().toString("yyyyMMdd-HHmmsszzz");
        require(QDir().mkpath(outputRoot + "/sequence"), "create ignored evidence folder");

        QFile revealFile(sourceRoot + "/examples/visual-beats-sequential-reveal-v3.production.json");
        require(revealFile.open(QIODevice::ReadOnly), "sequential reveal example exists");
        const auto revealBytes = revealFile.readAll();
        const auto revealRecipe = studio::parseRecipe(revealBytes, sourceRoot + "/examples");
        require(revealRecipe.error.isEmpty(), "sequential reveal example parses: " + revealRecipe.error);
        QMap<QString, studio::PcmAudio> revealAudio;
        revealAudio.insert("revelacoes-u1", speechForFrames(150));
        auto reveal = studio::compileProduction(revealRecipe.recipe, revealAudio,
                                                outputRoot + "/reveal-voice.wav");
        require(reveal.error.isEmpty(), "sequential reveal example compiles: " + reveal.error);
        require(reveal.project.nodes.size() == 3 && reveal.project.visualBeats.size() == 3,
                "example compiles exactly three targets and three beats");
        const auto &first = beat(reveal.project, "vb-reveal-1");
        const auto &second = beat(reveal.project, "vb-reveal-2");
        const auto &third = beat(reveal.project, "vb-reveal-3");
        require(first.frame < second.frame && second.frame < third.frame,
                "cueProgress beats resolve to increasing measured-audio frames");
        require(first.frame == 30 && second.frame == 69 && third.frame == 109,
                "20%, 46%, and 73% resolve against the 150-frame measured cue");
        for (const auto *nodeId : {"vb-first", "vb-second", "vb-third"}) {
            const auto *node = studio::findNode(reveal.project, nodeId);
            require(node && node->opacity == 0, QStringLiteral("target starts with opacity zero: ") + nodeId);
        }
        const auto *firstNode = studio::findNode(reveal.project, "vb-first");
        const auto *secondNode = studio::findNode(reveal.project, "vb-second");
        const auto *thirdNode = studio::findNode(reveal.project, "vb-third");
        require(studio::evaluate(*firstNode, first.frame - 1).opacity == 0 &&
                    studio::evaluate(*firstNode, first.frame).opacity == 1 &&
                    studio::evaluate(*firstNode, first.frame + 1).opacity == 1,
                "first show is hidden at T-1, visible at T, and persistent at T+1");
        require(studio::evaluate(*secondNode, second.frame - 1).opacity == 0 &&
                    studio::evaluate(*secondNode, second.frame).opacity == 1 &&
                    studio::evaluate(*thirdNode, third.frame - 1).opacity == 0 &&
                    studio::evaluate(*thirdNode, third.frame).opacity == 1,
                "second and third shows occur exactly at their event frames");
        require(studio::evaluate(*firstNode, second.frame).opacity == 1 &&
                    studio::evaluate(*secondNode, third.frame).opacity == 1,
                "earlier reveals remain visible as later objects appear");

        studio::CompositionPainter revealPainter;
        const auto base = revealPainter.frame(reveal.project, first.frame - 1);
        const auto firstVisible = revealPainter.frame(reveal.project, first.frame);
        const auto secondVisible = revealPainter.frame(reveal.project, second.frame);
        const auto thirdVisible = revealPainter.frame(reveal.project, third.frame);
        const QRect boxes[]{{100, 220, 220, 190}, {530, 220, 220, 190}, {960, 220, 220, 190}};
        require(changedPixels(base, firstVisible, boxes[0]) > 0, "QPainter draws the chip at its beat");
        require(changedPixels(base, firstVisible, boxes[1]) == 0 &&
                    changedPixels(base, firstVisible, boxes[2]) == 0,
                "network and brain remain absent after the first beat");
        require(changedPixels(base, secondVisible, boxes[1]) > 0 &&
                    changedPixels(base, secondVisible, boxes[2]) == 0,
                "QPainter reveals only the network at the second beat");
        require(changedPixels(base, thirdVisible, boxes[2]) > 0,
                "QPainter draws the brain at the third beat");
        for (const auto [frame, name] : {
                 std::pair{first.frame - 1, QString("01-before-first.png")},
                 {first.frame, QString("02-first.png")},
                 {first.frame + 1, QString("03-first-plus-one.png")},
                 {second.frame - 1, QString("04-before-second.png")},
                 {second.frame, QString("05-second.png")},
                 {third.frame - 1, QString("06-before-third.png")},
                 {third.frame, QString("07-third.png")}})
            writeStill(revealPainter, reveal.project, frame, outputRoot + "/sequence/" + name);

        require(studio::writeWave(outputRoot + "/reveal-voice.wav", reveal.mix).isEmpty(),
                "write the example's measured PCM track for export validation");
        const auto projectPath = outputRoot + "/visual-beats-reveal.studio.json";
        const auto saved = studio::saveProject(projectPath, reveal.project, {});
        require(saved.error.isEmpty(), "save compiled reveal project: " + saved.error);
        const auto reopened = studio::loadProject(projectPath);
        require(reopened.error.isEmpty() && reopened.project.visualBeats.size() == 3,
                "reopen keeps all Visual Beats");
        studio::CompositionPainter reopenedPainter;
        require(reopenedPainter.frame(reopened.project, first.frame) == firstVisible &&
                    reopenedPainter.frame(reopened.project, third.frame) == thirdVisible,
                "save/reopen preserves the beat-rendered preview frames");

        QFile motionFile(sourceRoot + "/examples/visual-beats-transform-camera-v3.production.json");
        require(motionFile.open(QIODevice::ReadOnly), "transform and camera example exists");
        const auto motionRecipe = studio::parseRecipe(motionFile.readAll(), sourceRoot + "/examples");
        require(motionRecipe.error.isEmpty(), "transform and camera example parses: " + motionRecipe.error);
        QMap<QString, studio::PcmAudio> motionAudio;
        motionAudio.insert("enquadramento-u1", speechForFrames(150));
        motionAudio.insert("enquadramento-u2", speechForFrames(150));
        auto motion = studio::compileProduction(motionRecipe.recipe, motionAudio,
                                                outputRoot + "/motion-voice.wav");
        require(motion.error.isEmpty(), "transform and camera example compiles: " + motion.error);
        require(std::any_of(motion.project.audioTracks.begin(), motion.project.audioTracks.end(),
                            [](const auto &track) { return track.name == "Efeito: chime-no-detalhe"; }),
                "sound beat compiles as an editable audio track");
        const auto &transform = beat(motion.project, "mover-alvo");
        const auto &camera = beat(motion.project, "aproximar-camera");
        const auto *target = studio::findNode(motion.project, "alvo");
        require(target && target->parentId == "grupo-alvo", "transform target inside a group is preserved");
        const auto transformAt = studio::evaluate(*target, transform.frame);
        const auto transformMiddle = studio::evaluate(*target, transform.frame + transform.durationFrames / 2);
        const auto transformEnd = studio::evaluate(*target, transform.frame + transform.durationFrames);
        require(transformMiddle.x > transformAt.x && transformMiddle.x < transformEnd.x &&
                    transformMiddle.scale > transformAt.scale && transformMiddle.scale < transformEnd.scale,
                "transform beat interpolates during durationFrames on a grouped child");
        require(std::abs(studio::evaluateCamera(motion.project, camera.frame - 1).zoom - 1) < 1e-8 &&
                    studio::evaluateCamera(motion.project, camera.frame + camera.durationFrames).zoom > 1.14,
                "camera beat holds before T and reaches its target after its duration");
        const auto &hide = beat(motion.project, "ocultar-titulo");
        const auto &show = beat(motion.project, "revelar-titulo");
        const auto &customOpacity = beat(motion.project, "opacidade-final-do-titulo");
        const auto *title = studio::findNode(motion.project, "titulo-final");
        require(title && std::abs(studio::evaluate(*title, customOpacity.frame + customOpacity.durationFrames).opacity - .64) < 1e-8 &&
                    std::abs(studio::evaluate(*title, hide.frame - 1).opacity - .64) < 1e-8 &&
                    studio::evaluate(*title, hide.frame).opacity == 0 &&
                    studio::evaluate(*title, show.frame - 1).opacity == 0 &&
                    std::abs(studio::evaluate(*title, show.frame).opacity - .64) < 1e-8,
                "transform sets custom opacity and hide/show restores it at the exact event frame");
        const auto &semiHide = beat(motion.project, "ocultar-brilho");
        const auto &semiShow = beat(motion.project, "restaurar-brilho");
        const auto *semi = studio::findNode(motion.project, "brilho-semitransparente");
        const auto semiBeforeHide = studio::evaluate(*semi, semiHide.frame - 1).opacity;
        const auto semiAtHide = studio::evaluate(*semi, semiHide.frame).opacity;
        const auto semiBeforeShow = studio::evaluate(*semi, semiShow.frame - 1).opacity;
        const auto semiAtShow = studio::evaluate(*semi, semiShow.frame).opacity;
        require(semi && semiBeforeHide > 0 && semiBeforeHide <= .42 && semiAtHide == 0 &&
                    semiBeforeShow == 0 && std::abs(semiAtShow - .42) < 1e-8,
                QString("hide/show opacity: %1, %2, %3, %4 at %5/%6")
                    .arg(semiBeforeHide).arg(semiAtHide).arg(semiBeforeShow).arg(semiAtShow)
                    .arg(semiHide.frame).arg(semiShow.frame));
        const auto *sameFrame = studio::findNode(motion.project, "revelacao-ordem-estavel");
        const auto &sameFrameHide = beat(motion.project, "mesmo-quadro-ocultar");
        const auto &sameFrameShow = beat(motion.project, "mesmo-quadro-revelar");
        require(sameFrame && sameFrameHide.frame == sameFrameShow.frame &&
                    studio::evaluate(*sameFrame, sameFrameHide.frame - 1).opacity == 0 &&
                    studio::evaluate(*sameFrame, sameFrameHide.frame).opacity == 1,
                "same-frame beats resolve deterministically in declared hide-then-show order");
        require(studio::validate(motion.project).empty(), "all action-type example output validates");

        QFile report(outputRoot + "/report.json");
        require(report.open(QIODevice::WriteOnly | QIODevice::Truncate), "write regression report");
        report.write(QJsonDocument(QJsonObject{
            {"status", "passed"}, {"revealFrames", QJsonArray{qint64(first.frame), qint64(second.frame), qint64(third.frame)}},
            {"imageSequence", outputRoot + "/sequence"}, {"savedProject", projectPath},
            {"recipeExamples", QJsonArray{"visual-beats-sequential-reveal-v3.production.json",
                                          "visual-beats-transform-camera-v3.production.json"}},
            {"verifiedActions", QJsonArray{"show", "hide", "transform", "camera", "sound"}},
            {"qPainterImagesPassed", true}, {"saveReopenPassed", true}}).toJson(QJsonDocument::Indented));
        report.close();
        std::cout << "Visual Beat timing/render regression passed. Evidence: " << outputRoot.toStdString() << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
