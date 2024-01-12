cmake_minimum_required (VERSION 3.10)

project ("DreamRender")

set(CMAKE_CXX_STANDARD 17)

add_subdirectory("common")
add_subdirectory("core")

add_executable(DreamRender main.cpp)

target_include_directories(DreamRender PUBLIC 
    ./common
    ./core
    ./external/stb/include
    ./external/tinyobjloader/include
    ./external/embree/include
    ./external/glad/include
    ./external/glfw3/include
    ./external/glm/include
    ./external/nlohmann_json/include
)

target_link_libraries(DreamRender PUBLIC
    common
    core
    ../external/glfw3/lib/glfw3dll
    ../external/glad/lib/glad
    ../external/embree/lib/embree3
    ../external/embree/lib/tbb
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(TARGET_NAME DreamRender)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shader $<TARGET_FILE_DIR:${TARGET_NAME}>/shader

    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/external/embree/bin/embree3.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/embree3.dll

    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/external/embree/bin/tbb12.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/tbb12.dll

    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw3/bin/glfw3.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/glfw3.dll
)

