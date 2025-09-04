#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <pain/base/plog.h>
#include <pain/base/spdlog_sink.h>

DEFINE_string(log_level, "debug", "Log level");

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    static pain::SpdlogSink s_spdlog_sink;
    logging::SetLogSink(&s_spdlog_sink);

    pain::LoggerOptions logger_options = {
        .file_name = "test_deva.log",
        .name = "deva",
        .level_log = spdlog::level::from_str(FLAGS_log_level),
        .async_threads = 1,
    };

    static auto s_flush_log = pain::make_logger(logger_options);

    pain::init_tracer("test_deva");
    auto stop_tracer = pain::make_scope_exit([]() {
        pain::cleanup_tracer();
    });

    auto ret = RUN_ALL_TESTS();
    PLOG_INFO(("desc", "test finished")("ret", ret));
    return ret;
}
