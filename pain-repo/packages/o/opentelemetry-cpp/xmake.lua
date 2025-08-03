package("opentelemetry-cpp")

    set_homepage("https://opentelemetry.io/")
    set_description("opentelemetry cpp library")

    add_urls("https://github.com/open-telemetry/opentelemetry-cpp.git", {submodules = false})
    add_versions("1.16.1", "baecbb95bd63df53e0af16e87bc683967962c5f8")


    add_deps("cmake")
    add_deps("libcurl")
    add_deps("protobuf-cpp")
    add_deps("nlohmann_json")
    add_deps("abseil")

    add_links("opentelemetry_version",
        "opentelemetry_exporter_in_memory",
        "opentelemetry_exporter_otlp_http",
        "opentelemetry_exporter_otlp_http_log",
        "opentelemetry_exporter_otlp_http_metric",
        "opentelemetry_exporter_ostream_logs",
        "opentelemetry_exporter_ostream_metrics",
        "opentelemetry_exporter_ostream_span",
        "opentelemetry_otlp_recordable",
        "opentelemetry_exporter_otlp_http_client",
        "opentelemetry_http_client_curl",
        "opentelemetry_proto",
        "opentelemetry_logs",
        "opentelemetry_metrics",
        "opentelemetry_trace",
        "opentelemetry_resources",
        "opentelemetry_common")

    on_load(function (package)
        package:add("defines",
            "OPENTELEMETRY_DEPRECATED_SDK_FACTORY",
            "HAVE_ABSEIL",
            "OPENTELEMETRY_ABI_VERSION_NO=2",
            "OPENTELEMETRY_STL_VERSION=2023")
    end)

    on_install("linux", function (package)
        local configs = {"-DWITH_OTLP_HTTP=ON"}
        table.insert(configs, "-DWITH_STL=ON")
        table.insert(configs, "-DBUILD_TESTING=OFF")
        table.insert(configs, "-DWITH_BENCHMARK=OFF")
        table.insert(configs, "-DWITH_EXAMPLES=OFF")
        table.insert(configs, "-DWITH_FUNC_TESTS=OFF")
        table.insert(configs, "-DWITH_ABSEIL=ON")
        table.insert(configs, "-DWITH_ABI_VERSION_1=OFF")
        table.insert(configs, "-DWITH_ABI_VERSION_2=ON")
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        import("package.tools.cmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({
            test = [[
                #include "opentelemetry/exporters/ostream/span_exporter_factory.h"
                #include "opentelemetry/exporters/otlp/otlp_http_exporter_factory.h"
                #include "opentelemetry/exporters/otlp/otlp_http_exporter_options.h"
                #include "opentelemetry/sdk/trace/exporter.h"
                #include "opentelemetry/sdk/trace/processor.h"
                #include "opentelemetry/sdk/trace/simple_processor_factory.h"
                #include "opentelemetry/sdk/trace/tracer_provider_factory.h"
                #include "opentelemetry/trace/provider.h"
                namespace trace_api = opentelemetry::trace;
                namespace trace_sdk = opentelemetry::sdk::trace;
                namespace trace_exporter = opentelemetry::exporter::trace;
                namespace otlp = opentelemetry::exporter::otlp;
                void test() {
                    otlp::OtlpHttpExporterOptions opts;
                    opts.url = "http://localhost:4318/v1/traces";
                    auto exporter = otlp::OtlpHttpExporterFactory::Create(opts);
                    auto processor = trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
                    std::shared_ptr<opentelemetry::trace::TracerProvider> provider =
                        trace_sdk::TracerProviderFactory::Create(std::move(processor));
                    //set the global trace provider
                    trace_api::Provider::SetTracerProvider(provider);
                }
            ]]
        }, {configs = {languages = "c++20"}}))
    end)
