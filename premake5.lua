-- Workspace
workspace "minivector"
configurations { "Debug", "Release" }
location "build"



-- MicroVector project
project "minivector"
kind "ConsoleApp"
language "C++"
targetdir "build/bin/%{cfg.buildcfg}"

files {
    "source/**.h",
    "source/**.c",

    -- Glad
    "vendor/glad/include/glad/**.h",
    "vendor/glad/include/KHR/**.h",
    "vendor/glad/src/**.c",

    -- HandmadeMath
    "vendor/HandmadeMath/**.h",
}

-- Include directories
includedirs {
    "source",
    "vendor/glad/include",
    "vendor/HandmadeMath",
}

-- Link libraries
links {
    "glfw",
    "GL",
    "dl",
}

libdirs {
    os.findlib("glfw"),
}

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"

filter "configurations:Release"
defines { "NDEBUG" }
optimize "On"
