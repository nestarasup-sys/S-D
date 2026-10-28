#include <QGuiApplication>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <studio/composition/text_layout.hpp>
#include <studio/composition/visual_quality.hpp>
#include <studio/core/project.hpp>

namespace {
void check(bool value, const char *why) {
    if (!value)
        throw std::runtime_error(why);
}
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    try {
        studio::Node fitted;
        fitted.kind = studio::NodeKind::Text;
        fitted.text = "Uma citação documental extensa em português deve reorganizar suas linhas sem perder nenhuma palavra original. O texto da IA continua completo e legível, enquanto o espaço interno do template é aproveitado com cuidado, mantendo a hierarquia visual e a leitura confortável em uma abertura cinematográfica.";
        fitted.width = 500;
        fitted.height = 300;
        fitted.fontSize = 54;
        fitted.textAutoFit = true;
        fitted.textMinFontSize = 18;
        fitted.textPadding = 4;
        fitted.role = "premium:quote:text";
        const auto layout = studio::resolvePremiumTextLayout(fitted, studio::ArtDirection{});
        check(layout.reduced && layout.pixelSize >= 18 && layout.fits,
              "premium text layout wraps and shrinks within its declared readable floor");
        fitted.text = QString(260, QLatin1Char('W')).toStdString();
        const auto overflow = studio::resolvePremiumTextLayout(fitted, studio::ArtDirection{});
        check(!overflow.fits && overflow.pixelSize == 18,
              "a long unbreakable word remains intact and reports overflow at the minimum size");

        auto project = studio::makeEmpty();
        project.width = 1280;
        project.height = 720;
        project.duration = 90;
        project.schemaVersion = 15;
        project.scenes = {{"scene-a", "Cena", "", 0, 90}};
        project.shots = {{"shot-a", "scene-a", "Composição", "cut", 0, 90}};
        studio::Node title;
        title.id = "title-outside";
        title.name = "Título principal";
        title.kind = studio::NodeKind::Text;
        title.role = "premium:title:main";
        title.text = "TÍTULO IMPORTANTE";
        title.x = 72;
        title.y = 48;
        title.width = 430;
        title.height = 75;
        title.fontSize = 56;
        title.animated = false;
        title.start = 0;
        title.duration = 90;
        studio::Node caption = title;
        caption.id = "caption-over-title";
        caption.name = "Legenda";
        caption.role = "caption";
        caption.text = "Uma legenda cobre o título";
        caption.x = 72;
        caption.y = 48;
        caption.width = 400;
        caption.height = 80;
        caption.fontSize = 28;
        auto offscreen = title;
        offscreen.id = "title-outside";
        offscreen.name = "Título fora do quadro";
        offscreen.x = -85;
        project.nodes = {offscreen, title, caption};
        project.camera = {{0, 640, 360, 1, 0, "linear", "shot-a", "title-outside", "spatial", .08, .16}};
        const auto report = studio::VisualQualityAnalyzer::analyze(project);
        const auto hasType = [&](const QString &type) {
            for (const auto &issue : report.issues)
                if (issue.type == type)
                    return true;
            return false;
        };
        check(hasType("text-clipped"), "visual analyzer reports essential text outside the canvas");
        check(hasType("text-caption-collision"), "visual analyzer reports captions covering essential text");
        check(hasType("camera-target-framing"), "visual analyzer checks the final camera target against safe bounds");
        check(report.toJson()["format"] == "studio-dark-visual-quality" && report.sampledFrames.size() > 0,
              "visual quality report is structured and includes sampled frames");

        auto clean = studio::makeEmpty();
        clean.duration = 30;
        studio::Node body;
        body.id = "body";
        body.name = "Texto";
        body.kind = studio::NodeKind::Text;
        body.text = "Uma frase legível";
        body.x = 220;
        body.y = 180;
        body.width = 500;
        body.height = 70;
        body.fontSize = 22;
        body.animated = false;
        body.duration = 30;
        clean.nodes = {body};
        const auto cleanReport = studio::VisualQualityAnalyzer::analyze(clean);
        check(cleanReport.issues.empty(), "plain legible scene does not receive speculative diagnostics");

        auto adapted = clean;
        fitted.id = "adapted-premium-quote";
        fitted.name = "Citação adaptada";
        fitted.text = "Uma citação documental extensa em português deve reorganizar suas linhas sem perder nenhuma palavra original. O texto da IA continua completo e legível, enquanto o espaço interno do template é aproveitado com cuidado, mantendo a hierarquia visual e a leitura confortável em uma abertura cinematográfica.";
        fitted.x = 60;
        fitted.y = 80;
        fitted.start = 0;
        fitted.duration = 30;
        fitted.opacity = 0;
        fitted.animated = false;
        fitted.motion = "none";
        fitted.keyframes = {{0, 0, 0, 0, 1, 0, "linear"}, {1, 0, 0, 0, 1, 1, "linear"}};
        adapted.nodes = {fitted};
        const auto adaptedReport = studio::VisualQualityAnalyzer::analyze(adapted);
        check(std::any_of(adaptedReport.issues.begin(), adaptedReport.issues.end(), [](const auto &issue) {
                  return issue.type == "text-adapted" && issue.severity == studio::VisualIssueSeverity::Information;
              }),
              "an adapted premium slot is reported once after it becomes visible, including when its start is hidden");

        auto revealProject = studio::makeEmpty();
        revealProject.duration = 30;
        revealProject.scenes = {{"reveal-scene", "Revelação", "", 0, 30}};
        revealProject.shots = {{"reveal-shot", "reveal-scene", "Reveal", "cut", 0, 30}};
        studio::Node reveal;
        reveal.id = "offscreen-reveal";
        reveal.name = "Alvo fora de tela";
        reveal.kind = studio::NodeKind::Shape;
        reveal.x = -5000;
        reveal.y = 160;
        reveal.width = 240;
        reveal.height = 120;
        reveal.opacity = 0;
        reveal.animated = false;
        reveal.motion = "none";
        reveal.start = 0;
        reveal.duration = 30;
        reveal.keyframes = {{10, reveal.x, reveal.y, 0, 1, 1, "linear"}};
        revealProject.nodes = {reveal};
        revealProject.visualBeats = {{"show-offscreen", "reveal-shot", "offscreen-reveal", "show", 10, 0}};
        const auto offscreenReveal = studio::VisualQualityAnalyzer::analyze(revealProject);
        check(std::any_of(offscreenReveal.issues.begin(), offscreenReveal.issues.end(), [](const auto &issue) {
                  return issue.type == "show-target-offscreen";
              }),
              "visual analysis warns when a correctly timed show is entirely outside the viewport");
        revealProject.nodes.front().x = 420;
        revealProject.nodes.front().keyframes.front().x = 420;
        const auto visibleReveal = studio::VisualQualityAnalyzer::analyze(revealProject);
        check(std::none_of(visibleReveal.issues.begin(), visibleReveal.issues.end(), [](const auto &issue) {
                  return issue.type == "show-target-offscreen";
              }),
              "an on-screen show event does not receive a speculative offscreen warning");
        std::cout << "Visual quality tests: all checks passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
