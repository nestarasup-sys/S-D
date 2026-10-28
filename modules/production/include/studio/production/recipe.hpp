#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <optional>
#include <studio/core/project.hpp>
#include <vector>

namespace studio {
struct Utterance {
    QString id, text;
    double targetDurationSeconds{0}; // Optional speech slot including pause in fit mode.
};
struct DirectedTime {
    int cue{0};
    double progress{-1}; // Relative to the measured speech, or -1 when offsetSeconds is used.
    double offsetSeconds{0};
    int wordIndex{-1}; // Zero-based word emitted by the selected local voice.
};
struct DirectedKeyframe {
    int cue{0};
    double progress{0};
    double offsetSeconds{0};
    double cueProgress{-1}; // 0..1 follows the adjusted speech duration.
    double x{0}, y{0}, rotation{0}, scale{1}, opacity{1};
    std::string easing{"linear"};
    int wordIndex{-1};
};
struct DirectedCameraKeyframe {
    int cue{0};
    double progress{0};
    double offsetSeconds{0};
    double cueProgress{-1};
    double x{640}, y{360}, zoom{1}, rotation{0};
    std::string easing{"linear"};
};
struct DirectedCameraPresetKeyframe {
    double offsetSeconds{0};
    double x{640}, y{360}, zoom{1}, rotation{0};
    std::string easing{"linear"};
};
struct DirectedCameraTarget {
    QString targetId;
    QString reveal{"spatial"};
    DirectedTime at;
    double padding{1.25};
    std::string easing{"ease-in-out"};
};
struct DirectedCameraPath {
    double safeMargin{0.08};
    double captionSafeBottom{0.16};
    std::vector<DirectedCameraTarget> targets;
};
struct DirectedElement {
    Node node;
    std::int64_t videoDurationFrames{0};
    int cue{0};
    double progress{0};
    double offsetSeconds{0};
    double cueProgress{-1};
    int wordIndex{-1};
    bool utteranceOnly{false};
    bool shotRelative{false};
    bool presetTiming{false}; // Preserved frame offsets, lifetimes and keys from a stored composition.
    bool premiumTemplate{false}; // Registered official template; internal animation scales to the measured cue span.
    std::vector<DirectedKeyframe> keyframes;
    std::vector<DirectedElement> children;
};
struct DirectedBeat {
    QString id, action, targetId;
    QString soundSource;
    DirectedTime at;
    int durationFrames{0};
    double soundGain{1};
    std::optional<double> x, y, rotation, scale, opacity, zoom;
};
struct DirectedAudioTrack {
    QString id, name;
    QString source;
    DirectedTime start;
    double gain{1}, duckAmount{0.65};
    double sourceInSeconds{0}, playbackRate{1};
    bool duckUnderVoice{false};
    int fadeInFrames{0}, fadeOutFrames{0};
    std::vector<AudioTrack::GainPoint> gainAutomation;
};
struct DirectedShot {
    QString id, name, transitionIn{"cut"}, cameraPreset;
    bool autoStart{false};
    QString transitionDirection{"left"};
    int transitionDurationFrames{0};
    DirectedTime start;
    double cameraX{640}, cameraY{360}, cameraZoom{1}, cameraRotation{0};
    QString focusTargetId;
    double focusPadding{1.25};
    bool focusFollow{false};
    std::optional<DirectedCameraPath> cameraPath;
    std::optional<ArtDirection> artDirection;
    std::vector<DirectedElement> elements;
    std::vector<DirectedBeat> beats;
};
struct DirectedScene {
    QString id, title, kicker, background;
    double targetDurationSeconds{0};
    std::optional<ArtDirection> artDirection;
    std::vector<Utterance> narration;
    std::vector<DirectedElement> elements;
    std::vector<DirectedCameraKeyframe> camera;
    std::vector<DirectedShot> shots;
    std::vector<DirectedAudioTrack> audioTracks;
};
struct ProductionRecipe {
    int version{1};
    QString title, voice{"auto"};
    int speechRate{0};
    bool renderCaptions{true};
    QString timingMode{"natural"};
    double targetDurationSeconds{0};
    bool preserveVoicePitch{true};
    int width{1280}, height{720};
    int editingIntensity{-1}; // Optional v3 preset for generated, otherwise-unspecified motion defaults.
    QString editingRhythm{"manual"};
    QString wordAnimationPreset{"highlight"};
    int wordAnimationDurationFrames{8};
    double wordAnimationStartScale{0.82}, wordAnimationTravelY{12};
    ArtDirection artDirection;
    QMap<QString, std::vector<DirectedCameraPresetKeyframe>> cameraPresets;
    double musicGain{.025}, effectsGain{.035};
    std::vector<DirectedScene> scenes;
    QJsonObject original;
};
struct RecipeResult {
    ProductionRecipe recipe;
    QString error;
};
RecipeResult parseRecipe(const QByteArray &, const QString &baseDirectory, const QJsonObject &library = {});
QJsonObject capabilities();
QString aiGuide(const QJsonObject &library = {});
} // namespace studio
