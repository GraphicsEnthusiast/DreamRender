add_rules("mode.debug", "mode.release")
set_languages("c++17")

add_requires("glm")
add_requires("glfw")
add_requires("glad")
add_requires("stb")
add_requires("tinyobjloader")
add_requires("embree 3.13.5")
add_requires("nlohmann_json")
--add_requires("openvdb 11.0.0")

add_includedirs("common")
add_includedirs("core")

includes("common")
includes("core")

target("DreamRender")
    set_kind("binary")
    add_deps("common")
    add_deps("core")
    add_files("main.cpp")
    add_packages("glm", "glfw", "glad", "stb", "tinyobjloader", 
    "embree", --[["openvdb",--]] "nlohmann_json")
    after_build(function (target)
        local targetDir = target:targetdir()
        os.cp("shader", targetDir)
    end)
