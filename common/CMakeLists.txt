project ("common")

set(CMAKE_CXX_STANDARD 17)

add_library(common STATIC
    Shader.h
    Shader.cpp
    RenderPass.h
    RenderPass.cpp
)
target_include_directories(common PUBLIC
    ../external/glad/include
    ../external/glfw3/include
    ../external/glm/include
)

target_link_libraries(common PUBLIC
    ../external/glfw3/lib/glfw3dll
    ../external/glad/lib/glad
)