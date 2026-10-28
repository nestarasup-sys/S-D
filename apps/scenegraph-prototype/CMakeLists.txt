qt_add_executable(studio_scenegraph_prototype main.cpp gpu_effect_item.cpp gpu_effect_item.hpp
    gpu_memory_diagnostics.cpp gpu_memory_diagnostics.hpp)
target_link_libraries(studio_scenegraph_prototype PRIVATE studio_core studio_composition Qt6::Quick Qt6::GuiPrivate)
qt_add_shaders(studio_scenegraph_prototype gpu_effect_shaders
    PREFIX "/gpu-effects"
    BASE "${PROJECT_SOURCE_DIR}/tests/shaders"
    FILES
        "${PROJECT_SOURCE_DIR}/tests/shaders/fullscreen.vert"
        "${PROJECT_SOURCE_DIR}/tests/shaders/alpha_mask.frag"
        "${PROJECT_SOURCE_DIR}/tests/shaders/gaussian_blur_horizontal.frag"
        "${PROJECT_SOURCE_DIR}/tests/shaders/gaussian_blur_vertical.frag"
        "${PROJECT_SOURCE_DIR}/tests/shaders/glow_composite.frag")
target_compile_definitions(studio_scenegraph_prototype PRIVATE
    STUDIO_PRIMITIVE_REFERENCE_DIR="${PROJECT_SOURCE_DIR}/tests/reference/render-primitives"
    STUDIO_HYBRID_REFERENCE_DIR="${PROJECT_SOURCE_DIR}/tests/reference/render-hybrid")
if(WIN32)
    target_link_libraries(studio_scenegraph_prototype PRIVATE psapi dxgi d3d11)
endif()
