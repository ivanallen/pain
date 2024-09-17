package("brpc")
    set_homepage("https://github.com/apache/incubator-brpc")
    set_description("brpc is an Industrial-grade RPC framework using C++ Language, which is often used in high performance system such as Search, Storage, Machine learning, Advertisement, Recommendation etc.")

    add_urls("https://github.com/apache/brpc.git")
    add_versions("1.10.0", "046fd435c7377af2827349bde71b5736ca172406")
    add_patches("1.10.0", path.join(os.scriptdir(), "patches", "1.10.0", "cmake.patch"))

    -- we enable zlib in protobuf-cpp, because brpc need google/protobuf/io/gzip_stream.h
    add_deps("protobuf-cpp")
    add_deps("leveldb", "gflags", "openssl", "libzip", "snappy", "zlib")
    add_deps("cmake")

    if is_plat("macosx") then
        add_frameworks("Foundation", "CoreFoundation", "Security", "CoreGraphics", "CoreText")
        add_ldflags("-Wl,-U,_ProfilerStart", "-Wl,-U,_ProfilerStop")
    elseif is_plat("linux") then
        add_syslinks("rt", "dl", "pthread")
    end

    on_install("linux", "macosx", function (package)
        local configs = {"-DWITH_DEBUG_SYMBOLS=OFF", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DWITH_SNAPPY=ON"}
        -- io.replace("CMakeLists.txt", 'set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS}', 'set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CPP_FLAGS}', {plain = true})
        -- io.replace("CMakeLists.txt", 'set(CMAKE_C_FLAGS "${CMAKE_CPP_FLAGS}', 'set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CPP_FLAGS}', {plain = true})
        import("package.tools.cmake").install(package, configs, {packagedeps = "zlib"})
        if not package:config("shared") then
            os.rm(package:installdir("lib/*.dylib"))
            os.rm(package:installdir("lib/*.so"))
        end
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({
            test = [[
              #include <brpc/server.h>
              static void test() {
                brpc::Server server;
              }
            ]]
        }, {configs = {languages = "c++11"}}))
    end)
