target("pain")
    set_kind("static")
    add_files("**.cc")
    add_deps("core")
    add_deps("base")
    add_packages("protobuf-cpp", {public = true})