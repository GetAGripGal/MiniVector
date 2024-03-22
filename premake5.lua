-- Options:
newoption {
    trigger = "win96",
    description = "Build for windows96.net"
}

-- Workspace
workspace "minivector"
configurations { "Debug", "Release" }
location "build"

-- GLFW project
project "glfw3"

language "C"
targetdir "build/bin/%{cfg.buildcfg}"

filter "not options:win96"
kind "SharedLib"
files {
    "vendor/glfw/include/GLFW/**.h",
    "vendor/glfw/src/**.c",
}
-- Include directories
includedirs {
    "vendor/glfw/include",
}

filter "options:win96"
kind "StaticLib"

-- Platform specific
filter { "system:linux", "not options:win96" }
defines { "_GLFW_X11" }
links {
    "X11",
    "Xrandr",
    "Xi",
    "Xxf86vm",
    "Xinerama",
    "Xcursor",
    "GL",
    "rt",
    "m",
    "dl",
    "pthread",
}

filter { "system:windows", "not options:win96" }
defines { "_GLFW_WIN32" }

links {
    "opengl32",
    "gdi32",

}

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"

filter "configurations:Release"
defines { "NDEBUG" }
optimize "On"


-- Win96 sdk project
filter "options:win96"
project "win96"
kind "StaticLib"
language "C"
targetdir "build/bin/%{cfg.buildcfg}"

filter "options:win96"
includedirs {
    "vendor/win96sdk/sdklib/include",
}
files {
    "vendor/win96sdk/sdklib/include/win96/**.h",
    "vendor/win96sdk/sdklib/src/**.c",
}

-- MicroVector project
project "minivector"
kind "ConsoleApp"
language "C"
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

    "vendor/win96sdk/sdklib/include",
}
libdirs {
    "build/bin/%{cfg.buildcfg}",
}


-- Windows 96
filter { "options:win96" }
defines { "_WIN96" }
buildoptions { "-pthread" }
linkoptions { " -s ASYNCIFY -s ASYNCIFY_IMPORTS=[\"emscripten_asm_const_int\"] -s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE_STRICT=0 -s ALLOW_TABLE_GROWTH -s ASSERTIONS=1" }
links { "win96" }

-- Platform specific
filter { "system:windows", "not options:win96" }
links {
    "glfw3",
    "opengl32",
    "gdi32",
}

filter { "system:linux", "not options:win96" }
links {
    "glfw3",
    "dl",
    "m"
}

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"

filter "configurations:Release"
defines { "NDEBUG" }
optimize "Speed"
