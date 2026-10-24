add_library(studio_composition STATIC src/painter.cpp src/native_painter.cpp)
target_include_directories(studio_composition PUBLIC include)
target_link_libraries(studio_composition PUBLIC studio_core Qt6::Gui Qt6::Svg)
