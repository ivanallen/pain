package("uuid_v4")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/crashoz/uuid_v4.git")
    set_description("A single header argument parser for C++17")
    set_license("MIT")

    add_urls("https://github.com/crashoz/uuid_v4.git")
    add_versions("1.0.0", "bae4100d7a4b08fc48faecbc49f0fd135f6f2a24")

    on_load(function (package)
        package:add("cxxflags", "-march=native")
    end)

    on_install(function (package)
        import("package.tools.cmake").install(package)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            void test() {
                UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
                UUIDv4::UUID uuid = uuidGenerator.getUUID();
            }
        ]]}, {configs = {languages = "c++17"}, includes = "uuid_v4/uuid_v4.h"}))
    end)
