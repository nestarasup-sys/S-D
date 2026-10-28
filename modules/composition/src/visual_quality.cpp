#include <studio/composition/visual_quality.hpp>

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QJsonArray>
#include <QRectF>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <unordered_map>
#include <studio/composition/render_snapshot.hpp>
#include <studio/composition/text_layout.hpp>

namespace studio {
namespace {
QString severityName(VisualIssueSeverity severity) {
    switch (severity) {
    case VisualIssueSeverity::Information: return QStringLiteral("informativo");
    case VisualIssueSeverity::Warning: return QStringLiteral("aviso");
    case VisualIssueSeverity::Error: return QStringLiteral("erro-composicao");
    }
    return QStringLiteral("aviso");
}
bool essentialText(const Node &node) {
    return node.textAutoFit || node.role == "caption" || node.role == "header" ||
           node.role.rfind("premium:", 0) == 0 || node.fontSize >= 28;
}
bool decorativeRole(const Node &node) {
    return node.role == "background" || node.role == "kicker" || node.role == "accent-rule" ||
           node.role == "highlight-rule" || node.role == "quote-mark" ||
           node.role == "premium:quote:mark" || node.role == "decorative";
}
QRectF intersected(const QRectF &a, const QRectF &b) {
    return a.intersected(b);
}
double overlapRatio(const QRectF &a, const QRectF &b) {
    const auto overlap = intersected(a, b);
    const double area = std::max(0.0, overlap.width()) * std::max(0.0, overlap.height());
    const double smaller = std::min(a.width() * a.height(), b.width() * b.height());
    return smaller > 0 ? area / smaller : 0;
}
double luminance(const QColor &color) {
    const auto channel = [](double value) {
        value /= 255.0;
        return value <= .04045 ? value / 12.92 : std::pow((value + .055) / 1.055, 2.4);
    };
    return .2126 * channel(color.red()) + .7152 * channel(color.green()) + .0722 * channel(color.blue());
}
double contrastRatio(const QColor &a, const QColor &b) {
    const auto first = luminance(a), second = luminance(b);
    return (std::max(first, second) + .05) / (std::min(first, second) + .05);
}
bool enteringOrLeaving(const Node &node, std::int64_t frame) {
    if (node.animated && node.motion != "none" && frame - node.start < node.motionFrames)
        return true;
    return node.exitFrames > 0 && frame >= node.start + node.duration - node.exitFrames;
}
QString nodeLabel(const Node &node) {
    return QString::fromStdString(node.name.empty() ? node.id : node.name);
}
} // namespace

QJsonObject VisualQualityIssue::toJson() const {
    return {{"type", type}, {"severity", severityName(severity)}, {"sceneId", sceneId},
            {"shotId", shotId}, {"elementId", elementId}, {"firstFrame", qint64(firstFrame)},
            {"lastFrame", qint64(lastFrame)}, {"description", description}, {"evidence", evidence},
            {"suggestion", suggestion}};
}
QString VisualQualityIssue::displayText() const {
    QString location = shotId.isEmpty() ? sceneId : shotId;
    if (location.isEmpty())
        location = QStringLiteral("frame %1").arg(firstFrame);
    else
        location += QStringLiteral(" · frame %1").arg(firstFrame);
    if (!elementId.isEmpty())
        location += QStringLiteral(" · ") + elementId;
    return QStringLiteral("[%1] %2\n  %3\n  Evidência: %4\n  Sugestão: %5")
        .arg(severityName(severity), location, description, evidence, suggestion);
}
QJsonObject VisualQualityReport::toJson() const {
    QJsonArray frames, issueArray;
    for (const auto frame : sampledFrames)
        frames.append(qint64(frame));
    for (const auto &issue : issues)
        issueArray.append(issue.toJson());
    return {{"format", "studio-dark-visual-quality"}, {"version", 1}, {"project", projectName},
            {"canvas", QJsonObject{{"width", width}, {"height", height}}},
            {"sampledFrames", frames}, {"sampledFrameCount", int(sampledFrames.size())},
            {"issueCount", int(issues.size())}, {"issues", issueArray}};
}

VisualQualityReport VisualQualityAnalyzer::analyze(const Project &project) {
    VisualQualityReport report;
    report.projectName = QString::fromStdString(project.name);
    report.width = project.width;
    report.height = project.height;
    if (project.width <= 0 || project.height <= 0 || project.duration <= 0)
        return report;

    std::set<std::int64_t> frames{0, project.duration - 1};
    const auto addFrame = [&](std::int64_t frame) {
        if (frame >= 0 && frame < project.duration)
            frames.insert(frame);
    };
    for (const auto &shot : project.shots) {
        addFrame(shot.start);
        addFrame(shot.start + shot.duration / 2);
        addFrame(shot.start + shot.duration - 1);
    }
    for (const auto &node : project.nodes) {
        if (node.kind != NodeKind::Text)
            continue;
        addFrame(node.start);
        addFrame(node.start + std::min<std::int64_t>(node.motionFrames, node.duration - 1));
        addFrame(node.start + node.duration - 1);
    }
    for (const auto &beat : project.visualBeats) {
        if (beat.action == "show" || beat.action == "hide") {
            addFrame(beat.frame - 1);
            addFrame(beat.frame);
            addFrame(beat.frame + 1);
        } else
            addFrame(beat.frame);
    }
    for (std::size_t i = 0; i < project.camera.size(); ++i) {
        const auto &key = project.camera[i];
        addFrame(key.frame);
        if (i + 1 >= project.camera.size() || project.camera[i + 1].shotId != key.shotId)
            continue;
        const auto next = project.camera[i + 1].frame;
        for (int part = 1; part <= 3; ++part)
            addFrame(key.frame + (next - key.frame) * part / 4);
    }
    report.sampledFrames.assign(frames.begin(), frames.end());

    std::map<QString, std::size_t> issueIndex;
    std::set<std::string> textAdaptationReported;
    const auto addIssue = [&](QString type, VisualIssueSeverity severity, std::int64_t frame,
                              QString sceneId, QString shotId, QString elementId, QString description,
                              QString evidence, QString suggestion) {
        const auto key = type + QChar(0x1f) + shotId + QChar(0x1f) + elementId;
        const auto found = issueIndex.find(key);
        if (found != issueIndex.end()) {
            auto &existing = report.issues[found->second];
            existing.firstFrame = std::min(existing.firstFrame, frame);
            existing.lastFrame = std::max(existing.lastFrame, frame);
            if (int(severity) > int(existing.severity)) {
                existing.severity = severity;
                existing.description = std::move(description);
                existing.evidence = std::move(evidence);
                existing.suggestion = std::move(suggestion);
            }
            return;
        }
        issueIndex.emplace(key, report.issues.size());
        report.issues.push_back({std::move(type), severity, std::move(sceneId), std::move(shotId),
                                 std::move(elementId), frame, frame, std::move(description),
                                 std::move(evidence), std::move(suggestion)});
    };

    std::unordered_map<std::int64_t, RenderFrameSnapshot> snapshots;
    snapshots.reserve(frames.size());
    for (const auto frame : frames)
        snapshots.emplace(frame, RenderSnapshot::build(project, frame).primary());

    struct TextBox {
        const RenderNodeSnapshot *resolved{};
        QRectF inkBounds;
        bool important{false};
    };
    for (const auto &[frame, snapshot] : snapshots) {
        std::vector<TextBox> textBoxes;
        textBoxes.reserve(snapshot.nodes.size());
        const QRectF canvas(0, 0, project.width, project.height);
        const QRectF safe(double(snapshot.backgroundArt.safeMargin),
                          double(snapshot.backgroundArt.safeMargin),
                          std::max(1, project.width - 2 * snapshot.backgroundArt.safeMargin),
                          std::max(1, project.height - 2 * snapshot.backgroundArt.safeMargin));
        for (const auto &resolved : snapshot.nodes) {
            const auto &node = resolved.node;
            if (node.kind != NodeKind::Text || node.text.empty() || !resolved.visible ||
                resolved.inheritedOpacity < .05)
                continue;
            const auto textLayout = resolvePremiumTextLayout(node, resolved.artDirection);
            QFont font(QString::fromStdString(resolved.artDirection.fontFamily));
            font.setPixelSize(textLayout.pixelSize);
            font.setWeight(QFont::DemiBold);
            const auto alignment = node.role == "caption" ? Qt::AlignCenter : Qt::AlignLeft | Qt::AlignTop;
            QRectF content(0, 0, node.width, node.height);
            if (node.role == "caption")
                content = content.adjusted(20, 8, -20, -8);
            else if (node.textAutoFit)
                content = textLayout.contentBounds;
            const QString contentText = QString::fromStdString(node.text);
            auto ink = measureWrappedText(contentText, font, content, alignment).translated(content.topLeft());
            if (node.role == "caption")
                ink.translate(0, std::max(0.0, (content.height() - ink.height()) / 2.0));
            ink = resolved.sceneTransform.mapRect(ink);
            if (node.role != "caption")
                ink = snapshot.cameraTransform.mapRect(ink);
            if (ink.isEmpty())
                continue;
            const bool important = essentialText(node) && !decorativeRole(node);
            textBoxes.push_back({&resolved, ink, important});

            if (important && !textLayout.fits && !enteringOrLeaving(node, frame)) {
                addIssue(QStringLiteral("text-overflow"), VisualIssueSeverity::Warning, frame,
                         QString::fromStdString(snapshot.sceneId),
                         QString::fromStdString(node.shotId.empty() && snapshot.shotId
                                                     ? *snapshot.shotId : node.shotId),
                         QString::fromStdString(node.id),
                         QStringLiteral("O texto ‘%1’ não cabe na área definida.").arg(nodeLabel(node)),
                         QStringLiteral("Caixa %1×%2; texto medido %3×%4 px; fonte %5 px (mínimo %6 px).")
                             .arg(node.width, 0, 'f', 0).arg(node.height, 0, 'f', 0)
                             .arg(textLayout.measuredBounds.width(), 0, 'f', 1)
                             .arg(textLayout.measuredBounds.height(), 0, 'f', 1)
                             .arg(textLayout.pixelSize).arg(node.textAutoFit ? node.textMinFontSize : 12),
                         QStringLiteral("Aumente a área do slot ou encurte o texto manualmente; nenhuma palavra foi removida."));
            } else if (node.textAutoFit && textLayout.reduced &&
                       textAdaptationReported.insert(node.id).second) {
                addIssue(QStringLiteral("text-adapted"), VisualIssueSeverity::Information, frame,
                         QString::fromStdString(snapshot.sceneId),
                         QString::fromStdString(node.shotId.empty() && snapshot.shotId
                                                     ? *snapshot.shotId : node.shotId),
                         QString::fromStdString(node.id),
                         QStringLiteral("O texto ‘%1’ foi reduzido para caber no slot.").arg(nodeLabel(node)),
                         QStringLiteral("Fonte solicitada %1 px; fonte usada %2 px.")
                             .arg(node.fontSize, 0, 'f', 1).arg(textLayout.pixelSize),
                         QStringLiteral("Confira a leitura no preview, especialmente em retrato."));
            }
            if (important && !enteringOrLeaving(node, frame) && resolved.inheritedOpacity > .9) {
                const QRectF visibleText = ink.intersected(canvas);
                const double ratio = ink.width() * ink.height() > 0
                                         ? visibleText.width() * visibleText.height() / (ink.width() * ink.height())
                                         : 1;
                if (ratio < .97) {
                    addIssue(QStringLiteral("text-clipped"), VisualIssueSeverity::Error, frame,
                             QString::fromStdString(snapshot.sceneId),
                             QString::fromStdString(node.shotId.empty() && snapshot.shotId
                                                         ? *snapshot.shotId : node.shotId),
                             QString::fromStdString(node.id),
                             QStringLiteral("O texto ‘%1’ ultrapassa o quadro.").arg(nodeLabel(node)),
                             QStringLiteral("%1% da área medida está dentro do canvas (%2×%3).")
                                 .arg(qRound(ratio * 100)).arg(project.width).arg(project.height),
                             QStringLiteral("Reposicione ou reduza o elemento, respeitando a câmera e a área de legendas."));
                }
                const bool safeZoneText = node.textAutoFit || node.role == "header" ||
                                          node.role.find(":title:") != std::string::npos ||
                                          node.role.find(":quote:") != std::string::npos;
                if (safeZoneText && !safe.contains(ink)) {
                    addIssue(QStringLiteral("safe-area"), VisualIssueSeverity::Warning, frame,
                             QString::fromStdString(snapshot.sceneId),
                             QString::fromStdString(node.shotId.empty() && snapshot.shotId
                                                         ? *snapshot.shotId : node.shotId),
                             QString::fromStdString(node.id),
                             QStringLiteral("O texto ‘%1’ invade a margem segura.").arg(nodeLabel(node)),
                             QStringLiteral("Margem definida: %1 px; texto em (%2,%3)–(%4,%5).")
                                 .arg(snapshot.backgroundArt.safeMargin).arg(ink.left(), 0, 'f', 0)
                                 .arg(ink.top(), 0, 'f', 0).arg(ink.right(), 0, 'f', 0).arg(ink.bottom(), 0, 'f', 0),
                             QStringLiteral("Afaste o texto da borda ou reveja a margem de arte do shot."));
                }
                if (!node.color.empty() || node.role == "caption") {
                    const QColor foreground(QString::fromStdString(
                        node.role == "caption" ? resolved.artDirection.textColor : node.color));
                    const RenderNodeSnapshot *background = nullptr;
                    for (const auto &candidate : snapshot.nodes) {
                        if (candidate.drawOrder >= resolved.drawOrder || candidate.node.kind != NodeKind::Shape ||
                            candidate.inheritedOpacity < .8 || !candidate.sceneBounds.contains(ink.center()))
                            continue;
                        if (!background || candidate.drawOrder > background->drawOrder)
                            background = &candidate;
                    }
                    QColor backgroundColor;
                    if (node.role == "caption")
                        backgroundColor = QColor(QString::fromStdString(resolved.artDirection.captionBackgroundColor));
                    else if (background)
                        backgroundColor = QColor(QString::fromStdString(background->node.color));
                    if (foreground.isValid() && backgroundColor.isValid()) {
                        const double ratio = contrastRatio(foreground, backgroundColor);
                        if (ratio < 3.0)
                            addIssue(QStringLiteral("low-contrast"), VisualIssueSeverity::Warning, frame,
                                     QString::fromStdString(snapshot.sceneId),
                                     QString::fromStdString(node.shotId.empty() && snapshot.shotId
                                                                 ? *snapshot.shotId : node.shotId),
                                     QString::fromStdString(node.id),
                                     QStringLiteral("O contraste do texto ‘%1’ pode ser insuficiente.").arg(nodeLabel(node)),
                                     QStringLiteral("Contraste medido entre texto e a superfície sólida imediatamente atrás: %1:1.")
                                         .arg(ratio, 0, 'f', 2),
                                     QStringLiteral("Aumente a diferença de luminosidade; fundos com imagem ou gradiente não são inferidos."));
                    }
                }
            }
        }
        for (std::size_t i = 0; i < textBoxes.size(); ++i) {
            const auto &first = textBoxes[i];
            if (!first.important || first.resolved->inheritedOpacity < .9 ||
                enteringOrLeaving(first.resolved->node, frame))
                continue;
            for (std::size_t j = i + 1; j < textBoxes.size(); ++j) {
                const auto &second = textBoxes[j];
                if ((!second.important && second.resolved->node.role != "caption") ||
                    second.resolved->inheritedOpacity < .9 || enteringOrLeaving(second.resolved->node, frame))
                    continue;
                const double ratio = overlapRatio(first.inkBounds, second.inkBounds);
                if (ratio < .22)
                    continue;
                const auto &a = first.resolved->node;
                const auto &b = second.resolved->node;
                const auto caption = a.role == "caption" ? &a : b.role == "caption" ? &b : nullptr;
                addIssue(caption ? QStringLiteral("text-caption-collision") : QStringLiteral("text-collision"),
                         VisualIssueSeverity::Warning, frame, QString::fromStdString(snapshot.sceneId),
                         QString::fromStdString(snapshot.shotId.value_or(a.shotId)),
                         QString::fromStdString(caption ? (caption == &a ? b.id : a.id) : a.id + "," + b.id),
                         caption ? QStringLiteral("Uma legenda cobre texto importante.")
                                 : QStringLiteral("Dois textos importantes se sobrepõem."),
                         QStringLiteral("As caixas visíveis se sobrepõem em %1% da menor área de texto.")
                             .arg(qRound(ratio * 100)),
                         QStringLiteral("Mova ou redimensione um dos textos, ou ajuste o shot para evitar a faixa de legendas."));
            }
        }
    }

    // A show beat is checked immediately before and after its declared frame.
    for (const auto &beat : project.visualBeats) {
        if (beat.action != "show")
            continue;
        const auto target = std::find_if(project.nodes.begin(), project.nodes.end(),
                                         [&](const auto &node) { return node.id == beat.targetId; });
        if (target == project.nodes.end())
            continue;
        auto visibleAt = [&](std::int64_t frame) {
            const auto snapshot = snapshots.find(frame);
            if (snapshot == snapshots.end())
                return false;
            const auto resolved = std::find_if(snapshot->second.nodes.begin(), snapshot->second.nodes.end(),
                                               [&](const auto &node) { return node.node.id == beat.targetId; });
            return resolved != snapshot->second.nodes.end() && resolved->visible && resolved->inheritedOpacity > .01;
        };
        const bool before = beat.frame > 0 && visibleAt(beat.frame - 1);
        const bool now = visibleAt(beat.frame);
        const bool after = beat.frame + 1 < project.duration ? visibleAt(beat.frame + 1) : now;
        if (before || !now || !after) {
            QString reason = before ? QStringLiteral("já está visível antes do beat")
                             : !now ? QStringLiteral("não está visível no frame do beat")
                                    : QStringLiteral("desaparece logo após o beat");
            addIssue(QStringLiteral("show-beat-visibility"), before || !now ? VisualIssueSeverity::Error
                                                                             : VisualIssueSeverity::Warning,
                     beat.frame, QString::fromStdString(target->sceneId), QString::fromStdString(beat.shotId),
                     QString::fromStdString(target->id),
                     QStringLiteral("O alvo ‘%1’ %2.").arg(nodeLabel(*target), reason),
                     QStringLiteral("Visibilidade: frame anterior=%1, frame do evento=%2, frame seguinte=%3.")
                         .arg(before ? "sim" : "não", now ? "sim" : "não", after ? "sim" : "não"),
                     QStringLiteral("Confira a opacidade inicial, a duração do elemento e a ordem temporal do beat."));
        } else if (beat.action == "show") {
            const auto snapshot = snapshots.find(beat.frame);
            if (snapshot != snapshots.end()) {
                const auto resolved = std::find_if(snapshot->second.nodes.begin(), snapshot->second.nodes.end(),
                                                   [&](const auto &node) { return node.node.id == beat.targetId; });
                if (resolved != snapshot->second.nodes.end()) {
                    QRectF bounds = resolved->sceneBounds;
                    if (target->kind == NodeKind::Group)
                        for (const auto &child : snapshot->second.nodes)
                            if (child.node.id != target->id &&
                                child.hierarchy.size() > resolved->hierarchy.size() &&
                                std::equal(resolved->hierarchy.begin(), resolved->hierarchy.end(),
                                           child.hierarchy.begin()) && child.visible &&
                                child.inheritedOpacity > .05)
                                bounds = bounds.united(child.sceneBounds);
                    const QRectF viewport(0, 0, project.width, project.height);
                    if (!bounds.intersects(viewport))
                        addIssue(QStringLiteral("show-target-offscreen"), VisualIssueSeverity::Warning,
                                 beat.frame, QString::fromStdString(target->sceneId),
                                 QString::fromStdString(beat.shotId), QString::fromStdString(target->id),
                                 QStringLiteral("O alvo ‘%1’ aparece fora da tela no frame do Visual Beat.")
                                     .arg(nodeLabel(*target)),
                                 QStringLiteral("Bounds transformados: (%1,%2)–(%3,%4); canvas %5×%6.")
                                     .arg(bounds.left(), 0, 'f', 1).arg(bounds.top(), 0, 'f', 1)
                                     .arg(bounds.right(), 0, 'f', 1).arg(bounds.bottom(), 0, 'f', 1)
                                     .arg(project.width).arg(project.height),
                                 QStringLiteral("Mova o beat para um enquadramento visível ou mantenha-o apenas se a revelação fora de tela for intencional."));
                }
            }
        }
    }

    // camera.path provenance lets the report identify the actual intended object.
    for (const auto &key : project.camera) {
        if (key.targetId.empty())
            continue;
        const auto target = std::find_if(project.nodes.begin(), project.nodes.end(),
                                         [&](const auto &node) { return node.id == key.targetId; });
        if (target == project.nodes.end())
            continue;
        std::int64_t checkFrame = key.frame;
        if (key.revealMode == "temporal" || key.revealMode == "combined")
            for (const auto &beat : project.visualBeats)
                if (beat.shotId == key.shotId && beat.targetId == key.targetId && beat.action == "show") {
                    checkFrame = beat.frame;
                    break;
                }
        const auto snapshot = snapshots.find(checkFrame);
        if (snapshot == snapshots.end())
            continue;
        const auto resolved = std::find_if(snapshot->second.nodes.begin(), snapshot->second.nodes.end(),
                                           [&](const auto &node) { return node.node.id == key.targetId; });
        if (resolved == snapshot->second.nodes.end() || !resolved->visible || resolved->inheritedOpacity < .05) {
            addIssue(QStringLiteral("camera-target-hidden"), VisualIssueSeverity::Error, checkFrame,
                     QString::fromStdString(target->sceneId), QString::fromStdString(key.shotId),
                     QString::fromStdString(key.targetId),
                     QStringLiteral("O alvo do travelling ‘%1’ não está visível no enquadramento planejado.")
                         .arg(nodeLabel(*target)),
                     QStringLiteral("Camera.path declara reveal=%1 para o alvo %2.")
                         .arg(QString::fromStdString(key.revealMode), QString::fromStdString(key.targetId)),
                     QStringLiteral("Confira o beat show, a visibilidade do grupo ancestral e o intervalo do elemento."));
            continue;
        }
        QRectF bounds = resolved->sceneBounds;
        if (target->kind == NodeKind::Group) {
            for (const auto &child : snapshot->second.nodes)
                if (child.node.id != key.targetId && child.hierarchy.size() > resolved->hierarchy.size() &&
                    std::equal(resolved->hierarchy.begin(), resolved->hierarchy.end(), child.hierarchy.begin()) &&
                    child.visible && child.inheritedOpacity > .05)
                    bounds = bounds.united(child.sceneBounds);
        }
        const QRectF safe(key.safeMargin * project.width,
                          key.safeMargin * project.height,
                          project.width * (1 - 2 * key.safeMargin),
                          project.height * (1 - 2 * key.safeMargin - key.captionSafeBottom));
        const auto inside = bounds.intersected(safe);
        const double area = bounds.width() * bounds.height();
        const double ratio = area > 0 ? inside.width() * inside.height() / area : 0;
        if (!safe.contains(bounds) || ratio < .78)
            addIssue(QStringLiteral("camera-target-framing"), VisualIssueSeverity::Warning, checkFrame,
                     QString::fromStdString(target->sceneId), QString::fromStdString(key.shotId),
                     QString::fromStdString(key.targetId),
                     QStringLiteral("O alvo ‘%1’ fica parcialmente fora da área segura no fim do travelling.")
                         .arg(nodeLabel(*target)),
                     QStringLiteral("Área visível do alvo: %1%; margem %2%, faixa inferior reservada %3%.")
                         .arg(qRound(ratio * 100)).arg(qRound(key.safeMargin * 100))
                         .arg(qRound(key.captionSafeBottom * 100)),
                     QStringLiteral("Ajuste padding do alvo ou amplie o enquadramento final."));
    }

    return report;
}
} // namespace studio
