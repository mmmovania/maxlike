cmake_minimum_required(VERSION 3.20)
project(maxlike VERSION 0.10.0 LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "maxlike is a Windows application")
endif()

# GLAD: the generated OpenGL 3.3 core + WGL loader (third_party/glad, see its README).
add_library(glad STATIC
    third_party/glad/src/gl.c
    third_party/glad/src/wgl.c
)
target_include_directories(glad PUBLIC third_party/glad/include)

add_executable(maxlike WIN32
    src/main.cpp
    src/win/App.cpp
    src/win/App.h
    src/win/Window.cpp
    src/win/Window.h
    src/ui/Theme.cpp
    src/ui/Theme.h
    src/ui/MainFrame.cpp
    src/ui/MainFrame.h
    src/ui/Metrics.h
    src/ui/FrameLayout.cpp
    src/ui/FrameLayout.h
    src/ui/Bevel.cpp
    src/ui/Bevel.h
    src/ui/CommandPanel.cpp
    src/ui/CommandPanel.h
    src/ui/CommandPanelLayout.cpp
    src/ui/CommandPanelLayout.h
    src/ui/Rollup.cpp
    src/ui/Rollup.h
    src/ui/SpinnerModel.cpp
    src/ui/SpinnerModel.h
    src/ui/ParametersSpec.cpp
    src/ui/ParametersSpec.h
    src/ui/ModifiersDialog.cpp
    src/ui/ModifiersDialog.h
    src/ui/Dialogs.cpp
    src/ui/Dialogs.h
    src/ui/CommandTable.cpp
    src/ui/CommandTable.h
    src/ui/MenuSpec.cpp
    src/ui/MenuSpec.h
    src/ui/MenuBuilder.cpp
    src/ui/MenuBuilder.h
    src/ui/TabStrip.cpp
    src/ui/TabStrip.h
    src/ui/TabPanel.cpp
    src/ui/TabPanel.h
    src/ui/ToolbarSpec.cpp
    src/ui/ToolbarSpec.h
    src/ui/Toolbar.cpp
    src/ui/Toolbar.h
    src/ui/FlyoutStrip.cpp
    src/ui/FlyoutStrip.h
    src/ui/TimeSliderGeometry.cpp
    src/ui/TimeSliderGeometry.h
    src/ui/TimeSlider.cpp
    src/ui/TimeSlider.h
    src/ui/TrackBar.cpp
    src/ui/TrackBar.h
    src/ui/StatusLayout.cpp
    src/ui/StatusLayout.h
    src/ui/StatusBar.cpp
    src/ui/StatusBar.h
    src/ui/ViewportLayout.cpp
    src/ui/ViewportLayout.h
    src/ui/Viewport.cpp
    src/ui/Viewport.h
    src/ui/ViewPanel.cpp
    src/ui/ViewPanel.h
    src/gl/GLContext.cpp
    src/gl/GLContext.h
    src/gl/LineRenderer.cpp
    src/gl/LineRenderer.h
    src/gl/MeshRenderer.cpp
    src/gl/MeshRenderer.h
    src/view/OrthoGrid.cpp
    src/view/OrthoGrid.h
    src/view/PerspectiveView.cpp
    src/view/PerspectiveView.h
    src/view/Tripod.cpp
    src/view/Tripod.h
    src/view/Navigation.cpp
    src/view/Navigation.h
    src/view/Picking.cpp
    src/view/Picking.h
    src/view/TransformAxes.cpp
    src/view/TransformAxes.h
    src/ui/LabelText.cpp
    src/ui/LabelText.h
    src/math/Mat4.cpp
    src/math/Mat4.h
    src/math/Quat.cpp
    src/math/Quat.h
    src/scene/Mesh.cpp
    src/scene/Mesh.h
    src/scene/ParamBlock.cpp
    src/scene/ParamBlock.h
    src/scene/Object.h
    src/scene/Node.cpp
    src/scene/Node.h
    src/scene/Scene.cpp
    src/scene/Scene.h
    src/scene/Primitives.cpp
    src/scene/Primitives.h
    src/scene/TeapotData.h
    src/scene/Modifier.cpp
    src/scene/Modifier.h
    src/scene/Undo.cpp
    src/scene/Undo.h
    src/scene/Creation.cpp
    src/scene/Creation.h
    src/scene/CreationTool.cpp
    src/scene/CreationTool.h
    src/scene/MoveMath.cpp
    src/scene/MoveMath.h
    src/scene/Clone.cpp
    src/scene/Clone.h
    src/scene/MeshTopology.cpp
    src/scene/MeshTopology.h
    src/scene/Summary.cpp
    src/scene/Summary.h
    src/scene/TypeIn.cpp
    src/scene/TypeIn.h
    src/scene/Arrange.cpp
    src/scene/Arrange.h
    src/scene/Groups.cpp
    src/scene/Groups.h
    src/scene/SoftSelection.cpp
    src/scene/SoftSelection.h
    src/io/Cfb.cpp
    src/io/Cfb.h
    src/io/Chunks.cpp
    src/io/Chunks.h
    src/io/MaxScene.cpp
    src/io/MaxScene.h
    src/io/MaxNodes.cpp
    src/io/MaxNodes.h
    src/io/MaxObjects.cpp
    src/io/MaxObjects.h
    src/io/MaxImport.cpp
    src/io/MaxImport.h
    src/io/MaxViewports.cpp
    src/io/MaxViewports.h
    src/io/NativeFile.cpp
    src/io/NativeFile.h
    src/io/Merge.cpp
    src/io/Merge.h
    src/io/Utf16.h
    src/scene/DemoScene.cpp
    src/scene/DemoScene.h
    res/resource.h
    res/maxlike.rc
)

