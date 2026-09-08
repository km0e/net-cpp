set_project("xsl")
set_xmakever("3.0.0")
set_version("0.1.0", { build = "%Y%m%d%H%M" })

-- add release, debug and coverage modes
add_rules(
    "mode.debug",
    "mode.release",
    "mode.coverage",
    "mode.valgrind",
    "plugin.compile_commands.autoupdate",
    { outputdir = "." }
)

set_warnings("everything")
-- set_warnings("all", "error", 'pedantic', 'extra')

set_languages("cxx26")

if is_mode("release") then
    set_optimize("fastest")
    set_policy("build.optimization.lto", true)
end

-- Static libraries never link, so ldflags only take effect on binary targets.
add_ldflags("-fuse-ld=mold", { force = true })

-- Keep in sync with cmake/deps.cmake (quill pinned to match CPM's v10.0.1,
-- gtest main=true to match GTest::gtest_main).
add_requires("quill 10.0.1", "cli11", "openssl3", "asio")
add_requires("gtest 1.17.0", { configs = { main = true }, system = false })

-- Compile-time log level; INFO by default, same as CMake's XSL_LOG_LEVEL.
option("log_level")
do
    set_default("info")
    set_values("none", "trace", "debug", "info", "warning", "error", "critical")
    set_description("Compile-time log level (QUILL_COMPILE_ACTIVE_LOG_LEVEL).")
    after_check(function(option)
        -- quill LogMacros.h: TRACE_L3=0 .. CRITICAL=8, no NONE constant;
        -- none -> 9 (= CRITICAL + 1) compiles every log statement out.
        local map = {
            none = "9",
            trace = "QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L1",
            debug = "QUILL_COMPILE_ACTIVE_LOG_LEVEL_DEBUG",
            info = "QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO",
            warning = "QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING",
            error = "QUILL_COMPILE_ACTIVE_LOG_LEVEL_ERROR",
            critical = "QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL",
        }
        option:add("defines", "QUILL_COMPILE_ACTIVE_LOG_LEVEL=" .. map[option:value()], { public = true })
    end)
end

-- Opt-in unity (jumbo) build: faster full rebuilds, worse incrementals.
-- xmake 3.1.1: add_rules() is a target-scope API, so targets opt in by
-- calling xsl_enable_unity() inside their target block.
option("unity")
do
    set_default(false)
    set_showmenu(true)
    set_description("Enable unity build (faster clean builds, worse incremental).")
end

function xsl_enable_unity()
    if has_config("unity") then
        add_rules("c++.unity_build", { batchsize = 8 })
    end
end

-- xsl_log: quill bootstrap + log macros; every module links this PUBLIC.
target("xsl_log")
do
    set_kind("static")
    add_files("src/log.cpp")
    xsl_enable_unity()
    add_includedirs("$(projectdir)/include", { public = true })
    add_options("log_level", { public = true })
    add_packages("quill", { public = true })
end

-- Umbrella target; pulls in every module (as in CMake, OpenSSL/quill reach
-- consumers transitively through xsl_asio / xsl_log).
target("xsl")
do
    set_kind("static")
    add_deps("xsl_sys", "xsl_net", "xsl_coro", "xsl_wheel", "xsl_asio")
    add_headerfiles("$(projectdir)/include/(xsl/**.h)")
    add_includedirs("$(projectdir)/include", { public = true })
end

includes("src")
includes("test")
includes("examples")
