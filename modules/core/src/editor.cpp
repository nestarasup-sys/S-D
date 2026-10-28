#include <stdexcept>
#include <algorithm>
#include <new>
#include <utility>
#include <studio/core/editor.hpp>

namespace studio {
namespace {
void synchronizeLinkedVideoAudio(Project &project) {
    for (auto &track : project.audioTracks) {
        if (track.linkedVideoNodeId.empty())
            continue;
        const auto *node = findNode(project, track.linkedVideoNodeId);
        if (!node || node->kind != NodeKind::Video) {
            track.linkedVideoNodeId.clear();
            continue;
        }
        track.source = node->source;
        track.start = node->start;
        track.duration = node->duration;
        track.sourceInSeconds = node->videoSourceInSeconds;
        track.playbackRate = node->videoPlaybackRate;
        track.fadeInFrames = std::min(track.fadeInFrames, track.duration);
        track.fadeOutFrames = std::min(track.fadeOutFrames, track.duration - track.fadeInFrames);
        std::erase_if(track.gainAutomation,
                      [&](const auto &point) { return point.frame >= track.duration; });
    }
}
} // namespace
Editor::Editor(Project project) {
    reset(std::move(project));
}
void Editor::reset(Project project) {
    if (auto error = validate(project); !error.empty())
        throw std::invalid_argument(error);
    project_ = std::move(project);
    past_.clear();
    future_.clear();
    gesturing_ = false;
    gestureChanged_ = false;
}
void Editor::remember(Project project) {
    if (past_.size() >= 100)
        past_.erase(past_.begin());
    past_.push_back(std::move(project));
}
std::string Editor::execute(const std::function<void(Project &)> &mutation) try {
    auto candidate = project_;
    mutation(candidate);
    synchronizeLinkedVideoAudio(candidate);
    if (auto error = validate(candidate); !error.empty())
        return error;
    if (!gesturing_)
        remember(project_);
    else
        gestureChanged_ = true;
    project_ = std::move(candidate);
    future_.clear();
    return {};
} catch (const std::bad_alloc &) {
    return "Memória insuficiente para validar e aplicar esta edição; o projeto atual foi preservado.";
}
void Editor::beginGesture() {
    if (gesturing_)
        return;
    gesturing_ = true;
    gestureChanged_ = false;
    gestureStart_ = project_;
}
void Editor::endGesture() {
    if (!gesturing_)
        return;
    if (gestureChanged_)
        remember(std::move(gestureStart_));
    gesturing_ = false;
    gestureChanged_ = false;
}
bool Editor::undo() try {
    endGesture();
    if (past_.empty())
        return false;
    future_.push_back(project_);
    project_ = std::move(past_.back());
    past_.pop_back();
    return true;
} catch (const std::bad_alloc &) {
    return false;
}
bool Editor::redo() try {
    endGesture();
    if (future_.empty())
        return false;
    remember(project_);
    project_ = std::move(future_.back());
    future_.pop_back();
    return true;
} catch (const std::bad_alloc &) {
    return false;
}
} // namespace studio
