cmake_minimum_required(VERSION 3.0.0)

project(dream LANGUAGES C CXX)

# Find OpenGL
find_package(OpenGL REQUIRED)

set(ROOT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Add glad
set(GLAD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include)
add_library(glad ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/src/glad.c)

# Add glfw
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_INSTALL OFF)
set(GLFW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)

# Add glm
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glm/include)

# Add spdlog
set(SPDLOG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/spdlog/include)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/spdlog)

# Add imgui
set(IMGUI_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
)
set(IMGUI_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/misc/cpp/imgui_stdlib.cpp
)
add_library(imgui ${IMGUI_SRCS})

# Add fmt
set(FMT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/fmt/include)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/fmt)

# Add nlohmann
set(NLOHMANN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/nlohmann/include)

# Add embree
set(EMBREE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/embree/include)
set(EMBREE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/embree/lib)

# Add stb
set(STB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/stb)

# Add tiny
set(TINY_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/tiny)

# Add common utils
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/common)
set(COMMON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)

link_directories(
    ${EMBREE_LIB_DIR}
)

include_directories(
    ${ROOT_INCLUDE_DIR}
    ${GLAD_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${SPDLOG_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIR}
    ${FMT_INCLUDE_DIR}
    ${NLOHMANN_INCLUDE_DIR}
    ${EMBREE_INCLUDE_DIR}
    ${STB_INCLUDE_DIR}
    ${TINY_INCLUDE_DIR}
    ${COMMON_INCLUDE_DIR}
)

add_executable(${PROJECT_NAME}
    utils.h
    main.cpp
)

target_link_libraries(${PROJECT_NAME}
    glad
    glfw
    spdlog
    imgui
    fmt
    embree3
    tbb
    common
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)