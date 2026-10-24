add_library(studio_storage STATIC src/project_store.cpp)
target_include_directories(studio_storage PUBLIC include)
target_link_libraries(studio_storage PUBLIC studio_core Qt6::Core)
