// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef SPDLOG_HEADER_ONLY
#include <gddeploy_spdlog/sinks/sink.h>
#endif

#include <gddeploy_spdlog/common.h>

SPDLOG_INLINE bool gddeploy_spdlog::sinks::sink::should_log(gddeploy_spdlog::level::level_enum msg_level) const
{
    return msg_level >= level_.load(std::memory_order_relaxed);
}

SPDLOG_INLINE void gddeploy_spdlog::sinks::sink::set_level(level::level_enum log_level)
{
    level_.store(log_level, std::memory_order_relaxed);
}

SPDLOG_INLINE gddeploy_spdlog::level::level_enum gddeploy_spdlog::sinks::sink::level() const
{
    return static_cast<gddeploy_spdlog::level::level_enum>(level_.load(std::memory_order_relaxed));
}
