set_project("pain")
set_languages("c++23")

set_version("1.0.0", {build = "%Y%m%d"})

add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})
set_policy("build.across_targets_in_parallel", false)
set_policy("check.auto_ignore_flags", false)

add_repositories("pain-repo pain-repo")

if is_mode("debug") then
    add_cxxflags("-fsanitize=address")
    add_ldflags("-fsanitize=address")
end

add_requires("protobuf-cpp", {configs = {zlib = true}})
add_requireconfs("**.protobuf-cpp", {override=true, configs = {zlib = true}})

add_requires("brpc")
add_requires("fmt")
add_requires("spdlog")
add_requires("boost")
add_requires("gtest")
add_requires("argparse")
add_requires("fmt")
add_requires("nlohmann_json")

add_includedirs("include")
add_includedirs("src")

includes("src")
