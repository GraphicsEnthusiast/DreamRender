cmake_minimum_required (VERSION 3.10)

project ("DreamRender")

set(CMAKE_CXX_STANDARD 17)

add_subdirectory("opengl_utils")
add_subdirectory("core")

add_executable(DreamRender main.cpp)

target_include_directories(DreamRender PUBLIC 
    opengl_utils/
    opengl_utils/include
    core/include
    dep/embree/include
    dep/oidn/include
    dep/glad/include
    dep/glfw3/include
    dep/glm/include
    dep/nlohmann-json/include
)

target_link_libraries(DreamRender PUBLIC
    opengl_utils
    core
    ../dep/glfw3/lib/glfw3dll
    ../dep/glad/lib/glad
    ../dep/oidn/lib/OpenImageDenoise
    ../dep/embree/lib/embree3
    ../dep/embree/lib/tbb
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(TARGET_NAME DreamRender)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shader $<TARGET_FILE_DIR:${TARGET_NAME}>/../shader

    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/scene $<TARGET_FILE_DIR:${TARGET_NAME}>/scene

    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/dep/embree/bin/embree3.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/embree3.dll

    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/dep/embree/bin/tbb12.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/tbb12.dll

    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/dep/oidn/bin/OpenImageDenoise.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/OpenImageDenoise.dll

    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/dep/glfw3/bin/glfw3.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/glfw3.dll
)