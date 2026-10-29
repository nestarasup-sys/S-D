add_library(studio_storage STATIC src/project_store.cpp src/workspace_store.cpp src/premium_catalog.cpp)
target_include_directories(studio_storage PUBLIC include)
target_link_libraries(studio_storage PUBLIC studio_core Qt6::Core Qt6::Gui)
file(GLOB PREMIUM_MANIFEST_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/assets/premium/*.manifest.json")
qt_add_resources(studio_storage premium_manifests
    PREFIX "/studio/premium"
    BASE "${PROJECT_SOURCE_DIR}/assets/premium"
    FILES ${PREMIUM_MANIFEST_FILES})
qt_add_resources(studio_storage ui_component_manifest
    PREFIX "/studio/ui-components"
    BASE "${PROJECT_SOURCE_DIR}/assets/ai-library/v1/components"
    FILES "${PROJECT_SOURCE_DIR}/assets/ai-library/v1/components/asset-manifest.json")
