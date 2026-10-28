add_library(studio_composition STATIC src/painter.cpp src/render_snapshot.cpp src/composition_graph.cpp src/effect_pipeline.cpp src/native_painter.cpp src/native_painter_extra.cpp src/text_layout.cpp src/visual_quality.cpp)
target_include_directories(studio_composition PUBLIC include)
target_link_libraries(studio_composition PUBLIC studio_core Qt6::Gui Qt6::Svg)
file(GLOB AI_LIBRARY_IMAGE_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/assets/ai-library/v1/*/*.png")
qt_add_resources(studio_composition ai_library
    PREFIX "/studio/ai-library/v1"
    BASE "${PROJECT_SOURCE_DIR}/assets/ai-library/v1"
    FILES ${AI_LIBRARY_IMAGE_FILES})
