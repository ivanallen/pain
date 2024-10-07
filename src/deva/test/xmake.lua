add_defines("UNIT_TEST")
add_cxxflags("-fno-access-control")
add_packages("gtest")

target("test_deva_namespace")
    set_kind("binary")
    add_files("test_namespace.cc")
    add_files("../namespace.cc")
    add_tests("deva")
    add_deps("pain_base")
    add_deps("pain_core")
    add_packages("uuid_v4")
