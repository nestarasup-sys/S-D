#pragma once

#include <QJsonObject>
#include <QString>
#include <vector>
#include <studio/core/project.hpp>

namespace studio {

enum class VisualIssueSeverity { Information, Warning, Error };

struct VisualQualityIssue {
    QString type;
    VisualIssueSeverity severity{VisualIssueSeverity::Warning};
    QString sceneId, shotId, elementId;
    std::int64_t firstFrame{0}, lastFrame{0};
    QString description, evidence, suggestion;
    QJsonObject toJson() const;
    QString displayText() const;
};

struct VisualQualityReport {
    QString projectName;
    int width{0}, height{0};
    std::vector<std::int64_t> sampledFrames;
    std::vector<VisualQualityIssue> issues;
    QJsonObject toJson() const;
};

class VisualQualityAnalyzer final {
  public:
    static VisualQualityReport analyze(const Project &project);
};

} // namespace studio
