#pragma once
#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <studio/core/project.hpp>

namespace studio {
struct LibraryItem {
    struct PoseArtwork {
        std::string pose, resourcePath;
        double anchorX{205}, anchorBottom{386};
    };
    std::string id, name, category;
    double width, height;
    // Optional immutable artwork embedded for this native ID. The original procedural
    // renderer remains an internal fallback if the image resource cannot be decoded.
    std::string resourcePath, fallbackId;
    int sourceWidth{0}, sourceHeight{0};
    bool previewOnly{false};
    std::vector<PoseArtwork> poseArtworks;

    LibraryItem(std::string itemId, std::string itemName, std::string itemCategory,
                double itemWidth, double itemHeight, std::string itemResourcePath = {},
                std::string itemFallbackId = {}, int imageWidth = 0, int imageHeight = 0,
                bool isPreviewOnly = false)
        : id(std::move(itemId)), name(std::move(itemName)), category(std::move(itemCategory)),
          width(itemWidth), height(itemHeight), resourcePath(std::move(itemResourcePath)),
          fallbackId(std::move(itemFallbackId)), sourceWidth(imageWidth),
          sourceHeight(imageHeight), previewOnly(isPreviewOnly) {}
    const PoseArtwork *poseArtwork(const std::string &pose) const;
    std::string artworkForPose(const std::string &pose) const;
};
const std::vector<LibraryItem> &nativeLibrary();
const LibraryItem *libraryItem(const std::string &id);
struct PremiumDeclarativeLayer {
    std::string id, parentId, name, component, role, text, color, paletteToken, animation;
    double x{0}, y{0}, width{0}, height{0}, fontSize{44}, opacity{1}, rotation{0}, scale{1};
    int animationDelayFrames{0}, animationDurationFrames{24}, staggerFrames{0};
};
struct PremiumDeclarativeLayout {
    int referenceWidth{1280}, referenceHeight{720};
    std::vector<PremiumDeclarativeLayer> layers;
};
struct PremiumDeclarativeDefinition {
    PremiumDeclarativeLayout landscape, portrait;
};
struct PremiumPaletteBinding {
    std::string token;
    std::vector<std::string> nodeIds;
};
struct PremiumTextLayoutPolicy {
    std::string role;
    double minimumFontSize{12};
    double padding{0};
};
struct PremiumAssetDefinition {
    std::string id, name, type, category, manifestResource;
    std::vector<Node> (*build)(int canvasWidth, int canvasHeight){nullptr};
    std::vector<std::string> slotRoles;
    std::vector<std::string> paletteTokens;
    std::shared_ptr<const PremiumDeclarativeDefinition> declarative;
    std::vector<PremiumPaletteBinding> paletteBindings;
    int manifestVersion{1};
    std::vector<PremiumTextLayoutPolicy> textLayoutPolicies;
};
const std::vector<PremiumAssetDefinition> &premiumLibrary();
const PremiumAssetDefinition *premiumAsset(const std::string &id);
bool registerDeclarativePremium(PremiumAssetDefinition definition, std::string &error);
bool registerPremiumTextLayoutPolicies(const std::string &assetId,
                                      std::vector<PremiumTextLayoutPolicy> policies, std::string &error);
std::vector<Node> buildDeclarativePremium(const PremiumAssetDefinition &definition, int canvasWidth,
                                         int canvasHeight);
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
std::string canonicalPoseName(const std::string &name);
} // namespace studio
