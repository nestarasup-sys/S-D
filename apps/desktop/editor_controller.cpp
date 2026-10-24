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

namespace {
QString q(const std::string &s) {
    return QString::fromStdString(s);
}
QVariantMap map(const studio::Node &n) {
    return {{"id", q(n.id)},
            {"name", q(n.name)},
            {"kind", n.kind == studio::NodeKind::Text     ? "text"
                     : n.kind == studio::NodeKind::Shape  ? "shape"
                     : n.kind == studio::NodeKind::Native ? "native"
                     : n.kind == studio::NodeKind::Svg    ? "svg"
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
        const auto next = hasAudio() ? int(audioPlayer_.position() * fps() / 1000.)
                                     : playStart_ + int(clock_.elapsed() * fps() / 1000.);
        if (hasAudio() && audioPlayer_.mediaStatus() == QMediaPlayer::EndOfMedia) {
            presentFrame(duration() - 1);
            stop();
            return;
        }
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
    return n ? map(*n) : QVariantMap{};
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
}
void EditorController::selectNode(QString id) {
    selectedId_ = std::move(id);
    emit documentChanged();
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
    if (before.isEmpty() || !before.contains(key) || before[key] == value)
        return;
    if ((key == "start" || key == "duration") &&
        (!std::isfinite(value.toDouble()) || std::floor(value.toDouble()) != value.toDouble())) {
        notify("Tempos devem ser números inteiros de frames.");
        return;
    }
    auto error = editor_.execute([&](studio::Project &p) {
        for (auto &n : p.nodes)
            if (q(n.id) == selectedId_) {
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
    if (!n || (n->x == x && n->y == y))
        return;
    auto error = editor_.execute([&](studio::Project &p) {
        for (auto &node : p.nodes)
            if (q(node.id) == selectedId_) {
                node.x = x;
                node.y = y;
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
    int lower = 0, upper = duration() - int(node->duration);
    for (const auto &scene : project().scenes)
        if (scene.id == node->sceneId) {
            lower = int(scene.start);
            upper = int(scene.start + scene.duration - node->duration);
        }
    start = std::clamp(start, lower, upper);
    if (node->start == start)
        return;
    auto error = editor_.execute([&](studio::Project &p) {
        for (auto &n : p.nodes)
            if (q(n.id) == id)
                n.start = start;
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
    auto error = editor_.execute([&](studio::Project &p) {
        if (n.role == "background")
            p.nodes.insert(p.nodes.begin(), n);
        else
            p.nodes.push_back(n);
    });
    if (error.empty()) {
        selectedId_ = q(n.id);
        changed();
        notify("Objeto adicionado. Edite no inspetor ou arraste na prévia.");
    } else
        notify(q(error));
}
void EditorController::removeSelected() {
    if (selected().isEmpty())
        return;
    editor_.execute([&](studio::Project &p) {
        std::erase_if(p.nodes, [&](const studio::Node &n) { return q(n.id) == selectedId_; });
    });
    selectedId_.clear();
    changed();
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
    if (oldScene != currentSceneId()) {
        selectedId_.clear();
        emit documentChanged();
    }
}
void EditorController::stop() {
    playing_ = false;
    playback_.stop();
    audioPlayer_.pause();
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
    if (hasAudio()) {
        if (!QFile::exists(q(project().audioSource))) {
            playing_ = false;
            notify("Arquivo de áudio ausente.");
            return;
        }
        audioPlayer_.setPosition(qRound64(frame_ * 1000. / fps()));
        audioPlayer_.play();
    }
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
}
void EditorController::open(QUrl url) {
    auto loaded = studio::loadProject(url.toLocalFile());
    if (!loaded.error.isEmpty()) {
        notify(loaded.error);
        return;
    }
    reset(std::move(loaded.project), url.toLocalFile(), loaded.revision);
    notify("Projeto aberto.");
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
