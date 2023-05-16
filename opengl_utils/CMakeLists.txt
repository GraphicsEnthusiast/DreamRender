project ("opengl_utils")

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_library(opengl_utils src/RenderPass.cpp src/Shader.cpp)

target_include_directories(opengl_utils
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(opengl_utils PUBLIC
glfw
glad::glad
glm::glm
)