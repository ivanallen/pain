target("asura")
    set_kind("binary")
    add_files("**.cc")
    add_deps("pain_base")
    add_deps("pain_core")
    add_deps("pain_common")
    add_packages("uuid_v4")
    add_packages("brpc")
