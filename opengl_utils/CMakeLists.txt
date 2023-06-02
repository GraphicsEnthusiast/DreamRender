project ("opengl_utils")

set(CMAKE_CXX_STANDARD 17)

add_library(opengl_utils src/RenderPass.cpp src/Shader.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp
    imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_impl_glfw.cpp imgui/imgui_impl_opengl3.cpp)

target_include_directories(opengl_utils
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    ../dep/glad/include
    ../dep/glfw3/include
    ../dep/glm/include
)

target_link_libraries(opengl_utils PUBLIC
    ../dep/glfw3/lib/glfw3dll
    ../dep/glad/lib/glad
)