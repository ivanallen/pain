#pragma once

#define TOKENPASTE(x, y) x##y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)

#define RUN(statement)                                                         \
  struct TOKENPASTE2(__run, __LINE__) {                                        \
    TOKENPASTE2(__run, __LINE__)() { statement; }                              \
  } TOKENPASTE2(TOKENPASTE2(__run, __LINE__), _instance)

#define ARGS(cmd, parent)                                                      \
  static argparse::ArgumentParser cmd##_parser(format_command(#cmd));          \
  parent.add_subparser(cmd##_parser);                                          \
  cmd##_parser.add_parents(parent);                                            \
  (cmd##_parser)

#define COMMAND(name)                                                          \
  butil::Status name(argparse::ArgumentParser &);                              \
  struct __add_##name {                                                        \
    __add_##name() { add(#name, name); }                                       \
  } __add_##name##_instance;                                                   \
  butil::Status name(argparse::ArgumentParser &args)

#define SPAN(span)                                                             \
  auto tracer = pain::base::get_tracer("sad");                                 \
  auto span = tracer->StartSpan(__func__);                                     \
  auto scope = tracer->WithActiveSpan(span);
