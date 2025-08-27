project(common)

set(COMMON_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/console.h
    ${CMAKE_CURRENT_SOURCE_DIR}/console.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/interface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_pass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render_pass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_graph.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_pipeline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render_pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/color.h
    ${CMAKE_CURRENT_SOURCE_DIR}/color.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/spectrum.h
    ${CMAKE_CURRENT_SOURCE_DIR}/spectrum.cpp
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ROOT_INCLUDE_DIR}
    ${GLAD_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${SPDLOG_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIR}
    ${FMT_INCLUDE_DIR}
)

add_library(${PROJECT_NAME} ${COMMON_SRCS})

target_link_libraries(${PROJECT_NAME}
    glad
    glfw
    spdlog
    imgui
    fmt
)