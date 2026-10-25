#include "editor_controller.hpp"
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>
#include <algorithm>
#include <studio/core/library.hpp>
#include <studio/production/recipe.hpp>
#include <studio/storage/workspace_store.hpp>

void EditorController::startProductionLog(const QString &recipeFileName) {
    productionLogPath_.clear();
    if (!QDir().mkpath(diagnosticsDirectory_))
        return;
    // Keep at most 30 recent attempts. Logs never contain the full JSON or spoken text.
    QDir directory(diagnosticsDirectory_);
    const auto old = directory.entryInfoList({"production-*.jsonl"}, QDir::Files, QDir::Time);
    for (int i = 29; i < old.size(); ++i)
        QFile::remove(old[i].absoluteFilePath());
    productionLogPath_ =
        directory.filePath("production-" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz") +
                           "-" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8) + ".jsonl");
    logProductionEvent("import-start", QFileInfo(recipeFileName).fileName());
    emit productionChanged();
}
void EditorController::logProductionEvent(const QString &event, const QString &message, int progress) {
    if (productionLogPath_.isEmpty())
        return;
    QFile file(productionLogPath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
        return;
    // One JSON object per line; no source JSON, narration, WAV data, or entire file paths.
    QJsonObject entry{{"format", "studio-dark-diagnostic"},
                      {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
                      {"event", event.left(80)}};
    if (!message.isEmpty())
        entry["detail"] = message.left(2000);
    if (progress >= 0)
        entry["progress"] = progress;
    file.write(QJsonDocument(entry).toJson(QJsonDocument::Compact));
    file.write("\n");
}
bool EditorController::openLogsFolder() {
    if (!QDir().mkpath(diagnosticsDirectory_)) {
        notify("Não foi possível criar a pasta de diagnósticos.");
        return false;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(diagnosticsDirectory_))) {
        notify("Não foi possível abrir a pasta de logs: " + diagnosticsDirectory_);
        return false;
    }
    notify("Pasta de logs aberta. Envie o JSONL da tentativa que falhou para diagnóstico.");
    return true;
}
QString EditorController::currentSceneId() const {
    for (const auto &s : project().scenes)
        if (frame_ >= s.start && frame_ < s.start + s.duration)
            return QString::fromStdString(s.id);
    return {};
}
QString EditorController::currentShotId() const {
    for (const auto &shot : project().shots)
        if (frame_ >= shot.start && frame_ < shot.start + shot.duration)
            return QString::fromStdString(shot.id);
    return {};
}
QVariantList EditorController::library() const {
    QVariantList list;
    for (const auto &a : studio::nativeLibrary())
        list.append(QVariantMap{{"id", QString::fromStdString(a.id)},
                                {"name", QString::fromStdString(a.name)},
                                {"category", QString::fromStdString(a.category)}});
    return list;
}
QVariantList EditorController::scenes() const {
    QVariantList list;
    for (const auto &s : project().scenes)
        list.append(QVariantMap{{"id", QString::fromStdString(s.id)},
                                {"name", QString::fromStdString(s.name)},
                                {"start", int(s.start)},
                                {"duration", int(s.duration)}});
    return list;
}
QVariantList EditorController::shots() const {
    QVariantList list;
    for (const auto &shot : project().shots) {
        const bool hasPrevious =
            std::any_of(project().shots.begin(), project().shots.end(), [&](const auto &other) {
                return other.sceneId == shot.sceneId && other.start < shot.start;
            });
        list.append(QVariantMap{{"id", QString::fromStdString(shot.id)},
                                {"sceneId", QString::fromStdString(shot.sceneId)},
                                {"name", QString::fromStdString(shot.name)},
                                {"transitionIn", QString::fromStdString(shot.transitionIn)},
                                {"transitionDurationFrames", shot.transitionDurationFrames},
                                {"transitionDirection", QString::fromStdString(shot.transitionDirection)},
                                {"hasPrevious", hasPrevious},
                                {"start", int(shot.start)},
                                {"duration", int(shot.duration)}});
    }
    return list;
}
QVariantList EditorController::visualBeats() const {
    QVariantList list;
    for (const auto &beat : project().visualBeats)
        list.append(QVariantMap{{"id", QString::fromStdString(beat.id)},
                                {"shotId", QString::fromStdString(beat.shotId)},
                                {"targetId", QString::fromStdString(beat.targetId)},
                                {"action", QString::fromStdString(beat.action)},
                                {"frame", int(beat.frame)},
                                {"durationFrames", int(beat.durationFrames)}});
    return list;
}
QVariantList EditorController::audioTracks() const {
    QVariantList list;
    for (const auto &track : project().audioTracks)
        list.append(QVariantMap{{"id", QString::fromStdString(track.id)},
                                {"name", QString::fromStdString(track.name)},
                                {"source", QFileInfo(QString::fromStdString(track.source)).fileName()},
                                {"start", int(track.start)},
                                {"duration", int(track.duration)},
                                {"gain", track.gain},
                                {"muted", track.muted},
                                {"duckUnderVoice", track.duckUnderVoice},
                                {"duckAmount", track.duckAmount}});
    return list;
}
void EditorController::addAudioTrack(QUrl source) {
    const auto path = source.toLocalFile();
    const QFileInfo file(path);
    const auto extension = file.suffix().toLower();
    if (!file.isFile() || !QFile::exists(path) ||
        !QStringList{"wav", "mp3", "m4a", "aac", "ogg", "flac", "wma"}.contains(extension)) {
        notify("Escolha um arquivo de áudio local WAV, MP3, M4A, AAC, OGG, FLAC ou WMA.");
        return;
    }
    if (project().audioTracks.size() >= 32) {
        notify("O projeto aceita até 32 pistas de áudio.");
        return;
    }
    stop();
    studio::AudioTrack track;
    track.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    track.name = file.completeBaseName().toStdString();
    track.source = file.absoluteFilePath().toStdString();
    track.start = frame_;
    track.duration = duration() - frame_;
    track.duckUnderVoice = hasMainAudio();
    const auto error = editor_.execute([&](studio::Project &p) {
        p.schemaVersion = 4;
        p.audioTracks.push_back(track);
    });
    if (!error.empty()) {
        notify(QString::fromStdString(error));
        return;
    }
    refreshAudioTracks();
    changed();
    notify("Pista adicionada: " + file.fileName());
}
void EditorController::removeAudioTrack(QString id) {
    stop();
    const auto error = editor_.execute([&](studio::Project &p) {
        std::erase_if(p.audioTracks, [&](const auto &track) { return track.id == id.toStdString(); });
    });
    if (!error.empty()) {
        notify(QString::fromStdString(error));
        return;
    }
    for (std::size_t i = 0; i < project().audioTracks.size() && i < audioTrackOutputs_.size(); ++i)
        if (QString::fromStdString(project().audioTracks[i].id) == id)
            audioTrackOutputs_[i]->setVolume(project().audioTracks[i].muted
                                                 ? 0
                                                 : std::clamp(project().audioTracks[i].gain * .9, 0.0, 1.0));
    changed();
}
void EditorController::setAudioTrackGain(QString id, double gain) {
    if (!std::isfinite(gain) || gain < 0 || gain > 4) {
        notify("O ganho da pista precisa ficar entre 0 e 4.");
        return;
    }
    const auto error = editor_.execute([&](studio::Project &p) {
        for (auto &track : p.audioTracks)
            if (track.id == id.toStdString())
                track.gain = gain;
    });
    if (!error.empty()) {
        notify(QString::fromStdString(error));
        return;
    }
    refreshAudioTracks();
    changed();
}
void EditorController::setAudioTrackDucking(QString id, bool enabled) {
    if (enabled && !hasMainAudio() && project().voiceDuckingSource.empty()) {
        notify("Ducking precisa de uma faixa de voz no projeto.");
        return;
    }
    const auto error = editor_.execute([&](studio::Project &p) {
        for (auto &track : p.audioTracks)
            if (track.id == id.toStdString())
                track.duckUnderVoice = enabled;
    });
    if (!error.empty()) {
        notify(QString::fromStdString(error));
        return;
    }
    changed();
}
void EditorController::setAudioTrackMuted(QString id, bool muted) {
    stop();
    const auto error = editor_.execute([&](studio::Project &p) {
        for (auto &track : p.audioTracks)
            if (track.id == id.toStdString())
                track.muted = muted;
    });
    if (!error.empty()) {
        notify(QString::fromStdString(error));
        return;
    }
    refreshAudioTracks();
    changed();
}
void EditorController::setAudioTrackTiming(QString id, int start, int length) {
    if (duration() < 1) return;
    start = std::clamp(start, 0, duration() - 1);
    length = std::clamp(length, 1, duration() - start);
    stop();
    const auto error = editor_.execute([&](studio::Project &p) {
        for (auto &track : p.audioTracks)
            if (track.id == id.toStdString()) {
                track.start = start;
                track.duration = length;
            }
    });
    if (!error.empty()) {
        notify(QString::fromStdString(error));
        return;
    }
    refreshAudioTracks();
    changed();
}
void EditorController::setupProduction() {
    connect(&production_, &QProcess::readyReadStandardOutput, this, [this] {
        productionBuffer_ += production_.readAllStandardOutput();
        while (productionBuffer_.contains('\n')) {
            auto at = productionBuffer_.indexOf('\n');
            auto line = productionBuffer_.left(at);
            productionBuffer_.remove(0, at + 1);
            const auto doc = QJsonDocument::fromJson(line);
            if (!doc.isObject()) {
                logProductionEvent("worker-protocol-error", "Resposta de progresso inválida.");
                continue;
            }
            const auto o = doc.object();
            const auto stage = o["stage"].toString();
            const auto error = o["error"].toString();
            productionProgress_ = o["progress"].toInt();
            logProductionEvent(error.isEmpty() ? "worker-stage" : "worker-error",
                               error.isEmpty() ? stage : error, productionProgress_);
            if (!error.isEmpty())
                notify(error + " (veja Logs)");
            else if (!stage.isEmpty())
                notify(stage);
            emit productionChanged();
        }
    });
    connect(
        &production_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int code, QProcess::ExitStatus status) {
            logProductionEvent(code == 0 && status == QProcess::NormalExit ? "completed" : "failed",
                               QString("worker exitCode=%1 exitStatus=%2")
                                   .arg(code)
                                   .arg(status == QProcess::NormalExit ? "normal" : "crash"),
                               productionProgress_);
            if (code == 0 && status == QProcess::NormalExit) {
                latestProduction_ = QUrl::fromLocalFile(productionDirectory_ + "/project.studio.json");
                if (productionBaseline_ == studio::serialize(project())) {
                    open(latestProduction_);
                    notify(
                        "Vídeo montado com voz, legendas e objetos editáveis. Reproduza ou exporte o MP4.");
                } else
                    notify("Produção pronta. Sua edição atual foi preservada; use Abrir resultado para ver o "
                           "vídeo gerado.");
            } else if (status == QProcess::CrashExit)
                notify("Produção interrompida; projeto anterior preservado. Veja Logs.");
            else
                notify("A produção não terminou com sucesso. Veja Logs.");
            QFile::remove(productionCancel_);
            emit productionChanged();
        });
    connect(&production_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        logProductionEvent("process-error", production_.errorString());
        notify("Falha ao iniciar produção: " + production_.errorString() + " (veja Logs)");
        emit productionChanged();
    });
}
void EditorController::importProduction(QUrl recipe) {
    if (producing() || exporting())
        return;
    startProductionLog(recipe.toLocalFile());
    QFile file(recipe.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        logProductionEvent("preflight-error", "Não foi possível abrir o arquivo JSON.");
        notify("Não foi possível abrir a receita. Veja Logs.");
        return;
    }
    if (file.size() > 1024 * 1024) {
        logProductionEvent("preflight-error", "Receita excede 1 MiB.");
        notify("Receita excede 1 MiB. Veja Logs.");
        return;
    }
    const auto workspaceRoot =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/workspace";
    studio::WorkspaceStore workspace(workspaceRoot);
    if (!workspace.error().isEmpty()) {
        logProductionEvent("preflight-error", workspace.error());
        notify(workspace.error() + " (veja Logs)");
        return;
    }
    auto parsed = studio::parseRecipe(file.readAll(), QFileInfo(file).absolutePath(), workspace.data());
    if (!parsed.error.isEmpty()) {
        logProductionEvent("recipe-invalid", parsed.error);
        notify(parsed.error + " (veja Logs)");
        return;
    }
    logProductionEvent(
        "recipe-valid",
        QString("version=%1, scenes=%2").arg(parsed.recipe.version).arg(parsed.recipe.scenes.size()));
    stop();
    productionBaseline_ = studio::serialize(project());
    productionBuffer_.clear();
    productionProgress_ = 0;
    auto parent = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/productions";
    QDir().mkpath(parent);
    auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    productionDirectory_ = parent + "/" + id;
    productionCancel_ = parent + "/cancel-" + id;
    logProductionEvent("worker-start", "Iniciando synthesis e montagem.");
    production_.start(QCoreApplication::applicationDirPath() + "/studio_produce.exe",
                      {"--recipe", recipe.toLocalFile(), "--output-dir", productionDirectory_,
                       "--cancel-file", productionCancel_, "--workspace-root", workspaceRoot});
    notify("Preparando produção com voz local…");
    emit productionChanged();
}
void EditorController::importCarExample() {
    if (producing() || exporting())
        return;
    auto directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/examples";
    QDir().mkpath(directory);
    QFile resource(":/examples/carros-caros.production.json");
    if (!resource.open(QIODevice::ReadOnly)) {
        notify("Exemplo indisponível.");
        return;
    }
    auto path = directory + "/carros-caros.production.json";
    QSaveFile file(path);
    auto bytes = resource.readAll();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        notify("Falha ao preparar exemplo.");
        return;
    }
    importProduction(QUrl::fromLocalFile(path));
}
void EditorController::cancelProduction() {
    if (!producing())
        return;
    QFile file(productionCancel_);
    if (file.open(QIODevice::WriteOnly)) {
        file.write("cancel");
        file.close();
        logProductionEvent("cancel-requested");
        notify("Cancelando produção após o trecho atual de voz…");
    }
}
