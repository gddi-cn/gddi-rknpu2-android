
#include "common/logger.h"

#include <cstdlib>
#include <iostream>

namespace gddeploy{

static void LoadEnvLevels() {
  auto p = std::getenv("SPDLOG_LEVEL");
  if (p) {
    const std::string str(p);
    if (str == "trace") {
      gddeploy_spdlog::set_level(gddeploy_spdlog::level::trace);
    } else if (str == "debug") {
      gddeploy_spdlog::set_level(gddeploy_spdlog::level::debug);
    } else if (str == "info") {
      gddeploy_spdlog::set_level(gddeploy_spdlog::level::info);
    } else if (str == "warn") {
      gddeploy_spdlog::set_level(gddeploy_spdlog::level::warn);
    } else if (str == "err") {
      gddeploy_spdlog::set_level(gddeploy_spdlog::level::err);
    } else if (str == "critical") {
      gddeploy_spdlog::set_level(gddeploy_spdlog::level::critical);
    } else if (str == "off") {
      gddeploy_spdlog::set_level(gddeploy_spdlog::level::off);
    }
  } else {
    gddeploy_spdlog::set_level(gddeploy_spdlog::level::err);
  }
  // gddeploy_spdlog::set_level(gddeploy_spdlog::level::err);
}

std::shared_ptr<gddeploy_spdlog::logger> CreateDefaultLogger() {
  LoadEnvLevels();
  constexpr const auto logger_name = "gddeploy";
  std::shared_ptr<gddeploy_spdlog::logger> ptr;
  if (!gddeploy_spdlog::get(logger_name)) {
    // 日志器不存在，创建一个新的
      ptr = gddeploy_spdlog::stdout_color_mt(logger_name);
  }
  else {
      // 日志器已存在，直接获取
      ptr = gddeploy_spdlog::get(logger_name);
  }
  return ptr;
}

static std::shared_ptr<gddeploy_spdlog::logger> s_ptr = CreateDefaultLogger(); 

gddeploy_spdlog::logger *GetLogger() {
  return s_ptr.get();
}

}