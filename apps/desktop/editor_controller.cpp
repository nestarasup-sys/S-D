#include "editor_controller.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QSvgRenderer>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <studio/core/library.hpp>
#include <studio/media/wave.hpp>
#include <unordered_map>

namespace {
QString q(const std::string &s) {
    return QString::fromStdString(s);
}
void upsertKeyframe(std::vector<studio::TransformKeyframe> &keys, studio::TransformKeyframe key) {
    auto it =
        std::lower_bound(keys.begin(), keys.end(), key.frame,
                         [](const auto &candidate, std::int64_t frame) { return candidate.frame < frame; });
    if (it != keys.end() && it->frame == key.frame) {
        key.easing = it->easing; // Editing values must preserve the curve.
        *it = key;
    } else
        keys.insert(it, key);
}
void upsertKeyframe(std::vector<studio::CameraKeyframe> &keys, studio::CameraKeyframe key) {
    auto it =
        std::lower_bound(keys.begin(), keys.end(), key.frame,
                         [](const auto &candidate, std::int64_t frame) { return candidate.frame < frame; });
    if (it != keys.end() && it->frame == key.frame) {
        key.easing = it->easing; // Editing values must preserve the curve.
        if (!it->shotId.empty())
            key.shotId = it->shotId;
        *it = key;
    } else
        keys.insert(it, key);
}
QVariantMap map(const studio::Node &n) {
    return {{"id", q(n.id)},
            {"name", q(n.name)},
            {"parentId", q(n.parentId)},
            {"keyframeCount", int(n.keyframes.size())},
            {"kind", n.kind == studio::NodeKind::Group    ? "group"
                     : n.kind == studio::NodeKind::Text   ? "text"
                     : n.kind == studio::NodeKind::Shape  ? "shape"
                     : n.kind == studio::NodeKind::Native ? "native"
                     : n.kind == studio::NodeKind::Svg    ? "svg"
                     : n.kind == studio::NodeKind::Video  ? "video"
                                                          : "image"},
            {"text", q(n.text)},
            {"color", q(n.color)},
            {"start", int(n.start)},
            {"duration", int(n.duration)},
            {"x", n.x},
            {"y", n.y},
            {"width", n.width},
            {"height", n.height},
            {"opacity", n.opacity},
            {"fontSize", n.fontSize},
            {"animated", n.animated},
            {"nativeId", q(n.nativeId)},
            {"pose", q(n.pose)},
            {"motion", q(n.motion)},
            {"secondaryColor", q(n.secondaryColor)},
            {"rotation", n.rotation},
            {"scale", n.scale},
            {"travelX", n.travelX},
            {"travelY", n.travelY},
            {"spinDegrees", n.spinDegrees}};
}
} // namespace
EditorController::EditorController(QObject *parent, QString recoveryDirectory) : QObject(parent) {
    savedDocument_ = studio::serialize(project());
    nodeModel_.sync(nodes());
    diagnosticsDirectory_ =
        (recoveryDirectory.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                                     : recoveryDirectory) +
        "/logs";
    setupProduction();
    audioPlayer_.setAudioOutput(&audioOutput_);
    audioOutput_.setVolume(.9);
    connect(&audioPlayer_, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, QString text) {
        if (hasAudio()) {
            stop();
            notify("Áudio na prévia: " + text);
        }
    });
    connect(this, &EditorController::documentChanged, this, [this] { nodeModel_.sync(nodes()); });
    auto data = recoveryDirectory.isEmpty()
                    ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                    : recoveryDirectory;
    QDir().mkpath(data);
    recoveryPath_ = data + "/recovery.studio.json";
    if (QFile::exists(recoveryPath_)) {
        auto r = studio::loadProject(recoveryPath_);
        recoveryRevision_ = r.revision;
    }
    playback_.setInterval(8);
    connect(&playback_, &QTimer::timeout, this, [this] {
        const auto next = hasMainAudio() ? int(audioPlayer_.position() * fps() / 1000.)
                                         : playStart_ + int(clock_.elapsed() * fps() / 1000.);
        if (hasMainAudio() && audioPlayer_.mediaStatus() == QMediaPlayer::EndOfMedia) {
            presentFrame(duration() - 1);
            stop();
            return;
        }
        syncAudioTrackPlayback(next);
        if (next >= duration()) {
            presentFrame(duration() - 1);
            stop();
        } else
            presentFrame(next);
    });
    autosave_.setInterval(1200);
    connect(&autosave_, &QTimer::timeout, this, &EditorController::autosave);
    autosave_.start();
    connect(&render_, &QProcess::readyReadStandardOutput, this, [this] {
        renderBuffer_ += render_.readAllStandardOutput();
        while (renderBuffer_.contains('\n')) {
            auto index = renderBuffer_.indexOf('\n');
            auto line = renderBuffer_.left(index);
            renderBuffer_.remove(0, index + 1);
            auto o = QJsonDocument::fromJson(line).object();
            exportProgress_ = o["progress"].toInt();
            if (!o["error"].toString().isEmpty())
                notify(o["error"].toString());
            emit exportChanged();
        }
    });
    connect(&render_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
                if (code == 0 && status == QProcess::NormalExit)
                    notify("MP4 exportado com sucesso.");
                else if (status == QProcess::CrashExit)
                    notify("O render foi interrompido; seu projeto foi preservado.");
                QFile::remove(renderSnapshot_);
                QFile::remove(cancelPath_);
                emit exportChanged();
            });
    connect(&render_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        notify("Erro no processo de render: " + render_.errorString());
        emit exportChanged();
    });
}
QVariantList EditorController::nodes() const {
    QVariantList list;
    const auto scene = currentSceneId();
    for (const auto &n : project().nodes)
        if (n.sceneId.empty() || q(n.sceneId) == scene)
            list.append(map(n));
    return list;
}
QVariantMap EditorController::selected() const {
    auto n = studio::findNode(project(), selectedId_.toStdString());
    if (!n)
        return {};
    auto result = map(*n);
    if (!n->keyframes.empty()) {
        auto key = std::find_if(n->keyframes.begin(), n->keyframes.end(),
                                [this](const auto &k) { return k.frame == frame_; });
        result["keyframeEasing"] =
            key == n->keyframes.end() ? QString{} : QString::fromStdString(key->easing);
        const auto state = studio::evaluate(*n, frame_);
        result["x"] = state.x;
        result["y"] = state.y;
        result["opacity"] = state.opacity;
        result["rotation"] = state.rotation;
        result["scale"] = state.scale;
    }
    return result;
}
QVariantMap EditorController::camera() const {
    const auto state = studio::evaluateCamera(project(), frame_);
    QString curve;
    for (const auto &key : project().camera)
        if (key.frame == frame_)
            curve = QString::fromStdString(key.easing);
    return {{"x", state.x},
            {"y", state.y},
            {"zoom", state.zoom},
            {"rotation", state.rotation},
            {"shotId", currentShotId()},
            {"keyframeEasing", curve}};
}
QVariantMap EditorController::artDirection() const {
    const auto &art = project().artDirection;
    return {{"backgroundColor", q(art.backgroundColor)},
            {"glowColor", q(art.glowColor)},
            {"primaryColor", q(art.primaryColor)},
            {"secondaryColor", q(art.secondaryColor)},
            {"textColor", q(art.textColor)},
            {"cardColor", q(art.cardColor)},
            {"captionBackgroundColor", q(art.captionBackgroundColor)},
            {"fontFamily", q(art.fontFamily)},
            {"captionPosition", q(art.captionPosition)},
            {"titleSize", art.titleSize},
            {"bodySize", art.bodySize},
            {"captionSize", art.captionSize},
            {"safeMargin", art.safeMargin}};
}
QString EditorController::title() const {
    return q(project().name);
}
bool EditorController::dirty() const {
    return savedDocument_ != studio::serialize(project());
}
bool EditorController::recoveryAvailable() const {
    return QFile::exists(recoveryPath_);
}
void EditorController::notify(QString message) {
    message_ = std::move(message);
    emit messageChanged();
}
void EditorController::changed() {
    autosaveEnabled_ = true;
    emit documentChanged();
    emit selectedChanged();
    emit cameraChanged();
}
void EditorController::selectNode(QString id) {
    selectedId_ = std::move(id);
    emit documentChanged();
    emit selectedChanged();
}
void EditorController::beginGesture() {
    editor_.beginGesture();
}
void EditorController::endGesture() {
    editor_.endGesture();
    emit documentChanged();
}
void EditorController::updateSelected(QString key, QVariant value) {
    auto before = selected();
    const bool transformProperty =
        key == "x" || key == "y" || key == "opacity" || key == "rotation" || key == "scale";
    const auto selectedNode = studio::findNode(project(), selectedId_.toStdString());
    const bool keyframedEdit = selectedNode && !selectedNode->keyframes.empty() && transformProperty;
    if (before.isEmpty() || !before.contains(key) || (before[key] == value && !keyframedEdit))
        return;
    if ((key == "start" || key == "duration") &&
        (!std::isfinite(value.toDouble()) || std::floor(value.toDouble()) != value.toDouble())) {
        notify("Tempos devem ser números inteiros de frames.");
        return;
    }
    auto error = editor_.execute([&](studio::Project &p) {
        for (auto &n : p.nodes)
            if (q(n.id) == selectedId_) {
                if (!n.keyframes.empty() && transformProperty) {
                    auto state = studio::evaluate(n, frame_);
                    if (key == "x")
                        state.x = value.toDouble();
                    else if (key == "y")
                        state.y = value.toDouble();
                    else if (key == "opacity")
                        state.opacity = value.toDouble();
                    else if (key == "rotation")
                        state.rotation = value.toDouble();
                    else if (key == "scale")
                        state.scale = value.toDouble();
                    upsertKeyframe(n.keyframes,
                                   {frame_, state.x, state.y, state.rotation, state.scale, state.opacity});
                    p.schemaVersion = std::max(p.schemaVersion, 3);
                    continue;
                }
                if (key == "text")
                    n.text = value.toString().toStdString();
                else if (key == "color")
                    n.color = value.toString().toStdString();
                else if (key == "x")
                    n.x = value.toDouble();
                else if (key == "y")
                    n.y = value.toDouble();
                else if (key == "width")
                    n.width = value.toDouble();
                else if (key == "height")
                    n.height = value.toDouble();
                else if (key == "opacity")
                    n.opacity = value.toDouble();
                else if (key == "fontSize")
                    n.fontSize = value.toDouble();
                else if (key == "start")
                    n.start = value.toInt();
                else if (key == "duration")
                    n.duration = value.toInt();
                else if (key == "animated")
                    n.animated = value.toBool();
                else if (key == "pose")
                    n.pose = value.toString().toStdString();
                else if (key == "motion")
                    n.motion = value.toString().toStdString();
                else if (key == "secondaryColor")
                    n.secondaryColor = value.toString().toStdString();
                else if (key == "rotation")
                    n.rotation = value.toDouble();
                else if (key == "scale")
                    n.scale = value.toDouble();
                else if (key == "travelX")
                    n.travelX = value.toDouble();
                else if (key == "travelY")
                    n.travelY = value.toDouble();
                else if (key == "spinDegrees")
                    n.spinDegrees = value.toDouble();
            }
    });
    if (error.empty())
        changed();
    else
        notify(q(error));
}
void EditorController::moveSelected(double x, double y) {
    auto n = studio::findNode(project(), selectedId_.toStdString());
    if (!n)
        return;
    const auto current = n->keyframes.empty()
                             ? studio::VisualState{n->x, n->y, n->opacity, n->rotation, n->scale}
                             : studio::evaluate(*n, frame_);
    if (current.x == x && current.y == y)
        return;
    auto error = editor_.execute([&](studio::Project &p) {
        for (auto &node : p.nodes)
            if (q(node.id) == selectedId_) {
                if (node.keyframes.empty()) {
                    node.x = x;
                    node.y = y;
                } else {
                    auto state = studio::evaluate(node, frame_);
                    state.x = x;
                    state.y = y;
                    upsertKeyframe(node.keyframes,
                                   {frame_, state.x, state.y, state.rotation, state.scale, state.opacity});
                    p.schemaVersion = std::max(p.schemaVersion, 3);
                }
            }
    });
    if (error.empty())
        changed();
    else
        notify(q(error));
}
void EditorController::moveClip(QString id, int start) {
    auto node = studio::findNode(project(), id.toStdString());
    if (!node)
        return;
    std::vector<std::string> ids{node->id};
    if (node->kind == studio::NodeKind::Group)
        for (std::size_t i = 0; i < ids.size(); ++i)
            for (const auto &child : project().nodes)
                if (child.parentId == ids[i])
                    ids.push_back(child.id);
    int lowerDelta = -int(node->start);
    int upperDelta = duration() - int(node->start + node->duration);
    for (const auto &candidate : project().nodes)
        if (std::find(ids.begin(), ids.end(), candidate.id) != ids.end())
            for (const auto &scene : project().scenes)
                if (scene.id == candidate.sceneId) {
                    lowerDelta = std::max(lowerDelta, int(scene.start - candidate.start));
                    upperDelta = std::min(
                        upperDelta, int(scene.start + scene.duration - candidate.start - candidate.duration));
                }
    const int delta = std::clamp(start - int(node->start), lowerDelta, upperDelta);
    if (delta == 0)
        return;
    auto error = editor_.execute([&](studio::Project &p) {
        for (auto &n : p.nodes)
            if (std::find(ids.begin(), ids.end(), n.id) != ids.end()) {
                n.start += delta;
                for (auto &key : n.keyframes)
                    key.frame += delta;
            }
    });
    if (error.empty())
        changed();
    else
        notify(q(error));
}
void EditorController::moveShotBoundary(QString rightShotId, int requestedFrame) {
    const auto rightId = rightShotId.toStdString();
    const auto right = std::find_if(project().shots.begin(), project().shots.end(),
                                    [&](const auto &shot) { return shot.id == rightId; });
    if (right == project().shots.end())
        return;
    const auto left =
        std::max_element(project().shots.begin(), project().shots.end(), [&](const auto &a, const auto &b) {
            const bool aCandidate = a.sceneId == right->sceneId && a.start < right->start;
            const bool bCandidate = b.sceneId == right->sceneId && b.start < right->start;
            if (aCandidate != bCandidate)
                return !aCandidate;
            return aCandidate && a.start < b.start;
        });
    if (left == project().shots.end() || left->sceneId != right->sceneId || left->start >= right->start)
        return;
    const auto oldBoundary = right->start;
    const auto rightEnd = right->start + right->duration;
    const auto boundary = std::clamp<std::int64_t>(requestedFrame, left->start + 1, rightEnd - 1);
    const auto delta = boundary - oldBoundary;
    if (delta == 0)
        return;
    for (const auto &node : project().nodes) {
        if (node.shotId == left->id && node.start + node.duration > boundary &&
            (node.start >= boundary || std::any_of(node.keyframes.begin(), node.keyframes.end(),
                                                   [&](const auto &key) { return key.frame >= boundary; }))) {
            notify("O corte não pode atravessar um elemento ou keyframe do shot anterior.");
            return;
        }
        if (node.shotId == rightId) {
            const bool reachesOldEnd = node.start + node.duration == rightEnd;
            const auto newStart = node.start + delta;
            const auto newDuration = reachesOldEnd ? rightEnd - newStart : node.duration;
            if (newStart >= rightEnd || newDuration < 1 || newStart + newDuration > rightEnd) {
                notify("O shot não cabe na nova duração porque contém elementos no final do plano.");
                return;
            }
        }
    }
    for (const auto &beat : project().visualBeats) {
        if (beat.shotId == left->id && beat.frame + beat.durationFrames >= boundary) {
            notify("O corte não pode atravessar um Visual Beat do shot anterior.");
            return;
        }
        if (beat.shotId == rightId && beat.frame + delta + beat.durationFrames >= rightEnd) {
            notify("O shot não cabe na nova duração porque contém Visual Beats no final do plano.");
            return;
        }
    }
    const auto leftId = left->id;
    const auto error = editor_.execute([&](studio::Project &p) {
        auto leftShot =
            std::find_if(p.shots.begin(), p.shots.end(), [&](const auto &shot) { return shot.id == leftId; });
        auto rightShot = std::find_if(p.shots.begin(), p.shots.end(),
                                      [&](const auto &shot) { return shot.id == rightId; });
        leftShot->duration += delta;
        rightShot->start += delta;
        rightShot->duration -= delta;
        for (auto &node : p.nodes) {
            if (node.shotId == leftId && node.start + node.duration > boundary)
                node.duration = boundary - node.start;
            if (node.shotId == rightId) {
                const bool reachedOldEnd = node.start + node.duration == rightEnd;
                node.start += delta;
                if (reachedOldEnd)
                    node.duration = rightEnd - node.start;
                for (auto &key : node.keyframes)
                    key.frame += delta;
            }
        }
        for (auto &beat : p.visualBeats)
            if (beat.shotId == rightId)
                beat.frame += delta;
        for (auto &key : p.camera)
            if (key.shotId == rightId)
                key.frame += delta;
    });
    if (error.empty())
        changed();
    else
        notify(q(error));
}
void EditorController::addNode(QString kind, QUrl source) {
    studio::Node n;
    n.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    n.start = frame_;
    n.duration = duration() - frame_;
    n.sceneId = currentSceneId().toStdString();
    for (const auto &s : project().scenes)
        if (s.id == n.sceneId)
            n.duration = std::max<std::int64_t>(1, s.start + s.duration - frame_);
    n.x = 140;
    n.y = 240;
    if (kind == "text") {
        n.name = "Texto";
        n.text = "Sua próxima ideia";
        n.height = 180;
    } else if (kind == "shape") {
        n.kind = studio::NodeKind::Shape;
        n.name = "Forma";
        n.width = 260;
        n.height = 160;
        n.color = "#b4f078";
    } else if (kind == "svg") {
        auto path = source.toLocalFile();
        QSvgRenderer svg(path);
        if (QFileInfo(path).size() > 4 * 1024 * 1024 || !svg.isValid()) {
            notify("SVG inválido ou maior que 4 MiB.");
            return;
        }
        n.kind = studio::NodeKind::Svg;
        n.name = QFileInfo(path).fileName().toStdString();
        n.source = path.toStdString();
        n.width = 400;
        n.height = 300;
    } else if (kind == "image") {
        QImageReader reader(source.toLocalFile());
        reader.setAutoTransform(true);
        auto size = reader.size();
        if (!reader.canRead() || size.width() < 1 || size.height() < 1 ||
            qint64(size.width()) * size.height() > 40000000) {
            notify("Imagem inválida ou maior que 40 megapixels.");
            return;
        }
        n.kind = studio::NodeKind::Image;
        n.name = QFileInfo(source.toLocalFile()).fileName().toStdString();
        n.source = source.toLocalFile().toStdString();
        n.width = 640;
        n.height = std::clamp(640. * size.height() / size.width(), 1., 720.);
        n.x = 320;
        n.y = 100;
    } else if (kind == "video") {
        const auto path = source.toLocalFile();
        const auto suffix = QFileInfo(path).suffix().toLower();
        static const QSet<QString> formats{"mp4", "mov", "mkv", "webm", "avi", "m4v"};
        if (!formats.contains(suffix) || !QFileInfo(path).isFile() || QFileInfo(path).size() <= 0) {
            notify("Escolha um vídeo local MP4, MOV, MKV, WebM, AVI ou M4V.");
            return;
        }
        n.kind = studio::NodeKind::Video;
        n.name = QFileInfo(path).fileName().toStdString();
        n.source = path.toStdString();
        n.width = canvasWidth();
        n.height = canvasHeight();
        n.x = 0;
        n.y = 0;
        n.animated = false;
        n.motion = "none";
        n.exitFrames = 0;
    } else if (const auto item = studio::libraryItem(kind.toStdString())) {
        n.kind = studio::NodeKind::Native;
        n.nativeId = item->id;
        n.name = item->name;
        n.width = item->width;
        n.height = item->height;
        n.color = "#efb873";
        if (item->category == "cenários") {
            n.x = 0;
            n.y = 0;
            n.width = canvasWidth();
            n.height = canvasHeight();
            n.animated = false;
            n.role = "background";
        }
    } else
        return;
    insertLibraryNode(std::move(n));
}
bool EditorController::insertLibraryNode(studio::Node n) {
    n.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    n.start = frame_;
    n.duration = duration() - frame_;
    n.sceneId = currentSceneId().toStdString();
    for (const auto &scene : project().scenes)
        if (scene.id == n.sceneId)
            n.duration = scene.start + scene.duration - frame_;
    if (const auto item = studio::libraryItem(n.nativeId);
        n.kind == studio::NodeKind::Native && item && item->category == "cenários") {
        n.role = "background";
        n.x = 0;
        n.y = 0;
        n.width = canvasWidth();
        n.height = canvasHeight();
        n.animated = false;
    }
    auto error = editor_.execute([&](studio::Project &p) {
        if (n.kind == studio::NodeKind::Video)
            p.schemaVersion = 5;
        if (n.role == "background")
            p.nodes.insert(p.nodes.begin(), n);
        else
            p.nodes.push_back(n);
    });
    if (error.empty()) {
        selectedId_ = q(n.id);
        changed();
        notify("Objeto adicionado. Edite no inspetor ou arraste na prévia.");
        return true;
    } else
        notify(q(error));
    return false;
}
bool EditorController::insertLibraryNodes(std::vector<studio::Node> nodes) {
    if (nodes.empty()) {
        notify("Composição vazia.");
        return false;
    }
    const auto sceneId = currentSceneId().toStdString();
    std::int64_t remaining = duration() - frame_;
    for (const auto &scene : project().scenes)
        if (scene.id == sceneId)
            remaining = scene.start + scene.duration - frame_;
    if (nodes.front().start != 0 || nodes.front().duration > remaining) {
        notify("A composição não cabe no intervalo restante desta cena. Posicione o cursor antes.");
        return false;
    }
    std::unordered_map<std::string, std::string> ids;
    for (const auto &n : nodes)
        if (n.id.empty() ||
            !ids.emplace(n.id, QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString()).second) {
            notify("Composição possui IDs inválidos ou duplicados.");
            return false;
        }
    for (auto &n : nodes) {
        const auto oldId = n.id, parent = n.parentId;
        n.id = ids.at(oldId);
        if (!parent.empty()) {
            if (!ids.contains(parent)) {
                notify("Composição contém um pai externo.");
                return false;
            }
            n.parentId = ids.at(parent);
        }
        n.start += frame_;
        for (auto &key : n.keyframes)
            key.frame += frame_;
        n.sceneId = sceneId;
    }
    const auto rootId = nodes.front().id;
    const auto error = editor_.execute([&](studio::Project &p) {
        p.schemaVersion = std::max(p.schemaVersion, 3);
        p.nodes.insert(p.nodes.end(), nodes.begin(), nodes.end());
    });
    if (!error.empty()) {
        notify(q(error));
        return false;
    }
    selectedId_ = q(rootId);
    changed();
    notify("Composição reutilizada com filhos e keyframes; uma ação de desfazer.");
    return true;
}
void EditorController::removeSelected() {
    const auto *selectedNode = studio::findNode(project(), selectedId_.toStdString());
    if (!selectedNode)
        return;
    std::vector<std::string> ids{selectedNode->id};
    if (selectedNode->kind == studio::NodeKind::Group) {
        for (std::size_t i = 0; i < ids.size(); ++i)
            for (const auto &node : project().nodes)
                if (node.parentId == ids[i])
                    ids.push_back(node.id);
    }
    editor_.execute([&](studio::Project &p) {
        std::erase_if(p.nodes, [&](const studio::Node &n) {
            return std::find(ids.begin(), ids.end(), n.id) != ids.end();
        });
    });
    selectedId_.clear();
    changed();
}
void EditorController::groupSelected() {
    const auto *selectedNode = studio::findNode(project(), selectedId_.toStdString());
    if (!selectedNode || selectedNode->kind == studio::NodeKind::Group ||
        selectedNode->role == "background") {
        notify("Selecione um objeto, não um grupo ou cenário de fundo.");
        return;
    }
    const studio::Node childSnapshot = *selectedNode;
    studio::Node group;
    group.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    group.name = (childSnapshot.name.empty() ? std::string("Grupo") : childSnapshot.name + " · grupo");
    group.kind = studio::NodeKind::Group;
    group.parentId = childSnapshot.parentId;
    group.start = childSnapshot.start;
    group.duration = childSnapshot.duration;
    group.x = childSnapshot.x;
    group.y = childSnapshot.y;
    group.width = childSnapshot.width;
    group.height = childSnapshot.height;
    group.sceneId = childSnapshot.sceneId;
    group.shotId = childSnapshot.shotId;
    auto error = editor_.execute([&](studio::Project &p) {
        auto child = std::find_if(p.nodes.begin(), p.nodes.end(),
                                  [&](const auto &node) { return node.id == childSnapshot.id; });
        if (child == p.nodes.end())
            return;
        child->x -= group.x;
        child->y -= group.y;
        for (auto &key : child->keyframes) {
            key.x -= group.x;
            key.y -= group.y;
        }
        child->parentId = group.id;
        p.nodes.push_back(group);
        p.schemaVersion = std::max(p.schemaVersion, 3);
    });
    if (error.empty()) {
        selectedId_ = q(group.id);
        changed();
        notify("Grupo criado. O objeto filho continua editável na lista da cena.");
    } else
        notify(q(error));
}
void EditorController::recordSelectedKeyframe() {
    const auto *selectedNode = studio::findNode(project(), selectedId_.toStdString());
    if (!selectedNode) {
        notify("Selecione um objeto ou grupo antes de gravar um keyframe.");
        return;
    }
    const studio::Node snapshot = *selectedNode;
    const auto state = studio::evaluate(snapshot, frame_);
    auto error = editor_.execute([&](studio::Project &p) {
        auto node = std::find_if(p.nodes.begin(), p.nodes.end(),
                                 [&](const auto &candidate) { return candidate.id == snapshot.id; });
        if (node == p.nodes.end())
            return;
        if (node->keyframes.empty()) {
            node->x = state.x;
            node->y = state.y;
            node->rotation = state.rotation;
            node->scale = state.scale;
            node->opacity = state.opacity;
            node->animated = false;
            node->motion = "none";
            node->travelX = 0;
            node->travelY = 0;
            node->spinDegrees = 0;
        }
        upsertKeyframe(node->keyframes,
                       {frame_, state.x, state.y, state.rotation, state.scale, state.opacity});
        p.schemaVersion = std::max(p.schemaVersion, 3);
    });
    if (error.empty()) {
        changed();
        notify(QString("Keyframe gravado no quadro %1.").arg(frame_));
    } else
        notify(q(error));
}
void EditorController::setSelectedKeyframeEasing(QString easing) {
    if (!studio::supportedEasing(easing.toStdString()))
        return;
    auto n = studio::findNode(project(), selectedId_.toStdString());
    if (!n)
        return;
    const auto at = std::find_if(n->keyframes.begin(), n->keyframes.end(),
                                 [this](const auto &k) { return k.frame == frame_; });
    if (at == n->keyframes.end()) {
        notify("Grave primeiro um keyframe neste quadro para configurar a curva.");
        return;
    }
    auto error = editor_.execute([&](studio::Project &p) {
        for (auto &node : p.nodes)
            if (q(node.id) == selectedId_)
                for (auto &key : node.keyframes)
                    if (key.frame == frame_)
                        key.easing = easing.toStdString();
    });
    if (error.empty())
        changed();
    else
        notify(q(error));
}
void EditorController::setCameraKeyframeEasing(QString easing) {
    if (!studio::supportedEasing(easing.toStdString()))
        return;
    auto at = std::find_if(project().camera.begin(), project().camera.end(),
                           [this](const auto &k) { return k.frame == frame_; });
    if (at == project().camera.end()) {
        notify("Altere primeiro um valor da câmera para gravar o keyframe deste quadro.");
        return;
    }
    const auto error = editor_.execute([&](studio::Project &p) {
        for (auto &key : p.camera)
            if (key.frame == frame_)
                key.easing = easing.toStdString();
    });
    if (error.empty())
        changed();
    else
        notify(q(error));
}
void EditorController::updateCamera(QString property, double value) {
    if (property != "x" && property != "y" && property != "zoom" && property != "rotation")
        return;
    const auto shotId = currentShotId().toStdString();
    auto error = editor_.execute([&](studio::Project &p) {
        auto state = studio::evaluateCamera(p, frame_);
        if (property == "x")
            state.x = value;
        else if (property == "y")
            state.y = value;
        else if (property == "zoom")
            state.zoom = value;
        else
            state.rotation = value;
        upsertKeyframe(p.camera, {frame_, state.x, state.y, state.zoom, state.rotation, "linear", shotId});
        p.schemaVersion = std::max(p.schemaVersion, 3);
    });
    if (error.empty())
        changed();
    else
        notify(q(error));
}
void EditorController::updateArtDirection(QString property, QVariant value) {
    const auto error = editor_.execute([&](studio::Project &p) {
        auto &art = p.artDirection;
        const auto key = property.toStdString();
        if (key == "backgroundColor") art.backgroundColor = value.toString().toStdString();
        else if (key == "glowColor") art.glowColor = value.toString().toStdString();
        else if (key == "primaryColor") art.primaryColor = value.toString().toStdString();
        else if (key == "secondaryColor") art.secondaryColor = value.toString().toStdString();
        else if (key == "textColor") art.textColor = value.toString().toStdString();
        else if (key == "cardColor") art.cardColor = value.toString().toStdString();
        else if (key == "captionBackgroundColor")
            art.captionBackgroundColor = value.toString().toStdString();
        else if (key == "fontFamily") art.fontFamily = value.toString().toStdString();
        else if (key == "captionPosition") art.captionPosition = value.toString().toStdString();
        else if (key == "titleSize") art.titleSize = value.toDouble();
        else if (key == "bodySize") art.bodySize = value.toDouble();
        else if (key == "captionSize") art.captionSize = value.toDouble();
        else if (key == "safeMargin") art.safeMargin = value.toInt();
    });
    if (error.empty())
        changed();
    else
        notify(q(error));
}
void EditorController::applyEditingIntensity(int intensity) {
    if (intensity < 0 || intensity > 100) {
        notify("A intensidade precisa ficar entre 0 e 100.");
        return;
    }
    const auto sceneId = currentSceneId().toStdString();
    const auto error = editor_.execute([&](studio::Project &p) {
        for (auto &node : p.nodes) {
            if (node.kind == studio::NodeKind::Group || node.kind == studio::NodeKind::Video ||
                (!sceneId.empty() && !node.sceneId.empty() && node.sceneId != sceneId))
                continue;
            if (intensity == 0) {
                node.motion = "none";
                node.animated = false;
                node.exitFrames = 0;
            } else {
                node.motion = intensity <= 30 ? "fade" : intensity <= 70 ? "rise" : "pop";
                node.animated = true;
                if (intensity <= 30)
                    node.motionFrames = int(std::lround(48.0 - intensity * 18.0 / 30.0));
                else if (intensity <= 70)
                    node.motionFrames = int(std::lround(30.0 - (intensity - 30) * 10.0 / 40.0));
                else
                    node.motionFrames = int(std::lround(20.0 - (intensity - 70) * 12.0 / 30.0));
                if (intensity <= 30)
                    node.exitFrames = int(std::lround(20.0 - intensity * 8.0 / 30.0));
                else if (intensity <= 70)
                    node.exitFrames = int(std::lround(12.0 - (intensity - 30) * 4.0 / 40.0));
                else
                    node.exitFrames = int(std::lround(8.0 - (intensity - 70) * 5.0 / 30.0));
            }
        }
    });
    if (error.empty())
        changed();
    else
        notify(q(error));
}
void EditorController::updateShotTransition(QString shotId, QString type, int durationFrames,
                                            QString direction) {
    const auto id = shotId.toStdString();
    if (type == "cut") {
        durationFrames = 0;
        direction = "left";
    } else if (durationFrames == 0) {
        durationFrames = 12;
    }
    const auto error = editor_.execute([&](studio::Project &p) {
        for (auto &shot : p.shots)
            if (shot.id == id) {
                shot.transitionIn = type.toStdString();
                shot.transitionDurationFrames = durationFrames;
                shot.transitionDirection = direction.toStdString();
            }
    });
    if (error.empty())
        changed();
    else
        notify(q(error));
}
void EditorController::undo() {
    if (editor_.undo())
        changed();
}
void EditorController::redo() {
    if (editor_.redo())
        changed();
}
void EditorController::setFrame(int frame) {
    if (playing_)
        stop();
    presentFrame(frame);
}
void EditorController::presentFrame(int frame) {
    frame = std::clamp(frame, 0, duration() - 1);
    if (frame_ == frame)
        return;
    auto oldScene = currentSceneId();
    frame_ = frame;
    emit frameChanged();
    emit cameraChanged();
    emit selectedChanged();
    if (oldScene != currentSceneId()) {
        selectedId_.clear();
        emit documentChanged();
        emit selectedChanged();
    }
}
void EditorController::stop() {
    playing_ = false;
    playback_.stop();
    audioPlayer_.pause();
    for (auto &player : audioTrackPlayers_)
        player->pause();
    std::fill(audioTrackPlaybackActive_.begin(), audioTrackPlaybackActive_.end(), false);
    emit playingChanged();
}
void EditorController::togglePlayback() {
    if (playing_) {
        stop();
        return;
    }
    if (frame_ >= duration() - 1)
        setFrame(0);
    playStart_ = frame_;
    clock_.restart();
    playing_ = true;
    if (hasMainAudio()) {
        if (!QFile::exists(q(project().audioSource))) {
            playing_ = false;
            notify("Arquivo de áudio ausente.");
            return;
        }
        audioPlayer_.setPosition(qRound64(frame_ * 1000. / fps()));
        audioPlayer_.play();
    }
    syncAudioTrackPlayback(frame_);
    playback_.start();
    emit playingChanged();
}
bool EditorController::save(QUrl url) {
    auto target = url.isEmpty() ? path_ : url.toLocalFile();
    if (target.isEmpty())
        return false;
    auto result = studio::saveProject(target, project(), target == path_ ? revision_ : QByteArray{});
    if (!result.error.isEmpty()) {
        notify(result.error);
        return false;
    }
    path_ = target;
    revision_ = result.revision;
    savedDocument_ = studio::serialize(project());
    notify("Projeto salvo: " + QFileInfo(path_).fileName());
    emit documentChanged();
    emit projectAvailable(path_);
    return true;
}
void EditorController::reset(studio::Project p, QString path, QByteArray revision) {
    stop();
    editor_.reset(std::move(p));
    path_ = std::move(path);
    revision_ = std::move(revision);
    savedDocument_ = studio::serialize(project());
    selectedId_.clear();
    frame_ = 0;
    audioPlayer_.setSource(project().audioSource.empty() ? QUrl{}
                                                         : QUrl::fromLocalFile(q(project().audioSource)));
    refreshAudioTracks();
    waveform_.clear();
    if (hasAudio()) {
        auto wave = studio::readWave(q(project().audioSource));
        if (wave.error.isEmpty())
            for (int i = 0; i < 256; ++i) {
                auto begin = wave.audio.samples.size() * i / 256,
                     end = wave.audio.samples.size() * (i + 1) / 256;
                double sum = 0;
                for (auto j = begin; j < end; ++j)
                    sum += wave.audio.samples[j] * wave.audio.samples[j];
                waveform_.append(std::sqrt(sum / std::max<std::size_t>(1, end - begin)));
            }
    }
    autosaveEnabled_ = false;
    emit frameChanged();
    emit documentChanged();
    emit selectedChanged();
    emit cameraChanged();
}
void EditorController::refreshAudioTracks() {
    for (auto &player : audioTrackPlayers_)
        player->stop();
    audioTrackPlayers_.clear();
    audioTrackOutputs_.clear();
    audioTrackPlaybackActive_.clear();
    for (const auto &track : project().audioTracks) {
        auto output = std::make_unique<QAudioOutput>(this);
        output->setVolume(track.muted ? 0 : std::clamp(track.gain * .9, 0.0, 1.0));
        auto player = std::make_unique<QMediaPlayer>(this);
        player->setAudioOutput(output.get());
        player->setSource(QUrl::fromLocalFile(q(track.source)));
        connect(player.get(), &QMediaPlayer::errorOccurred, this,
                [this, name = q(track.name)](QMediaPlayer::Error, const QString &error) {
                    notify("Pista '" + name + "': " + error);
                });
        audioTrackOutputs_.push_back(std::move(output));
        audioTrackPlayers_.push_back(std::move(player));
        audioTrackPlaybackActive_.push_back(false);
    }
}
void EditorController::syncAudioTrackPlayback(int frame) {
    for (std::size_t i = 0; i < project().audioTracks.size() && i < audioTrackPlayers_.size(); ++i) {
        const auto &track = project().audioTracks[i];
        const bool shouldPlay = !track.muted && frame >= track.start && frame < track.start + track.duration;
        const auto localFrame = std::clamp<std::int64_t>(frame - track.start, 0, track.duration);
        double fade = 1.0;
        if (track.fadeInFrames > 0)
            fade = std::min(fade, double(localFrame) / track.fadeInFrames);
        if (track.fadeOutFrames > 0)
            fade = std::min(fade, double(track.duration - localFrame) / track.fadeOutFrames);
        audioTrackOutputs_[i]->setVolume(shouldPlay ? std::clamp(track.gain * .9 * fade, 0.0, 1.0) : 0);
        if (shouldPlay && !audioTrackPlaybackActive_[i]) {
            const auto offset = std::max<std::int64_t>(0, frame - track.start);
            audioTrackPlayers_[i]->setPosition(qRound64(double(offset) * 1000. / fps()));
            audioTrackPlayers_[i]->play();
            audioTrackPlaybackActive_[i] = true;
        } else if (!shouldPlay && audioTrackPlaybackActive_[i]) {
            audioTrackPlayers_[i]->pause();
            audioTrackPlaybackActive_[i] = false;
        }
    }
}
bool EditorController::open(QUrl url) {
    auto loaded = studio::loadProject(url.toLocalFile());
    if (!loaded.error.isEmpty()) {
        notify(loaded.error);
        return false;
    }
    reset(std::move(loaded.project), url.toLocalFile(), loaded.revision);
    notify("Projeto aberto.");
    emit projectAvailable(path_);
    return true;
}
void EditorController::newProject(bool demo) {
    auto p = demo ? studio::makeDemo() : studio::makeEmpty();
    p.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    reset(std::move(p));
    notify(demo ? "Demonstração carregada." : "Composição vazia criada.");
}
void EditorController::autosave() {
    if (!autosaveEnabled_)
        return;
    auto r = studio::saveProject(recoveryPath_, project(), recoveryRevision_);
    if (r.error.isEmpty()) {
        recoveryRevision_ = r.revision;
        autosaveEnabled_ = false;
        emit documentChanged();
    } else {
        notify("Recuperação automática: " + r.error);
        autosaveEnabled_ = false;
    }
}
void EditorController::recover() {
    auto r = studio::loadProject(recoveryPath_);
    if (!r.error.isEmpty()) {
        notify(r.error);
        return;
    }
    reset(std::move(r.project));
    savedDocument_.clear();
    emit documentChanged();
    notify("Cópia recuperada. Salve com um nome novo.");
}
void EditorController::exportVideo(QUrl url) {
    if (exporting())
        return;
    stop();
    auto base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    renderSnapshot_ = base + "/render-" + id + ".json";
    cancelPath_ = base + "/cancel-" + id;
    auto snapshot = studio::saveProject(renderSnapshot_, project(), {});
    if (!snapshot.error.isEmpty()) {
        notify(snapshot.error);
        return;
    }
    auto ffmpeg = qEnvironmentVariable("STUDIO_FFMPEG");
    if (!QFile::exists(ffmpeg)) {
        notify("FFmpeg não encontrado. Inicie pelo ABRIR-STUDIO.bat.");
        QFile::remove(renderSnapshot_);
        return;
    }
    renderBuffer_.clear();
    exportProgress_ = 0;
    render_.start(QCoreApplication::applicationDirPath() + "/studio_render.exe",
                  {"--project", renderSnapshot_, "--output", url.toLocalFile(), "--ffmpeg", ffmpeg,
                   "--cancel-file", cancelPath_});
    notify(hasAudio() ? "Exportando vídeo com narração e trilha. Você pode continuar editando."
                      : "Exportando composição sem áudio. Você pode continuar editando.");
    emit exportChanged();
}
void EditorController::cancelExport() {
    if (!exporting())
        return;
    QFile cancel(cancelPath_);
    if (cancel.open(QIODevice::WriteOnly)) {
        cancel.write("cancel");
        cancel.close();
        notify("Cancelando exportação…");
    }
}
QString EditorController::timecode(int f) const {
    int seconds = int(f / fps());
    return QString("%1:%2:%3")
        .arg(seconds / 60, 2, 10, QChar('0'))
        .arg(seconds % 60, 2, 10, QChar('0'))
        .arg(f - int(seconds * fps()), 2, 10, QChar('0'));
}
