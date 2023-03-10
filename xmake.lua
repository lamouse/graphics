add_rules("mode.debug", "mode.release")
set_languages("c++23")
add_requires("vulkan", "glfw", "glm")


function copy(file) 
    after_build(function (target)
        os.cp(file, target:targetdir())
    end)
end


target("graphics")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("vulkan", "glfw", "glm")
    add_defines("VK_USE_PLATFORM_MACOS_MVK")
    copy("shader/*.spv")
    