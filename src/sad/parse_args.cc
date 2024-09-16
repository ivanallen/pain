#include <argparse/argparse.hpp>

namespace pain::sad {

argparse::ArgumentParser program("program_name");

namespace manusya {
argparse::ArgumentParser manusya_parser("manusya");
argparse::ArgumentParser surface_parser("surface", "1.0",
                                        argparse::default_arguments::none);
argparse::ArgumentParser create_chunk_parser("create-chunk");
argparse::ArgumentParser append_chunk_parser("append-chunk");
void init() {
  manusya_parser.add_description("send cmd to manusya server");
  manusya_parser.add_subparser(create_chunk_parser);
  manusya_parser.add_subparser(append_chunk_parser);
  surface_parser.add_argument("-h", "--host")
      .default_value(std::string("127.0.0.1:8003"))
      .help("ip port of this server");
  create_chunk_parser.add_parents(surface_parser);
  create_chunk_parser.add_description("create chunk");
  append_chunk_parser.add_parents(surface_parser);
  append_chunk_parser.add_description("append chunk");
  append_chunk_parser.add_argument("-c", "--chunk-id")
      .required()
      .help("chunk id");
}
} // namespace manusya

// clang-format off
void init(int argc, char *argv[]) {
  program.add_argument("--log-level").default_value(std::string("debug")).required().help("specify the log level");
  program.add_subparser(manusya::manusya_parser);

  program.add_argument("--log-level")
      .default_value(std::string("debug"))
      .required()
      .help("specify the log level");

  manusya::init();

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }
}
// clang-format off

} // namespace pain::sad