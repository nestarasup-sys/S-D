qt_add_executable(studio_scenegraph_prototype main.cpp)
target_link_libraries(studio_scenegraph_prototype PRIVATE studio_core studio_composition Qt6::Quick)
target_compile_definitions(studio_scenegraph_prototype PRIVATE
    STUDIO_PRIMITIVE_REFERENCE_DIR="${PROJECT_SOURCE_DIR}/tests/reference/render-primitives")
if(WIN32)
    target_link_libraries(studio_scenegraph_prototype PRIVATE psapi)
endif()
