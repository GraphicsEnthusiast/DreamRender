project ("core")

set(CMAKE_CXX_STANDARD 17)

add_library(core 
    src/Camera.cpp 
    src/Shape.cpp
    src/Light.cpp
    src/Material.cpp
    src/Texture.cpp
    src/Scene.cpp
    src/Integrator.cpp
    src/Render.cpp
    src/SceneParser.cpp
    src/Sampler.cpp
    src/Medium.cpp
)

target_include_directories(core PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
    ../dep/embree/include
    ../opengl_utils/include
    ../opengl_utils
    ../dep/oidn/include
    ../dep/glad/include
    ../dep/glfw3/include
    ../dep/glm/include
    ../dep/nlohmann-json/include
)

target_link_libraries(core PUBLIC
    opengl_utils
    ../dep/embree/lib/embree3
    ../dep/embree/lib/tbb
    ../dep/glfw3/lib/glfw3dll
    ../dep/glad/lib/glad
)