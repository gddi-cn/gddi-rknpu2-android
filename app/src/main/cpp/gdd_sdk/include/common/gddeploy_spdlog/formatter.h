// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <gddeploy_spdlog/fmt/fmt.h>
#include <gddeploy_spdlog/details/log_msg.h>

namespace gddeploy_spdlog {

class formatter
{
public:
    virtual ~formatter() = default;
    virtual void format(const details::log_msg &msg, memory_buf_t &dest) = 0;
    virtual std::unique_ptr<formatter> clone() const = 0;
};
} // namespace gddeploy_spdlog
