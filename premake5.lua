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
project "glfw"

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


    -- Windows 96
    filter "not options:win96"
        kind "SharedLib"

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

        filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

        filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

-- Win96 sdk project
project "win96"
    kind "StaticLib"
    language "C"
    targetdir "build/bin/%{cfg.buildcfg}"

    includedirs {
        "vendor/win96sdk/sdklib/include",
    }

    filter "options:win96"
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


    -- Windows 96
    filter { "options:win96" }
        defines { "_WIN96" }
        buildoptions { "-pthread" }
        linkoptions { "-s USE_GLFW=3 -USE_PTHREADS=1" }
        links { "win96" }

    -- Platform specific
    filter { "system:windows", "not options:win96" }
        links { "OpenGL32" }

    filter { "system:linux", "not options:win96" }
        links {
            "glfw",
            "dl",
            "m"
        }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
