package("argparse")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/p-ranav/argparse")
    set_description("A single header argument parser for C++17")
    set_license("MIT")

    add_urls("https://github.com/p-ranav/argparse/archive/refs/tags/v$(version).zip",
             "https://github.com/p-ranav/argparse.git")
    add_versions("3.1", "3e5a59ab7688dcd1f918bc92051a10564113d4f36c3bbed3ef596c25e519a062")

    on_install(function (package)
        os.cp("include/argparse/argparse.hpp", package:installdir("include/argparse"))
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            void test() {
                argparse::ArgumentParser test("test");
            }
        ]]}, {configs = {languages = "c++17"}, includes = "argparse/argparse.hpp"}))
    end)
