qt_add_executable(studio_dark WIN32 main.cpp editor_controller.cpp production_actions.cpp workspace_controller.cpp workspace_controller.hpp editor_controller.hpp preview_item.cpp preview_item.hpp)
qt_add_qml_module(studio_dark URI Studio.Dark VERSION 1.0
    QML_FILES qml/Main.qml qml/HomePage.qml qml/AssetLibraryPage.qml qml/StudioField.qml qml/StudioButton.qml qml/StudioComboBox.qml qml/LibraryPanel.qml qml/PreviewPanel.qml qml/InspectorPanel.qml qml/TimelinePanel.qml)
target_link_libraries(studio_dark PRIVATE studio_core studio_storage studio_composition studio_production Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia)
qt_add_resources(studio_dark car_example PREFIX "/examples" BASE "${PROJECT_SOURCE_DIR}/examples" FILES "${PROJECT_SOURCE_DIR}/examples/carros-caros.production.json")
