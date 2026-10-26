add_library(studio_composition STATIC src/painter.cpp src/render_snapshot.cpp src/composition_graph.cpp src/effect_pipeline.cpp src/native_painter.cpp src/native_painter_extra.cpp)
target_include_directories(studio_composition PUBLIC include)
target_link_libraries(studio_composition PUBLIC studio_core Qt6::Gui Qt6::Svg)
