add_requires("glm")
add_requires("glfw")
add_requires("glad")
add_requires("stb")
add_requires("tinyobjloader")
add_requires("embree 3.13.5")
add_requires("nlohmann_json")

target("core")
    set_kind("static")
    add_files("*.cpp")
    add_packages("glm", "glfw", "glad", "stb", "tinyobjloader", 
    "embree", "nlohmann_json")