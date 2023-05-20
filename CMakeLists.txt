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

add_subdirectory("opengl_utils")
add_subdirectory("core")
add_subdirectory("tbb")
add_subdirectory("embree")

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
    embree
    tbb
    tbbmalloc
    core
    ../oidn/lib/OpenImageDenoise
)
