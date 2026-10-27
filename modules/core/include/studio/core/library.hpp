#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <studio/core/project.hpp>

namespace studio {
struct LibraryItem {
    std::string id, name, category;
    double width, height;
};
const std::vector<LibraryItem> &nativeLibrary();
const LibraryItem *libraryItem(const std::string &id);
struct PremiumAssetDefinition {
    std::string_view id;
    std::string_view name;
    std::string_view type;
    std::string_view category;
    std::string_view manifestResource;
    std::vector<Node> (*build)(int canvasWidth, int canvasHeight);
    std::vector<std::string_view> slotRoles;
    std::vector<std::string_view> paletteTokens;
};
const std::vector<PremiumAssetDefinition> &premiumLibrary();
const PremiumAssetDefinition *premiumAsset(const std::string &id);
std::vector<Node> makePremiumAsset(const std::string &id, int canvasWidth = 1280, int canvasHeight = 720);
std::vector<Node> makePremiumSmartphone();
std::vector<Node> makePremiumSmartphone(int canvasWidth, int canvasHeight);
std::vector<Node> makePremiumTitleReveal(int canvasWidth, int canvasHeight);
std::vector<Node> makePremiumComparisonPack(int canvasWidth, int canvasHeight);
std::vector<Node> makePremiumDramaticQuote(int canvasWidth, int canvasHeight);
std::vector<Node> makePremiumLowerThirdEditorial(int canvasWidth, int canvasHeight);
std::vector<Node> makePremiumSourceLabel(int canvasWidth, int canvasHeight);
std::vector<Node> makePremiumNumberEmphasis(int canvasWidth, int canvasHeight);
bool premiumSlotRole(const std::string &role);
const std::vector<std::string> &motionNames();
const std::vector<std::string> &poseNames();
bool supportedMotion(const std::string &name);
bool supportedPose(const std::string &name);
} // namespace studio
