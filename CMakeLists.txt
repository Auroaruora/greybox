cmake_minimum_required(VERSION 3.16)

project(greybox LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw EXCLUDE_FROM_ALL)

set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glm EXCLUDE_FROM_ALL)

add_library(glew STATIC external/glew/src/glew.c)
target_include_directories(glew PUBLIC external/glew/include)
target_compile_definitions(glew PUBLIC GLEW_STATIC GLEW_NO_GLU)
target_link_libraries(glew PUBLIC OpenGL::GL)

add_library(imgui STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC external/imgui external/imgui/backends)
target_include_directories(imgui PRIVATE external/glfw/include)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/app.cpp
    src/app.h
    src/window.cpp
    src/window.h

    src/camera/camera.cpp
    src/camera/camera.h
    src/camera/camerapose.h
    src/camera/camerasystem.cpp
    src/camera/camerasystem.h
    src/camera/cameracontroller.cpp
    src/camera/cameracontroller.h
    src/camera/debugcontroller.cpp
    src/camera/debugcontroller.h
    src/camera/focuscontroller.cpp
    src/camera/focuscontroller.h
    src/camera/seatedcontroller.cpp
    src/camera/seatedcontroller.h

    src/debug/capture.cpp
    src/debug/capture.h
    src/debug/debug.cpp
    src/debug/debug.h
    src/debug/debuglayout.cpp
    src/debug/debuglayout.h
    src/debug/debugui.cpp
    src/debug/debugui.h
    src/debug/screenshot.cpp
    src/debug/screenshot.h

    src/desktop/desktop.cpp
    src/desktop/desktop.h

    src/event/gameevent.h

    src/gl/framebuffer.cpp
    src/gl/framebuffer.h
    src/gl/mesh.cpp
    src/gl/mesh.h
    src/gl/meshcache.cpp
    src/gl/meshcache.h
    src/gl/quad.cpp
    src/gl/quad.h
    src/gl/shaderprogram.cpp
    src/gl/shaderprogram.h
    src/gl/texture2d.cpp
    src/gl/texture2d.h
    src/gl/texturecache.cpp
    src/gl/texturecache.h

    src/input/actions.h
    src/input/bindings.cpp
    src/input/bindings.h
    src/input/cursor.cpp
    src/input/cursor.h
    src/input/input.cpp
    src/input/input.h

    src/interaction/aabb.cpp
    src/interaction/aabb.h
    src/interaction/interactable.cpp
    src/interaction/interactable.h
    src/interaction/interactionsystem.cpp
    src/interaction/interactionsystem.h
    src/interaction/lightswitch.cpp
    src/interaction/lightswitch.h
    src/interaction/monitor.cpp
    src/interaction/monitor.h
    src/interaction/mug.cpp
    src/interaction/mug.h
    src/interaction/ray.h

    src/render/scenerenderer.cpp
    src/render/scenerenderer.h
    src/render/spriterenderer.cpp
    src/render/spriterenderer.h

    src/screen/roomscreen.cpp
    src/screen/roomscreen.h
    src/screen/scene.cpp
    src/screen/scene.h
    src/screen/screen.cpp
    src/screen/screen.h

    src/story/roomstory.cpp
    src/story/roomstory.h
    src/story/story.h

    src/utils/imageloader.cpp
    src/utils/imageloader.h
    src/utils/lightingreader.cpp
    src/utils/lightingreader.h
    src/utils/objloader.cpp
    src/utils/objloader.h
    src/utils/paths.cpp
    src/utils/paths.h
    src/utils/scenedata.h
    src/utils/scenefilereader.cpp
    src/utils/scenefilereader.h
    src/utils/sceneparser.cpp
    src/utils/sceneparser.h
    src/utils/shaderloader.cpp
    src/utils/shaderloader.h
    src/utils/viewpointreader.cpp
    src/utils/viewpointreader.h
)

target_include_directories(${PROJECT_NAME} PRIVATE src external/json external/stb)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    GLFW_INCLUDE_NONE
    GLM_ENABLE_EXPERIMENTAL
    ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
    CAPTURES_DIR="${CMAKE_SOURCE_DIR}/captures"
    $<$<NOT:$<CONFIG:Release>>:DEBUG_TOOLS>
)


target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glm::glm
    glew
    imgui
)

if (APPLE)
    target_compile_definitions(imgui PRIVATE GL_SILENCE_DEPRECATION)
endif()
