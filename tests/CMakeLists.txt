add_executable(studio_tests core_and_storage_tests.cpp)
target_link_libraries(studio_tests PRIVATE studio_core studio_storage studio_composition)
add_test(NAME core_storage_composition COMMAND studio_tests)
set_tests_properties(core_storage_composition PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
add_executable(studio_controller_tests controller_tests.cpp ../apps/desktop/editor_controller.cpp ../apps/desktop/production_actions.cpp ../apps/desktop/editor_controller.hpp)
target_include_directories(studio_controller_tests PRIVATE ../apps/desktop)
target_link_libraries(studio_controller_tests PRIVATE studio_storage studio_production Qt6::Gui Qt6::Svg Qt6::Multimedia)
add_test(NAME application_controller COMMAND studio_controller_tests)
set_tests_properties(application_controller PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
add_executable(studio_production_tests production_tests.cpp)
target_link_libraries(studio_production_tests PRIVATE studio_production studio_storage studio_composition)
target_compile_definitions(studio_production_tests PRIVATE STUDIO_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME production_recipe_audio_library COMMAND studio_production_tests)
set_tests_properties(production_recipe_audio_library PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
add_executable(studio_workspace_tests workspace_tests.cpp ../apps/desktop/workspace_controller.cpp ../apps/desktop/workspace_controller.hpp ../apps/desktop/editor_controller.cpp ../apps/desktop/production_actions.cpp ../apps/desktop/editor_controller.hpp)
target_include_directories(studio_workspace_tests PRIVATE ../apps/desktop)
target_link_libraries(studio_workspace_tests PRIVATE studio_storage studio_production studio_composition Qt6::Gui Qt6::Svg Qt6::Multimedia)
add_test(NAME workspace_projects_library COMMAND studio_workspace_tests)
set_tests_properties(workspace_projects_library PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
add_executable(studio_render_snapshot_tests render_snapshot_tests.cpp)
target_link_libraries(studio_render_snapshot_tests PRIVATE studio_composition)
target_compile_definitions(studio_render_snapshot_tests PRIVATE
    STUDIO_REFERENCE_DIR="${PROJECT_SOURCE_DIR}/tests/reference/render-snapshot")
add_test(NAME render_snapshot_visual_regression COMMAND studio_render_snapshot_tests)
add_executable(studio_effect_pipeline_tests effect_pipeline_tests.cpp)
target_link_libraries(studio_effect_pipeline_tests PRIVATE studio_composition Qt6::Gui)
target_compile_definitions(studio_effect_pipeline_tests PRIVATE
    STUDIO_EFFECT_REFERENCE="${PROJECT_SOURCE_DIR}/tests/reference/render-effects/pipeline.png")
add_test(NAME experimental_render_effects COMMAND studio_effect_pipeline_tests)
set_tests_properties(experimental_render_effects PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(studio_gpu_effect_tests
    gpu_effect_runtime_tests.cpp
    ../apps/scenegraph-prototype/gpu_effect_item.cpp
    ../apps/scenegraph-prototype/gpu_effect_item.hpp)
target_include_directories(studio_gpu_effect_tests PRIVATE ../apps/scenegraph-prototype)
target_link_libraries(studio_gpu_effect_tests PRIVATE studio_composition Qt6::Quick Qt6::GuiPrivate)
target_compile_definitions(studio_gpu_effect_tests PRIVATE
    STUDIO_GPU_EFFECT_REFERENCE_DIR="${PROJECT_SOURCE_DIR}/tests/reference/gpu-effects"
    STUDIO_GPU_EFFECT_ARTIFACT_DIR="${PROJECT_SOURCE_DIR}/artifacts/gpu-effects-test")
qt_add_shaders(studio_gpu_effect_tests gpu_effect_shaders
    PREFIX "/gpu-effects"
    BASE "${PROJECT_SOURCE_DIR}/tests/shaders"
    FILES
        "${PROJECT_SOURCE_DIR}/tests/shaders/fullscreen.vert"
        "${PROJECT_SOURCE_DIR}/tests/shaders/alpha_mask.frag"
        "${PROJECT_SOURCE_DIR}/tests/shaders/gaussian_blur_horizontal.frag"
        "${PROJECT_SOURCE_DIR}/tests/shaders/gaussian_blur_vertical.frag"
        "${PROJECT_SOURCE_DIR}/tests/shaders/glow_composite.frag")
add_test(NAME scenegraph_gpu_effect_runtime COMMAND studio_gpu_effect_tests)
add_test(NAME scenegraph_gpu_effect_scene_animated
    COMMAND studio_scenegraph_prototype --gpu-effects-scene --nodes 5 --frames 8 --width 1280 --height 720
            --output "${PROJECT_SOURCE_DIR}/artifacts/gpu-effect-scene-ctest-animated")
add_test(NAME scenegraph_gpu_effect_scene_static
    COMMAND studio_scenegraph_prototype --gpu-effects-scene --nodes 5 --frames 8 --static-scene
            --width 1280 --height 720 --output "${PROJECT_SOURCE_DIR}/artifacts/gpu-effect-scene-ctest-static")
