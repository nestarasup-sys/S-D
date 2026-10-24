#pragma once
#include <string>
#include <vector>

namespace studio {
struct LibraryItem {
    std::string id, name, category;
    double width, height;
};
const std::vector<LibraryItem> &nativeLibrary();
const LibraryItem *libraryItem(const std::string &id);
const std::vector<std::string> &motionNames();
const std::vector<std::string> &poseNames();
bool supportedMotion(const std::string &name);
bool supportedPose(const std::string &name);
} // namespace studio
