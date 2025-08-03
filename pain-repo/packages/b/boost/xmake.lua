package("boost")

    set_homepage("https://www.boost.org/")
    set_description("Collection of portable C++ source libraries.")
    set_license("BSL-1.0")

    -- https://archives.boost.io/release/1.81.0/source/boost_1_81_0.tar.bz2
    add_urls("https://archives.boost.io/release/$(version).tar.bz2", {version = function (version)
        return version .. "/source/boost_" .. (version:gsub("%.", "_"))
    end})

    add_versions("1.81.0", "71feeed900fbccca04a3b4f2f84a7c217186f28a940ed8b7ed4725986baf99fa")

    add_configs("multi", { description = "Enable multi-thread support.",  default = true, type = "boolean"})
    add_configs("pyver", {description = "python version x.y, etc. 3.10", default = "3.10"})

    if is_plat("mingw") and is_subhost("msys") then
        add_extsources("pacman::boost")
    elseif is_plat("linux") then
        add_extsources("pacman::boost", "apt::libboost-all-dev")
    elseif is_plat("macosx") then
        add_extsources("brew::boost")
    end

    if is_plat("linux") then
        add_deps("bzip2", "zlib")
        add_syslinks("pthread", "dl")
    end

    local libnames = {"fiber",
                      "coroutine",
                      "context",
                      "regex",
                      "system",
                      "container",
                      "exception",
                      "timer",
                      "atomic",
                      "graph",
                      "serialization",
                      "random",
                      "wave",
                      "date_time",
                      "locale",
                      "iostreams",
                      "program_options",
                      "test",
                      "chrono",
                      "contract",
                      "graph_parallel",
                      "json",
                      "log",
                      "thread",
                      "filesystem",
                      "math",
                      "mpi",
                      "nowide",
                      "python",
                      "stacktrace",
                      "type_erasure"}

    local sublibs = {log = {"log_setup", "log"}, stacktrace = {"stacktrace_backtrace", "stacktrace_basic"}}

    for _, libname in ipairs(libnames) do
        add_components(libname)
        on_component(libname, function (package, component)
            function get_linkname(package, libname)
                if libname == "test" then
                    libname = "unit_test_framework"
                end

                local linkname
                if package:is_plat("windows") then
                    linkname = (package:config("shared") and "boost_" or "libboost_") .. libname
                else
                    linkname = "boost_" .. libname
                end
                if libname == "python" then
                    linkname = linkname .. package:config("pyver"):gsub("%p+", "")
                end
                if package:config("multi") then
                    linkname = linkname .. "-mt"
                end
                if package:is_plat("windows") then
                    local vs_runtime = package:config("vs_runtime")
                    if package:config("shared") then
                        if package:debug() then
                            linkname = linkname .. "-gd"
                        end
                    elseif vs_runtime == "MT" then
                        linkname = linkname .. "-s"
                    elseif vs_runtime == "MTd" then
                        linkname = linkname .. "-sgd"
                    elseif vs_runtime == "MDd" then
                        linkname = linkname .. "-gd"
                    end
                end
                return linkname
            end

            local libs = sublibs[libname]
            if libs then
                for _, lib in ipairs(libs) do
                    component:add("links", get_linkname(package, lib))
                end
            else
                component:add("links", get_linkname(package, libname))
            end
        end)
    end


    on_load(function (package)
        -- disable auto-link all libs
        if package:is_plat("windows") then
            package:add("defines", "BOOST_ALL_NO_LIB")
        end

        if package:config("python") then
            if not package:config("shared") then
                package:add("defines", "BOOST_PYTHON_STATIC_LIB")
            end
            package:add("deps", "python " .. package:config("pyver") .. ".x", {configs = {headeronly = true}})
        end
    end)

    on_install("macosx", "linux", "windows", "bsd", "mingw", "cross", function (package)
        import("core.base.option")

        -- force boost to compile with the desired compiler
        local file = io.open("user-config.jam", "a")
        if file then
            if package:is_plat("macosx") then
                -- we uses ld/clang++ for link stdc++ for shared libraries
                -- and we need `xcrun -sdk macosx clang++` to make b2 to get `-isysroot` automatically
                local cc = package:build_getenv("ld")
                if cc and cc:find("clang", 1, true) and cc:find("Xcode", 1, true) then
                    cc = "xcrun -sdk macosx clang++"
                end
                file:print("using darwin : : %s ;", cc)
            elseif package:is_plat("windows") then
                file:print("using msvc : : \"%s\" ;", (package:build_getenv("cxx"):gsub("\\", "\\\\")))
            else
                file:print("using gcc : : %s ;", package:build_getenv("cxx"):gsub("\\", "/"))
            end
            file:close()
        end

        local bootstrap_argv =
        {
            "--prefix=" .. package:installdir(),
            "--libdir=" .. package:installdir("lib"),
            "--without-icu"
        }
        if package:is_plat("windows") then
            import("core.tool.toolchain")
            local runenvs = toolchain.load("msvc"):runenvs()
            -- for bootstrap.bat, all other arguments are useless
            bootstrap_argv = { "msvc" }
            os.vrunv("bootstrap.bat", bootstrap_argv, {envs = runenvs})
        elseif package:is_plat("mingw") and is_host("windows") then
            bootstrap_argv = { "gcc" }
            os.vrunv("bootstrap.bat", bootstrap_argv)
            -- todo looking for better solution to fix the confict between user-config.jam and project-config.jam
            io.replace("project-config.jam", "using[^\n]+", "")
        else
            os.vrunv("./bootstrap.sh", bootstrap_argv)
        end
        os.vrun("./b2 headers")

        local njobs = option.get("jobs") or tostring(os.default_njob())
        local argv =
        {
            "--prefix=" .. package:installdir(),
            "--libdir=" .. package:installdir("lib"),
            "-d2",
            "-j" .. njobs,
            "--hash",
            "--layout=tagged-1.66", -- prevent -x64 suffix in case cmake can't find it
            "--user-config=user-config.jam",
            "-sNO_LZMA=1",
            "-sNO_ZSTD=1",
            "install",
            "threading=" .. (package:config("multi") and "multi" or "single"),
            "debug-symbols=" .. (package:debug() and "on" or "off"),
            "link=" .. (package:config("shared") and "shared" or "static")
        }

        if package:config("lto") then
            table.insert(argv, "lto=on")
        end
        if package:is_arch("aarch64", "arm+.*") then
            table.insert(argv, "architecture=arm")
        end
        if package:is_arch(".+64.*") then
            table.insert(argv, "address-model=64")
        else
            table.insert(argv, "address-model=32")
        end
        if package:is_plat("windows") then
            local vs_runtime = package:config("vs_runtime")
            if package:config("shared") then
                table.insert(argv, "runtime-link=shared")
            elseif vs_runtime and vs_runtime:startswith("MT") then
                table.insert(argv, "runtime-link=static")
            else
                table.insert(argv, "runtime-link=shared")
            end
            table.insert(argv, "cxxflags=-std:c++14")
            table.insert(argv, "toolset=msvc")
        elseif package:is_plat("mingw") then
            table.insert(argv, "toolset=gcc")
        else
            table.insert(argv, "cxxflags=-std=c++14")
            if package:config("pic") ~= false then
                table.insert(argv, "cxxflags=-fPIC")
            end
        end
        for _, libname in ipairs(libnames) do
            table.insert(argv, "--with-" .. libname)
        end
        os.vrunv("./b2", argv)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <boost/algorithm/string.hpp>
            #include <string>
            #include <vector>
            static void test() {
                std::string str("a,b");
                std::vector<std::string> vec;
                boost::algorithm::split(vec, str, boost::algorithm::is_any_of(","));
            }
        ]]}, {configs = {languages = "c++14"}}))

        if package:config("date_time") then
            assert(package:check_cxxsnippets({test = [[
                #include <boost/date_time/gregorian/gregorian.hpp>
                static void test() {
                    boost::gregorian::date d(2010, 1, 30);
                }
            ]]}, {configs = {languages = "c++14"}}))
        end
    end)
