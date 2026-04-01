project(common)

set(COMMON_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/console.h
    ${CMAKE_CURRENT_SOURCE_DIR}/console.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/interface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_pass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render_pass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_graph.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_pipeline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render_pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shape.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transform.h
    ${CMAKE_CURRENT_SOURCE_DIR}/transform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer_object.h
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer_object.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sampling.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sampling.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/srgb_to_spectrum.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sobol_matrices_1024x52.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cie.h
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
    ${LOADER_INCLUDE_DIR}
    ${BVH_INCLUDE_DIR}
)

add_library(${PROJECT_NAME} ${COMMON_SRCS})

target_link_libraries(${PROJECT_NAME}
    glad
    glfw
    spdlog
    imgui
    fmt
)