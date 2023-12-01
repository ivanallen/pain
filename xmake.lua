set_project("pain")
set_languages("c++23")

set_version("1.0.0", {build = "%Y%m%d"})

add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})
set_policy("build.across_targets_in_parallel", false)
set_policy("check.auto_ignore_flags", false)

add_includedirs("include")

includes("src")
