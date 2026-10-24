#pragma once
#include "project.hpp"
#include <functional>

namespace studio {
// Every mutation is validated on a candidate before it becomes visible.
class Editor {
  public:
    explicit Editor(Project project);
    const Project &project() const {
        return project_;
    }
    std::string execute(const std::function<void(Project &)> &mutation);
    void beginGesture();
    void endGesture();
    bool undo();
    bool redo();
    bool canUndo() const {
        return !past_.empty();
    }
    bool canRedo() const {
        return !future_.empty();
    }
    void reset(Project project);

  private:
    void remember(Project project);
    Project project_;
    std::vector<Project> past_, future_;
    bool gesturing_{false}, gestureChanged_{false};
    Project gestureStart_;
};
} // namespace studio
