package("gtest")

    set_homepage("https://github.com/google/googletest")
    set_description("Google Testing and Mocking Framework.")

    add_urls("https://github.com/google/googletest/archive/release-$(version).zip", {alias = "archive"})

    add_versions("1.12.1", "24564e3b712d3eb30ac9a85d92f7d720f60cc0173730ac166f27dda7fed76cb2")

    if is_plat("linux") then
        add_syslinks("pthread")
    end

    add_components("gtest")
    add_components("gtest_main")
    add_components("gmock")

    on_component("gtest", function (package, component)
        component:add("links", "gtest")
    end)

    on_component("gtest_main", function (package, component)
        component:add("links", "gtest_main")
        component:add("deps", "gtest")
    end)

    on_component("gmock", function (package, component)
        component:add("links", "gmock")
        component:add("deps", "gtest")
    end)

    on_install(function (package)
        io.writefile("xmake.lua", [[
            target("gtest")
                set_kind("static")
                set_languages("cxx11")
                add_files("googletest/src/gtest-all.cc")
                add_includedirs("googletest/include", "googletest")
                add_headerfiles("googletest/include/(**.h)")

            target("gtest_main")
                set_kind("static")
                set_languages("cxx11")
                add_files("googletest/src/gtest_main.cc")
                add_includedirs("googletest/include", "googletest")
                add_headerfiles("googletest/include/(**.h)")

            target("gmock")
                set_kind("static")
                set_languages("cxx11")
                add_files("googlemock/src/gmock-all.cc")
                add_includedirs("googlemock/include", "googlemock", "googletest/include", "googletest")
                add_headerfiles("googlemock/include/(**.h)")
        ]])
        import("package.tools.xmake").install(package)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            int factorial(int number) { return number <= 1 ? number : factorial(number - 1) * number; }
            TEST(FactorialTest, Zero) {
              testing::InitGoogleTest(0, (char**)0);
              EXPECT_EQ(1, factorial(1));
              EXPECT_EQ(2, factorial(2));
              EXPECT_EQ(6, factorial(3));
              EXPECT_EQ(3628800, factorial(10));
            }
        ]]}, {configs = {languages = "c++11"}, includes = "gtest/gtest.h"}))

        assert(package:check_cxxsnippets({test = [[
            using ::testing::AtLeast;

            class A {
            public:
                virtual void a_foo() { return; }
            };

            class mock_A : public A {
            public:
                MOCK_METHOD0(a_foo, void());
            };

            class B {
            public:
                A* target;
                B(A* param) : target(param) {}

                bool b_foo() { target->a_foo(); return true; }
            };

            TEST(test_code, step1) {
                mock_A a_obj;
                B b_obj(&a_obj);

                EXPECT_CALL(a_obj, a_foo()).Times(AtLeast(1));

                EXPECT_TRUE(b_obj.b_foo());
            }
        ]]}, {configs = {languages = "c++11"}, includes = {"gtest/gtest.h", "gmock/gmock.h"}}))
    end)
