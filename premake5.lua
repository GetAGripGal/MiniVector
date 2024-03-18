-- Workspace
workspace "minivector"
configurations { "Debug", "Release" }
location "build"

-- GLFW project
project "glfw"
kind "SharedLib"
language "C"
targetdir "build/bin/%{cfg.buildcfg}"

files {
    "vendor/glfw/include/GLFW/**.h",
    "vendor/glfw/src/**.c",
}

-- Include directories
includedirs {
    "vendor/glfw/include",
}

-- Link libraries
links {
    "GL",
    "X11",
    "Xrandr",
    "Xi",
    "Xxf86vm",
    "Xinerama",
    "Xcursor",
    "rt",
    "m",
    "dl",
    "pthread",
}

-- Platform specific
filter "system:linux"
defines { "_GLFW_X11" }

filter "system:windows"
defines { "_GLFW_WIN32" }

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"

filter "configurations:Release"
defines { "NDEBUG" }
optimize "On"


-- MicroVector project
project "minivector"
kind "ConsoleApp"
language "C++"
targetdir "build/bin/%{cfg.buildcfg}"

files {
    "source/**.h",
    "source/**.c",
    "source/legacy/**.h",
    "source/legacy/**.c",

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
    "source/legacy",

    "vendor/glad/include",
    "vendor/HandmadeMath",
    "vendor/glfw/include",
}

-- Link libraries
links {
    "glfw",
    "dl",
}

-- Now we need to add the OpenGL system libraries

filter { "system:windows" }
links { "OpenGL32" }

filter { "system:not windows" }
links { "GL" }

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"

filter "configurations:Release"
defines { "NDEBUG" }
optimize "On"
