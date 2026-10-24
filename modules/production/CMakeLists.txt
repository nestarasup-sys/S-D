add_library(studio_production STATIC src/recipe.cpp src/compiler.cpp src/guide.cpp)
target_include_directories(studio_production PUBLIC include)
target_link_libraries(studio_production PUBLIC studio_core studio_media Qt6::Core)
