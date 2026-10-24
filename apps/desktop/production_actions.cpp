#include "editor_controller.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include <studio/core/library.hpp>
#include <studio/production/recipe.hpp>

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
            auto o = QJsonDocument::fromJson(line).object();
            productionProgress_ = o["progress"].toInt();
            if (!o["error"].toString().isEmpty())
                notify(o["error"].toString());
            else
                notify(o["stage"].toString());
            emit productionChanged();
        }
    });
    connect(
        &production_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int code, QProcess::ExitStatus status) {
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
                notify("Produção interrompida; projeto anterior preservado.");
            QFile::remove(productionCancel_);
            emit productionChanged();
        });
    connect(&production_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        notify("Falha ao iniciar produção: " + production_.errorString());
        emit productionChanged();
    });
}
void EditorController::importProduction(QUrl recipe) {
    if (producing() || exporting())
        return;
    QFile file(recipe.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        notify("Não foi possível abrir a receita.");
        return;
    }
    if (file.size() > 1024 * 1024) {
        notify("Receita excede 1 MiB.");
        return;
    }
    auto parsed = studio::parseRecipe(file.readAll(), QFileInfo(file).absolutePath());
    if (!parsed.error.isEmpty()) {
        notify(parsed.error);
        return;
    }
    stop();
    productionBaseline_ = studio::serialize(project());
    productionBuffer_.clear();
    productionProgress_ = 0;
    auto parent = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/productions";
    QDir().mkpath(parent);
    auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    productionDirectory_ = parent + "/" + id;
    productionCancel_ = parent + "/cancel-" + id;
    production_.start(QCoreApplication::applicationDirPath() + "/studio_produce.exe",
                      {"--recipe", recipe.toLocalFile(), "--output-dir", productionDirectory_,
                       "--cancel-file", productionCancel_});
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
        notify("Cancelando produção após o trecho atual de voz…");
    }
}
void EditorController::saveAiGuide(QUrl url) {
    QSaveFile file(url.toLocalFile());
    auto bytes = studio::aiGuide().toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        notify("Não foi possível salvar o guia.");
    else
        notify("Guia para IA salvo. Envie-o junto do seu roteiro à IA escolhida.");
}
