#cmake .. -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows

cmake_minimum_required (VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE C:/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(VCPKG_TARGET_TRIPLET x64-windows)

#set(CMAKE_DEBUG_POSTFIX "_d")    
#set(CMAKE_RELEASE_POSTFIX "_r") 
#set_target_properties(${TARGET_NAME} PROPERTIES DEBUG_POSTFIX "_d") 
#set_target_properties(${TARGET_NAME} PROPERTIES RELEASE_POSTFIX "_r")
#set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project ("DreamRender")

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_subdirectory("opengl_utils")
add_subdirectory("core")

add_executable(DreamRender main.cpp)

target_include_directories(DreamRender PUBLIC 
    opengl_utils/
    opengl_utils/include
    embree/include
    core/include
    oidn/include
)

target_link_libraries(DreamRender PUBLIC
    glfw
    glad::glad
    glm::glm
    opengl_utils
    core
    nlohmann_json::nlohmann_json
    ../oidn/lib/OpenImageDenoise
    ../embree/lib/embree3
    ../embree/lib/tbb
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(TARGET_NAME DreamRender)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shader $<TARGET_FILE_DIR:${TARGET_NAME}>/../shader

    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/scene $<TARGET_FILE_DIR:${TARGET_NAME}>/scene

    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/embree/bin/embree3.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/embree3.dll

    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/embree/bin/tbb12.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/tbb12.dll

    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/oidn/bin/OpenImageDenoise.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/OpenImageDenoise.dll
)