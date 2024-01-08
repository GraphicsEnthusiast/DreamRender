project ("core")

set(CMAKE_CXX_STANDARD 17)

add_library(core STATIC
    Camera.cpp
    Camera.h
    Filter.cpp
    Filter.h
    Fresnel.cpp
    Fresnel.h
    Integrator.cpp
    Integrator.h
    Light.cpp
    Light.h
    Material.cpp
    Material.h
    Medium.cpp
    Medium.h
    Microfacet.cpp
    Microfacet.h
    PhaseFunction.cpp
    PhaseFunction.h
    PostProcessing.cpp
    PostProcessing.h
    Ray.cpp
    Ray.h
    Renderer.cpp
    Renderer.h
    Sampler.cpp
    Sampler.h
    Sampling.cpp
    Sampling.h
    Scene.cpp
    Scene.h
    Shape.cpp
    Shape.h
    SobolMatrices1024x52.h
    Spectrum.cpp
    Spectrum.h
    Texture.cpp
    Texture.h
    Transform.cpp
    Transform.h
    Utils.h
)

target_include_directories(core PUBLIC 
    ../common
    ../external/embree/include
    ../external/stb/include
    ../external/tinyobjloader/include
    ../external/glad/include
    ../external/glfw3/include
    ../external/glm/include
    ../external/nlohmann_json/include
)

target_link_libraries(core PUBLIC
    common
    openvdb
    ../external/embree/lib/embree3
    ../external/embree/lib/tbb
    ../external/glfw3/lib/glfw3dll
    ../external/glad/lib/glad
)