add_library(studio_media STATIC src/wave.cpp)
target_include_directories(studio_media PUBLIC include)
target_link_libraries(studio_media PUBLIC Qt6::Core)
