#include <stdexcept>
#include <studio/core/editor.hpp>
#include <utility>

namespace studio {
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
std::string Editor::execute(const std::function<void(Project &)> &mutation) {
    auto candidate = project_;
    mutation(candidate);
    if (auto error = validate(candidate); !error.empty())
        return error;
    if (!gesturing_)
        remember(project_);
    else
        gestureChanged_ = true;
    project_ = std::move(candidate);
    future_.clear();
    return {};
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
bool Editor::undo() {
    endGesture();
    if (past_.empty())
        return false;
    future_.push_back(project_);
    project_ = std::move(past_.back());
    past_.pop_back();
    return true;
}
bool Editor::redo() {
    endGesture();
    if (future_.empty())
        return false;
    remember(project_);
    project_ = std::move(future_.back());
    future_.pop_back();
    return true;
}
} // namespace studio
