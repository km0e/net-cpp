set_project("xsl")
set_xmakever("2.5.1")
set_version("0.1.0", {build = "%Y%m%d%H%M"})

-- add release , debug and coverage modes
add_rules("mode.debug", "mode.release", "mode.coverage")


toolchain("gcc")
    on_load(function (toolchain)
        if is_mode("debug") then
            toolchain:set_warnings("all", "error", 'pedantic', 'extra')
        end
    end)
toolchain_end()

toolchain("clang")
    on_load(function (toolchain)
        if is_mode("debug") then
            toolchain:set_warnings("all", "error", 'pedantic', 'extra')
        end
    end)
toolchain_end()

toolchain("msvc")
    on_load(function (toolchain)
        if is_mode("debug") then
            set_warnings("/W4", "/WX")
            add_defines("_SILENCE_CXX20_DEPRECATED_WARNING")
        end
    end)
toolchain_end()

set_languages("c++20")


add_requires("thread-pool", "spdlog", "cli11", "fmt")

-- add_requires("boost")

-- add_requires("abseil")

add_ldflags("-fuse-ld=mold")

add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

add_includedirs("$(projectdir)/include")

includes("include", "source", "test")
