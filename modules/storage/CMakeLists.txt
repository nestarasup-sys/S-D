add_library(studio_storage STATIC src/project_store.cpp src/workspace_store.cpp)
target_include_directories(studio_storage PUBLIC include)
target_link_libraries(studio_storage PUBLIC studio_core Qt6::Core Qt6::Gui)
qt_add_resources(studio_storage premium_manifests
    PREFIX "/studio/premium"
    BASE "${PROJECT_SOURCE_DIR}/assets/premium"
    FILES
        "${PROJECT_SOURCE_DIR}/assets/premium/smartphone-premium-01.manifest.json"
        "${PROJECT_SOURCE_DIR}/assets/premium/title-reveal-01.manifest.json"
        "${PROJECT_SOURCE_DIR}/assets/premium/comparison-pack-01.manifest.json"
        "${PROJECT_SOURCE_DIR}/assets/premium/dramatic-quote-01.manifest.json"
        "${PROJECT_SOURCE_DIR}/assets/premium/lower-third-editorial-01.manifest.json"
        "${PROJECT_SOURCE_DIR}/assets/premium/source-label-01.manifest.json"
        "${PROJECT_SOURCE_DIR}/assets/premium/number-emphasis-01.manifest.json")
