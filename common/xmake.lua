add_requires("glm")
add_requires("glfw")
add_requires("glad")

target("common")
    set_kind("static")
    add_files("*.cpp")
    add_packages("glm", "glfw", "glad")