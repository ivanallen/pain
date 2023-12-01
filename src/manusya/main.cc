#include <format>
#include <iostream>
#include <spdlog/spdlog.h>

int main() {
  std::cout << std::format("{} + {} = {}", 1, 2, 3) << std::endl;
  SPDLOG_INFO("{}", "hello manusya");
  return 0;
}