#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
#include <studio/composition/painter.hpp>
#include <studio/core/editor.hpp>
#include <studio/storage/project_store.hpp>

static int checks = 0;
static void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    try {
        auto p = studio::makeDemo();
        studio::Editor editor(p);
        check(studio::validate(p).empty(), "demo must validate");
        auto baseline = studio::serialize(editor.project());
        check(!editor.execute([](auto &c) { c.nodes[0].start = -1; }).empty(), "reject negative start");
        check(studio::serialize(editor.project()) == baseline && !editor.canUndo(),
              "invalid command must not mutate history");
        check(!editor.execute([](auto &c) { c.nodes[1].id = c.nodes[0].id; }).empty(), "reject duplicate ID");
        check(!editor.execute([](auto &c) { c.nodes[0].opacity = 2; }).empty(), "reject opacity overflow");
        editor.beginGesture();
        for (int i = 0; i < 20; ++i)
            editor.execute([i](auto &c) { c.nodes[0].x = 200 + i; });
        editor.endGesture();
        check(editor.undo(), "gesture undo");
        check(studio::serialize(editor.project()) == baseline, "one undo restores entire drag");
        check(editor.redo() && editor.project().nodes[0].x == 219, "redo entire drag");
        editor.undo();
        editor.execute([](auto &c) { c.nodes[0].y = 90; });
        check(!editor.canRedo(), "new edit invalidates redo");
        auto clip = p.nodes[1];
        check(!studio::active(clip, clip.start - 1), "clip start exclusive outside");
        check(studio::active(clip, clip.start), "clip starts at first frame");
        check(!studio::active(clip, clip.start + clip.duration), "clip end exclusive");
        check(studio::evaluate(clip, clip.start).opacity == 0, "animated entrance first frame");
        check(studio::evaluate(clip, clip.start + 24).opacity == clip.opacity, "animated entrance settles");
        // Incoming keyframe controls the segment easing; older files remain linear.
        studio::Node animated;
        animated.id = "curve-test";
        animated.animated = false;
        animated.duration = 90;
        animated.keyframes = {{0, 0, 0, 0, 1, 1}, {60, 100, 0, 0, 1, 1, "ease-in"}};
        p.nodes.push_back(animated);
        p.camera = {{0, 640, 360, 1, 0}, {60, 740, 360, 2, 0, "ease-out"}};
        check(studio::supportedEasing("linear") && studio::supportedEasing("ease-in") &&
                  studio::supportedEasing("ease-out") && studio::supportedEasing("ease-in-out") &&
                  !studio::supportedEasing("bounce"),
              "all four named keyframe easing curves are supported");
        check(std::abs(studio::evaluate(animated, 30).x - 12.5) < .001,
              "incoming ease-in uses cubic interpolation");
        check(std::abs(studio::evaluateCamera(p, 30).x - 727.5) < .001,
              "camera ease-out uses cubic interpolation");
        check(studio::serialize(studio::deserialize(studio::serialize(p)).project) == studio::serialize(p),
              "eased keyframes survive schema 3 roundtrip");
        auto portraitProject = studio::makeEmpty();
        portraitProject.width = 1080;
        portraitProject.height = 1920;
        const auto portraitRoundTrip = studio::deserialize(studio::serialize(portraitProject));
        check(studio::validate(portraitProject).empty() && portraitRoundTrip.error.isEmpty() &&
                  portraitRoundTrip.project.width == 1080 && portraitRoundTrip.project.height == 1920,
              "vertical project dimensions validate and survive JSON roundtrip");
        auto badCurve = p;
        badCurve.camera[1].easing = "spring-unknown";
        check(!studio::validate(badCurve).empty(), "reject unsupported camera curve");
        badCurve = p;
        badCurve.nodes.back().keyframes[1].easing = "spring-unknown";
        check(!studio::validate(badCurve).empty(), "reject unsupported object curve");
        p.fps = {30000, 1001};
        check(studio::validate(p).empty(), "fractional fps supported");
        check(studio::serialize(studio::deserialize(studio::serialize(p)).project) == studio::serialize(p),
              "round trip rational fps and document");
        auto malformed = QJsonDocument::fromJson(studio::serialize(p)).object();
        malformed["schemaVersion"] = 15;
        check(!studio::deserialize(QJsonDocument(malformed).toJson()).error.isEmpty(),
              "reject future schema");
        auto shotProject = studio::makeEmpty();
        shotProject.schemaVersion = 4;
        shotProject.duration = 120;
        shotProject.scenes = {{"story", "História", "Uma fala", 0, 120}};
        shotProject.shots = {{"wide", "story", "Plano aberto", "cut", 0, 60},
                             {"detail", "story", "Detalhe", "cut", 60, 60}};
        studio::Node shotNode;
        shotNode.id = "price";
        shotNode.name = "Preço";
        shotNode.sceneId = "story";
        shotNode.shotId = "detail";
        shotNode.start = 60;
        shotNode.duration = 60;
        shotProject.nodes = {shotNode};
        shotProject.visualBeats = {{"reveal", "detail", "price", "show", 72, 0}};
        shotProject.audioSource = "C:/media/voice.wav";
        shotProject.voiceDuckingSource = "C:/media/voice-stem.wav";
        shotProject.audioTracks = {
            {"bed", "Trilha de teste", "C:/media/bed.wav", 0, 120, .45, false, true, .7}};
        check(studio::validate(shotProject).empty(), "valid independent shots and beat");
        auto persistedShots = studio::deserialize(studio::serialize(shotProject));
        check(persistedShots.error.isEmpty() && persistedShots.project.shots.size() == 2 &&
                  persistedShots.project.visualBeats.size() == 1 &&
                  persistedShots.project.nodes.front().shotId == "detail" &&
                  persistedShots.project.audioTracks.size() == 1 &&
                  persistedShots.project.audioTracks.front().duckUnderVoice &&
                  persistedShots.project.audioTracks.front().gain == .45 &&
                  persistedShots.project.voiceDuckingSource == "C:/media/voice-stem.wav",
              "shots, beats and independent audio tracks survive schema 4 roundtrip");
        auto styledShotProject = shotProject;
        styledShotProject.schemaVersion = 11;
        studio::ArtDirection sceneArt = styledShotProject.artDirection;
        sceneArt.backgroundColor = "#112233";
        styledShotProject.scenes.front().artDirection = sceneArt;
        studio::ArtDirection shotArt = sceneArt;
        shotArt.backgroundColor = "#334455";
        styledShotProject.shots.front().artDirection = shotArt;
        auto persistedScopedStyles = studio::deserialize(studio::serialize(styledShotProject));
        check(studio::validate(styledShotProject).empty() && persistedScopedStyles.error.isEmpty() &&
                  persistedScopedStyles.project.scenes.front().artDirection->backgroundColor == "#112233" &&
                  persistedScopedStyles.project.shots.front().artDirection->backgroundColor == "#334455",
              "scene and shot art direction survive schema 11 roundtrip");
        studio::CompositionPainter scopedStylePainter;
        check(scopedStylePainter.frame(styledShotProject, 10) !=
                  scopedStylePainter.frame(styledShotProject, 80),
              "shot style overrides the scene style in the composition");
        auto legacyScopedStyle = styledShotProject;
        legacyScopedStyle.schemaVersion = 10;
        check(!studio::validate(legacyScopedStyle).empty(), "scene and shot styles require schema 11");
        auto animatedCaptionProject = studio::makeEmpty();
        animatedCaptionProject.schemaVersion = 12;
        animatedCaptionProject.duration = 20;
        studio::Node animatedCaption;
        animatedCaption.id = "caption-word-motion";
        animatedCaption.name = "Legenda";
        animatedCaption.text = "Uma palavra.";
        animatedCaption.role = "caption";
        animatedCaption.duration = 20;
        animatedCaption.x = 80;
        animatedCaption.y = 610;
        animatedCaption.width = 1120;
        animatedCaption.height = 80;
        animatedCaption.fontSize = 27;
        animatedCaption.captionWords = {{"Uma", 0, 8}, {"palavra.", 8, 20}};
        animatedCaption.wordAnimationPreset = "pop-rise";
        animatedCaption.wordAnimationDurationFrames = 6;
        animatedCaption.wordAnimationStartScale = .72;
        animatedCaption.wordAnimationTravelY = 18;
        animatedCaptionProject.nodes.push_back(animatedCaption);
        const auto persistedWordAnimation = studio::deserialize(studio::serialize(animatedCaptionProject));
        check(studio::validate(animatedCaptionProject).empty() && persistedWordAnimation.error.isEmpty() &&
                  persistedWordAnimation.project.schemaVersion == 12 &&
                  persistedWordAnimation.project.nodes.front().wordAnimationPreset == "pop-rise" &&
                  persistedWordAnimation.project.nodes.front().wordAnimationDurationFrames == 6 &&
                  persistedWordAnimation.project.nodes.front().wordAnimationStartScale == .72 &&
                  persistedWordAnimation.project.nodes.front().wordAnimationTravelY == 18,
              "word-animation settings survive the schema 12 roundtrip");
        auto timedAudioProject = shotProject;
        timedAudioProject.schemaVersion = 13;
        timedAudioProject.audioTracks.front().sourceInSeconds = 1.25;
        timedAudioProject.audioTracks.front().playbackRate = 1.5;
        const auto persistedTimedAudio = studio::deserialize(studio::serialize(timedAudioProject));
        check(studio::validate(timedAudioProject).empty() && persistedTimedAudio.error.isEmpty() &&
                  persistedTimedAudio.project.schemaVersion == 13 &&
                  persistedTimedAudio.project.audioTracks.front().sourceInSeconds == 1.25 &&
                  persistedTimedAudio.project.audioTracks.front().playbackRate == 1.5,
              "audio source trim and speed survive the schema 13 roundtrip");
        timedAudioProject.schemaVersion = 12;
        check(!studio::validate(timedAudioProject).empty(), "audio source trim and speed require schema 13");
        auto independentHighlight = studio::makeEmpty();
        independentHighlight.schemaVersion = 14;
        independentHighlight.artDirection.primaryColor = "#22aa44";
        independentHighlight.artDirection.captionHighlightColor = "#ff0055";
        const auto loadedHighlight = studio::deserialize(studio::serialize(independentHighlight));
        check(studio::validate(independentHighlight).empty() && loadedHighlight.error.isEmpty() &&
                  loadedHighlight.project.artDirection.primaryColor == "#22aa44" &&
                  loadedHighlight.project.artDirection.captionHighlightColor == "#ff0055",
              "schema 14 preserves caption highlight independently of the primary art color");
        auto linkedAudioProject = studio::makeEmpty();
        linkedAudioProject.schemaVersion = 13;
        linkedAudioProject.duration = 120;
        studio::Node linkedVideo;
        linkedVideo.id = "linked-video";
        linkedVideo.kind = studio::NodeKind::Video;
        linkedVideo.name = "Vídeo com áudio";
        linkedVideo.source = "C:/media/clip.mp4";
        linkedVideo.start = 10;
        linkedVideo.duration = 60;
        linkedVideo.videoSourceInSeconds = .5;
        linkedVideo.videoPlaybackRate = 1.5;
        linkedAudioProject.nodes.push_back(linkedVideo);
        studio::AudioTrack linkedTrack;
        linkedTrack.id = "linked-audio";
        linkedTrack.name = "Áudio do vídeo";
        linkedTrack.source = linkedVideo.source;
        linkedTrack.start = linkedVideo.start;
        linkedTrack.duration = linkedVideo.duration;
        linkedTrack.sourceInSeconds = linkedVideo.videoSourceInSeconds;
        linkedTrack.playbackRate = linkedVideo.videoPlaybackRate;
        linkedTrack.linkedVideoNodeId = linkedVideo.id;
        linkedAudioProject.audioTracks.push_back(linkedTrack);
        studio::Editor linkedAudioEditor(linkedAudioProject);
        check(linkedAudioEditor
                      .execute([](auto &project) {
                          project.nodes.front().start = 20;
                          project.nodes.front().duration = 45;
                          project.nodes.front().videoSourceInSeconds = 1.25;
                          project.nodes.front().videoPlaybackRate = 2;
                      })
                      .empty() &&
                  linkedAudioEditor.project().audioTracks.front().start == 20 &&
                  linkedAudioEditor.project().audioTracks.front().duration == 45 &&
                  linkedAudioEditor.project().audioTracks.front().sourceInSeconds == 1.25 &&
                  linkedAudioEditor.project().audioTracks.front().playbackRate == 2,
              "linked video audio follows layer movement, duration, trim and speed in one edit");
        const auto persistedLinkedAudio = studio::deserialize(studio::serialize(linkedAudioEditor.project()));
        check(persistedLinkedAudio.error.isEmpty() &&
                  persistedLinkedAudio.project.audioTracks.front().linkedVideoNodeId == "linked-video",
              "video audio linkage survives schema 13 project roundtrip");
        check(linkedAudioEditor.execute([](auto &project) { project.nodes.clear(); }).empty() &&
                  linkedAudioEditor.project().audioTracks.front().linkedVideoNodeId.empty(),
              "removing a video detaches its audio track without deleting the audio");
        animatedCaptionProject.schemaVersion = 11;
        check(!studio::validate(animatedCaptionProject).empty(),
              "word-animation settings require schema 12 and retain older project compatibility");
        auto soundBeatProject = shotProject;
        soundBeatProject.schemaVersion = 9;
        soundBeatProject.visualBeats.push_back({"price-chime", "detail", "sfx-price-chime", "sound", 80, 0});
        soundBeatProject.audioTracks.push_back(
            {"sfx-price-chime", "Efeito: price-chime", "C:/media/chime.wav", 80, 40, .65});
        const auto persistedSoundBeat = studio::deserialize(studio::serialize(soundBeatProject));
        check(studio::validate(soundBeatProject).empty() && persistedSoundBeat.error.isEmpty() &&
                  persistedSoundBeat.project.schemaVersion == 9 &&
                  persistedSoundBeat.project.visualBeats.back().action == "sound" &&
                  persistedSoundBeat.project.visualBeats.back().targetId == "sfx-price-chime" &&
                  persistedSoundBeat.project.audioTracks.back().start == 80,
              "sound Visual Beats persist as linked audio tracks in schema 9");
        auto detachedSoundBeat = soundBeatProject;
        detachedSoundBeat.audioTracks.back().start = 79;
        check(!studio::validate(detachedSoundBeat).empty(),
              "sound Visual Beat and audio track share a frame");
        detachedSoundBeat = soundBeatProject;
        detachedSoundBeat.audioTracks.pop_back();
        check(!studio::validate(detachedSoundBeat).empty(), "sound Visual Beat requires its linked track");
        auto fadedProject = shotProject;
        fadedProject.schemaVersion = 6;
        fadedProject.audioTracks.front().fadeInFrames = 12;
        fadedProject.audioTracks.front().fadeOutFrames = 18;
        const auto persistedFades = studio::deserialize(studio::serialize(fadedProject));
        check(studio::validate(fadedProject).empty() && persistedFades.error.isEmpty() &&
                  persistedFades.project.audioTracks.front().fadeInFrames == 12 &&
                  persistedFades.project.audioTracks.front().fadeOutFrames == 18,
              "audio track fades survive schema 6 roundtrip");
        auto overlappingFades = fadedProject;
        overlappingFades.audioTracks.front().fadeOutFrames = 109;
        check(!studio::validate(overlappingFades).empty(), "audio fades cannot exceed the track duration");
        auto automatedProject = shotProject;
        automatedProject.schemaVersion = 8;
        automatedProject.audioTracks.front().gainAutomation = {
            {0, .2, "linear"}, {30, 1.0, "ease-in-out"}, {60, .4, "ease-out"}};
        check(studio::validate(automatedProject).empty() &&
                  std::abs(studio::evaluateGainAutomation(automatedProject.audioTracks.front(), 0) - .2) <
                      1e-9 &&
                  std::abs(studio::evaluateGainAutomation(automatedProject.audioTracks.front(), 15) - .6) <
                      1e-9 &&
                  std::abs(studio::evaluateGainAutomation(automatedProject.audioTracks.front(), 30) - 1.0) <
                      1e-9 &&
                  std::abs(studio::evaluateGainAutomation(automatedProject.audioTracks.front(), 90) - .4) <
                      1e-9,
              "audio gain automation evaluates linear segments and easing curves");
        const auto persistedAutomation = studio::deserialize(studio::serialize(automatedProject));
        check(persistedAutomation.error.isEmpty() && persistedAutomation.project.schemaVersion == 8 &&
                  persistedAutomation.project.audioTracks.front().gainAutomation ==
                      automatedProject.audioTracks.front().gainAutomation,
              "audio gain automation survives schema 8 save and load");
        auto unsortedAutomation = automatedProject;
        std::swap(unsortedAutomation.audioTracks.front().gainAutomation[0],
                  unsortedAutomation.audioTracks.front().gainAutomation[1]);
        check(!studio::validate(unsortedAutomation).empty(), "gain automation points must be ordered");
        auto noVoiceDucking = shotProject;
        noVoiceDucking.audioSource.clear();
        noVoiceDucking.voiceDuckingSource.clear();
        check(!studio::validate(noVoiceDucking).empty(), "ducking requires a voice sidechain source");
        auto brokenShots = shotProject;
        brokenShots.shots[1].start = 59;
        check(!studio::validate(brokenShots).empty(), "reject overlapping visual shots");
        check(!studio::deserialize("{broken").error.isEmpty(), "reject broken JSON");
        QTemporaryDir directory;
        auto path = directory.filePath("project.studio.json");
        auto save = studio::saveProject(path, p, {});
        check(save.error.isEmpty(), "initial atomic save");
        check(!studio::saveProject(path, p, {}).error.isEmpty(), "new file save never silently overwrites");
        auto loaded = studio::loadProject(path);
        check(loaded.error.isEmpty() && loaded.revision == save.revision, "load revision");
        auto modified = p;
        modified.name = "Changed";
        auto updated = studio::saveProject(path, modified, save.revision);
        check(updated.error.isEmpty(), "save known revision");
        check(studio::loadProject(path + ".previous").project.name == p.name, "previous snapshot preserved");
        check(!studio::saveProject(path, p, save.revision).error.isEmpty(), "reject stale revision");
        check(studio::loadProject(path).project.name == "Changed", "stale save preserves current bytes");
        studio::CompositionPainter painter;
        auto first = painter.frame(p, 0), later = painter.frame(p, 90);
        check(first.size() == QSize(p.width, p.height) && first != later, "animated frames differ");
        auto same = painter.frame(p, 90);
        check(same == later, "frame evaluation deterministic");
        const auto svgPath = directory.filePath("cached.svg");
        QFile svgFile(svgPath);
        check(svgFile.open(QIODevice::WriteOnly) &&
                  svgFile.write("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"64\" height=\"64\"><rect "
                                "width=\"64\" height=\"64\" fill=\"#ff0000\"/></svg>") > 0,
              "write SVG renderer cache fixture");
        svgFile.close();
        auto svgProject = studio::makeEmpty();
        svgProject.width = 128;
        svgProject.height = 128;
        svgProject.duration = 4;
        studio::Node svgNode;
        svgNode.id = "cached-vector";
        svgNode.kind = studio::NodeKind::Svg;
        svgNode.source = svgPath.toStdString();
        svgNode.width = 64;
        svgNode.height = 64;
        svgNode.animated = false;
        svgNode.duration = 4;
        svgProject.nodes.push_back(svgNode);
        studio::CompositionPainter svgPainter;
        const auto svgFirst = svgPainter.frame(svgProject, 0);
        const auto svgSecond = svgPainter.frame(svgProject, 1);
        check(svgFirst == svgSecond && svgPainter.svgCacheStats().misses == 1 &&
                  svgPainter.svgCacheStats().hits == 1,
              "reused SVG is parsed once and produces deterministic frames");
        check(svgFile.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
                  svgFile.write("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"64\" height=\"64\"><rect "
                                "width=\"32\" height=\"64\" fill=\"#0000ff\"/></svg>") > 0,
              "replace cached SVG fixture");
        svgFile.close();
        const auto svgAfterReplace = svgPainter.frame(svgProject, 2);
        check(svgAfterReplace != svgFirst && svgPainter.svgCacheStats().misses == 2,
              "updated SVG file invalidates its parsed renderer cache entry");
        const auto ffmpeg = qEnvironmentVariable("STUDIO_FFMPEG", "ffmpeg");
        const auto videoPath = directory.filePath("moving-test.mp4");
        QProcess videoEncoder;
        videoEncoder.start(ffmpeg, {"-y", "-hide_banner", "-loglevel", "error", "-f", "lavfi", "-i",
                                    "testsrc2=size=64x64:rate=30:duration=2", "-c:v", "libx264", "-pix_fmt",
                                    "yuv420p", videoPath});
        check(videoEncoder.waitForStarted(5000) && videoEncoder.waitForFinished(15000) &&
                  videoEncoder.exitCode() == 0,
              "generate moving video fixture");
        auto videoProject = studio::makeEmpty();
        videoProject.schemaVersion = 5;
        videoProject.width = 128;
        videoProject.height = 128;
        videoProject.duration = 60;
        studio::Node video;
        video.id = "video-layer";
        video.kind = studio::NodeKind::Video;
        video.source = videoPath.toStdString();
        video.width = 128;
        video.height = 128;
        video.duration = 60;
        video.animated = false;
        videoProject.nodes.push_back(video);
        check(studio::validate(videoProject).empty(), "video layer project validates");
        const auto savedVideoProject = studio::deserialize(studio::serialize(videoProject));
        check(savedVideoProject.error.isEmpty() && savedVideoProject.project.schemaVersion == 5 &&
                  savedVideoProject.project.nodes.front().kind == studio::NodeKind::Video,
              "video layer survives schema 5 roundtrip");
        studio::CompositionPainter videoPainter;
        const auto videoFrame0 = videoPainter.frame(videoProject, 0);
        const auto videoFrame15 = videoPainter.frame(videoProject, 15);
        check(!videoFrame0.isNull() && videoFrame0 != videoFrame15,
              "video layer decodes successive moving frames");
        auto trimmedVideoProject = videoProject;
        trimmedVideoProject.schemaVersion = 10;
        trimmedVideoProject.nodes.front().videoSourceInSeconds = .5;
        trimmedVideoProject.nodes.front().videoPlaybackRate = 2;
        const auto savedTrimmedVideo = studio::deserialize(studio::serialize(trimmedVideoProject));
        check(studio::validate(trimmedVideoProject).empty() && savedTrimmedVideo.error.isEmpty() &&
                  savedTrimmedVideo.project.nodes.front().videoSourceInSeconds == .5 &&
                  savedTrimmedVideo.project.nodes.front().videoPlaybackRate == 2,
              "video trim and speed survive schema 10 roundtrip");
        studio::CompositionPainter trimmedVideoPainter;
        const auto trimmedFrame0 = trimmedVideoPainter.frame(trimmedVideoProject, 0);
        const auto trimmedFrame15 = trimmedVideoPainter.frame(trimmedVideoProject, 15);
        check(!trimmedFrame0.isNull() && !trimmedFrame15.isNull() && trimmedFrame0 == videoFrame15 &&
                  trimmedFrame15 == videoPainter.frame(videoProject, 45),
              "video preview applies source trim and playback speed at the project frame rate");
        auto croppedVideoProject = videoProject;
        croppedVideoProject.schemaVersion = 10;
        croppedVideoProject.nodes.front().videoCropX = .25;
        croppedVideoProject.nodes.front().videoCropY = .25;
        croppedVideoProject.nodes.front().videoCropWidth = .5;
        croppedVideoProject.nodes.front().videoCropHeight = .5;
        studio::CompositionPainter croppedVideoPainter;
        const auto croppedFrame = croppedVideoPainter.frame(croppedVideoProject, 0);
        check(studio::validate(croppedVideoProject).empty() && !croppedFrame.isNull() &&
                  croppedFrame != videoFrame0,
              "video preview applies a normalized manual crop");
        auto invalidCropProject = croppedVideoProject;
        invalidCropProject.nodes.front().videoCropX = .75;
        check(!studio::validate(invalidCropProject).empty(), "reject a video crop outside the source image");
        auto legacyVideoWithTrim = videoProject;
        legacyVideoWithTrim.nodes.front().videoSourceInSeconds = .5;
        check(!studio::validate(legacyVideoWithTrim).empty(), "video source controls require schema 10");
        auto missing = p;
        studio::Node image;
        image.id = "missing";
        image.kind = studio::NodeKind::Image;
        image.source = "missing.png";
        missing.nodes.push_back(image);
        check(!painter.frame(missing, 90).isNull(), "missing image preview does not crash");
        std::cout << "PASS: " << checks
                  << " checks (commands, timing, persistence, conflicts, composition)\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
