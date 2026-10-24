qt_add_executable(studio_produce main.cpp)
target_link_libraries(studio_produce PRIVATE studio_production studio_storage)
qt_add_resources(studio_produce speech_script PREFIX "/production" BASE "${PROJECT_SOURCE_DIR}/scripts" FILES "${PROJECT_SOURCE_DIR}/scripts/synthesize.ps1")
