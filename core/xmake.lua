add_requires("glm")
add_requires("glfw")
add_requires("glad")
add_requires("embree 3.13.5")

target("core")
    set_kind("static")
    add_files("*.cpp")
    add_packages("glm", "glfw", "glad", "embree")