target_include_directories(maxlike PRIVATE src res)

# The .rc file pulls in these files; tell the build so editing them recompiles it.
set_source_files_properties(res/maxlike.rc PROPERTIES OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/res/toolbar_icons.bmp;${CMAKE_CURRENT_SOURCE_DIR}/res/status_icons.bmp;${CMAKE_CURRENT_SOURCE_DIR}/res/panel_tab_icons.bmp;${CMAKE_CURRENT_SOURCE_DIR}/res/panel_category_icons.bmp;${CMAKE_CURRENT_SOURCE_DIR}/res/objects_icons.bmp;${CMAKE_CURRENT_SOURCE_DIR}/res/maxlike.manifest;${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h")

target_compile_definitions(maxlike PRIVATE
    UNICODE _UNICODE
    WIN32_LEAN_AND_MEAN NOMINMAX
    WINVER=0x0A00 _WIN32_WINNT=0x0A00
    MAXLIKE_VERSION="${PROJECT_VERSION}"
)

if(MSVC)
    target_compile_options(maxlike PRIVATE /W4 /permissive- /utf-8)
    # Our own manifest comes from res/maxlike.rc; stop the linker adding a second one.
    target_link_options(maxlike PRIVATE /MANIFEST:NO)
else()
    target_compile_options(maxlike PRIVATE -Wall -Wextra)
    target_link_options(maxlike PRIVATE -municode -static)
endif()

target_link_libraries(maxlike PRIVATE glad opengl32 user32 gdi32 msimg32 comctl32 comdlg32 shell32 psapi) # msimg32: TransparentBlt; comctl32: tooltips; shell32: drag-and-drop, command line

# ---- Unit tests (console program; run with: ctest --test-dir build -C Debug) ----
enable_testing()

# Each test is its own small console program; CTest runs them all.
function(maxlike_add_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE src)
    if(MSVC)
        target_compile_options(${name} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${name} PRIVATE -Wall -Wextra)
        target_link_options(${name} PRIVATE -static)
    endif()
    add_test(NAME ${name} COMMAND ${name})
endfunction()

maxlike_add_test(test_frame_layout tests/test_frame_layout.cpp src/ui/FrameLayout.cpp)
maxlike_add_test(test_menu_spec
    tests/test_menu_spec.cpp src/ui/MenuSpec.cpp src/ui/CommandTable.cpp)
maxlike_add_test(test_tab_strip tests/test_tab_strip.cpp src/ui/TabStrip.cpp)
maxlike_add_test(test_toolbar_spec tests/test_toolbar_spec.cpp src/ui/ToolbarSpec.cpp)
maxlike_add_test(test_time_slider tests/test_time_slider.cpp src/ui/TimeSliderGeometry.cpp)
maxlike_add_test(test_status_layout tests/test_status_layout.cpp src/ui/StatusLayout.cpp)
maxlike_add_test(test_viewport_layout tests/test_viewport_layout.cpp src/ui/ViewportLayout.cpp)
maxlike_add_test(test_ortho_grid tests/test_ortho_grid.cpp src/view/OrthoGrid.cpp src/math/Mat4.cpp)
maxlike_add_test(test_perspective tests/test_perspective.cpp src/math/Mat4.cpp src/view/PerspectiveView.cpp src/view/OrthoGrid.cpp)
maxlike_add_test(test_tripod tests/test_tripod.cpp src/view/Tripod.cpp)
maxlike_add_test(test_navigation tests/test_navigation.cpp src/view/Navigation.cpp src/view/OrthoGrid.cpp src/view/PerspectiveView.cpp src/math/Mat4.cpp)
maxlike_add_test(test_scene tests/test_scene.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
maxlike_add_test(test_primitives tests/test_primitives.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp)
maxlike_add_test(test_modifiers tests/test_modifiers.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
maxlike_add_test(test_undo tests/test_undo.cpp src/scene/Undo.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
maxlike_add_test(test_command_panel_layout tests/test_command_panel_layout.cpp src/ui/CommandPanelLayout.cpp)
maxlike_add_test(test_rollup tests/test_rollup.cpp src/ui/Rollup.cpp)
maxlike_add_test(test_creation tests/test_creation.cpp src/scene/Creation.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/math/Quat.cpp src/math/Mat4.cpp)
maxlike_add_test(test_creation_tool tests/test_creation_tool.cpp src/scene/CreationTool.cpp src/scene/Creation.cpp src/scene/Undo.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
maxlike_add_test(test_spinner tests/test_spinner.cpp src/ui/SpinnerModel.cpp)
maxlike_add_test(test_parameters_spec tests/test_parameters_spec.cpp src/ui/ParametersSpec.cpp src/scene/CreationTool.cpp src/scene/Creation.cpp src/scene/Undo.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
maxlike_add_test(test_picking tests/test_picking.cpp src/view/Picking.cpp)
maxlike_add_test(test_transform_axes tests/test_transform_axes.cpp src/view/TransformAxes.cpp)
maxlike_add_test(test_move_math tests/test_move_math.cpp src/scene/MoveMath.cpp src/scene/Node.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/math/Quat.cpp src/math/Mat4.cpp)
maxlike_add_test(test_cfb tests/test_cfb.cpp src/io/Cfb.cpp)
target_compile_definitions(test_cfb PRIVATE MAXLIKE_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
maxlike_add_test(test_chunks tests/test_chunks.cpp src/io/Chunks.cpp)
maxlike_add_test(test_samples tests/test_samples.cpp src/io/Cfb.cpp src/io/Chunks.cpp)
target_compile_definitions(test_samples PRIVATE MAXLIKE_SAMPLES="${CMAKE_CURRENT_SOURCE_DIR}/docs/reference/samples")
maxlike_add_test(test_max_scene tests/test_max_scene.cpp src/io/MaxScene.cpp src/io/Cfb.cpp src/io/Chunks.cpp)
target_compile_definitions(test_max_scene PRIVATE MAXLIKE_SAMPLES="${CMAKE_CURRENT_SOURCE_DIR}/docs/reference/samples")
maxlike_add_test(test_max_nodes tests/test_max_nodes.cpp src/io/MaxNodes.cpp src/io/MaxScene.cpp src/io/Cfb.cpp src/io/Chunks.cpp src/math/Quat.cpp src/math/Mat4.cpp)
target_compile_definitions(test_max_nodes PRIVATE MAXLIKE_SAMPLES="${CMAKE_CURRENT_SOURCE_DIR}/docs/reference/samples")
maxlike_add_test(test_max_objects tests/test_max_objects.cpp src/io/MaxObjects.cpp src/io/MaxNodes.cpp src/io/MaxScene.cpp src/io/Cfb.cpp src/io/Chunks.cpp src/scene/CreationTool.cpp src/scene/Creation.cpp src/scene/Undo.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
target_compile_definitions(test_max_objects PRIVATE MAXLIKE_SAMPLES="${CMAKE_CURRENT_SOURCE_DIR}/docs/reference/samples")
maxlike_add_test(test_max_import tests/test_max_import.cpp src/io/MaxImport.cpp src/io/MaxNodes.cpp src/io/MaxObjects.cpp src/io/MaxScene.cpp src/io/Cfb.cpp src/io/Chunks.cpp src/scene/CreationTool.cpp src/scene/Creation.cpp src/scene/Undo.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
target_compile_definitions(test_max_import PRIVATE MAXLIKE_SAMPLES="${CMAKE_CURRENT_SOURCE_DIR}/docs/reference/samples")
maxlike_add_test(test_max_viewports tests/test_max_viewports.cpp src/io/MaxViewports.cpp src/io/Cfb.cpp src/io/Chunks.cpp src/view/PerspectiveView.cpp src/view/OrthoGrid.cpp src/math/Quat.cpp src/math/Mat4.cpp)
target_compile_definitions(test_max_viewports PRIVATE MAXLIKE_SAMPLES="${CMAKE_CURRENT_SOURCE_DIR}/docs/reference/samples")
maxlike_add_test(test_native_file tests/test_native_file.cpp src/io/NativeFile.cpp src/io/MaxImport.cpp src/io/MaxNodes.cpp src/io/MaxObjects.cpp src/io/MaxScene.cpp src/io/Cfb.cpp src/io/Chunks.cpp src/scene/DemoScene.cpp src/scene/CreationTool.cpp src/scene/Creation.cpp src/scene/Undo.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
target_compile_definitions(test_native_file PRIVATE MAXLIKE_SAMPLES="${CMAKE_CURRENT_SOURCE_DIR}/docs/reference/samples")
maxlike_add_test(test_merge tests/test_merge.cpp src/io/Merge.cpp src/io/NativeFile.cpp src/io/MaxImport.cpp src/io/MaxNodes.cpp src/io/MaxObjects.cpp src/io/MaxScene.cpp src/io/Cfb.cpp src/io/Chunks.cpp src/scene/CreationTool.cpp src/scene/Creation.cpp src/scene/Undo.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
target_compile_definitions(test_merge PRIVATE MAXLIKE_SAMPLES="${CMAKE_CURRENT_SOURCE_DIR}/docs/reference/samples")
maxlike_add_test(test_clone tests/test_clone.cpp src/scene/Clone.cpp src/scene/Summary.cpp src/io/NativeFile.cpp src/io/MaxImport.cpp src/io/MaxNodes.cpp src/io/MaxObjects.cpp src/io/MaxScene.cpp src/io/Cfb.cpp src/io/Chunks.cpp src/scene/CreationTool.cpp src/scene/Creation.cpp src/scene/Undo.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
target_compile_definitions(test_clone PRIVATE MAXLIKE_SAMPLES="${CMAKE_CURRENT_SOURCE_DIR}/docs/reference/samples")
maxlike_add_test(test_edit_mesh tests/test_edit_mesh.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Undo.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/view/Picking.cpp src/math/Quat.cpp src/math/Mat4.cpp)
maxlike_add_test(test_typein tests/test_typein.cpp src/scene/TypeIn.cpp src/scene/MoveMath.cpp src/io/MaxImport.cpp src/io/MaxNodes.cpp src/io/MaxObjects.cpp src/io/MaxScene.cpp src/io/Cfb.cpp src/io/Chunks.cpp src/scene/CreationTool.cpp src/scene/Creation.cpp src/scene/Undo.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
target_compile_definitions(test_typein PRIVATE MAXLIKE_SAMPLES="${CMAKE_CURRENT_SOURCE_DIR}/docs/reference/samples")
maxlike_add_test(test_arrange tests/test_arrange.cpp src/scene/Arrange.cpp src/scene/Groups.cpp src/scene/TypeIn.cpp src/scene/Clone.cpp src/scene/MoveMath.cpp src/scene/CreationTool.cpp src/scene/Creation.cpp src/scene/Undo.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
maxlike_add_test(test_groups tests/test_groups.cpp src/scene/Groups.cpp src/scene/Undo.cpp src/scene/Modifier.cpp src/scene/MeshTopology.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/scene/Node.cpp src/scene/Scene.cpp src/math/Quat.cpp src/math/Mat4.cpp)
maxlike_add_test(test_soft_selection tests/test_soft_selection.cpp src/scene/SoftSelection.cpp src/scene/Primitives.cpp src/scene/Mesh.cpp src/scene/ParamBlock.cpp src/math/Quat.cpp src/math/Mat4.cpp)
