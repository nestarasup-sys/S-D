#include "editor_controller.hpp"
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
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
    productionLogPath_ = directory.filePath(
        "production-" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz") + "-" +
        QUuid::createUuid().toString(QUuid::WithoutBraces).left(8) + ".jsonl");
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
                                   .arg(code).arg(status == QProcess::NormalExit ? "normal" : "crash"),
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
    logProductionEvent("recipe-valid", QString("version=%1, scenes=%2")
                           .arg(parsed.recipe.version).arg(parsed.recipe.scenes.size()));
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
