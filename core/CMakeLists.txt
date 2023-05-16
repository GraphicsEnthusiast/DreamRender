project ("core")

set(CMAKE_CXX_STANDARD 17)

find_package(glm CONFIG REQUIRED)

add_library(core 
    src/Camera.cpp 
    src/Shape.cpp
    src/Light.cpp
    src/Material.cpp
    src/Texture.cpp
    src/Scene.cpp
    src/Integrator.cpp
    src/Render.cpp
)

target_include_directories(core PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
    ../embree/include
    ../opengl_utils/include
    ../oidn/include
)

target_link_libraries(core PUBLIC
    glm::glm
    embree
)