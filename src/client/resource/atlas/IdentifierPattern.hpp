/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <regex>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::client::resource::atlas {

/**
 * @brief 资源位置匹配模式
 *
 * 对齐原版 IdentifierPattern，用于 filter source。
 * namespace 和 path 均为可选正则；缺省侧匹配任意。
 * 最终 locationPredicate = ns 匹配 && path 匹配。
 */
struct IdentifierPattern {
    std::optional<std::regex> namespaceRegex;
    std::optional<std::regex> pathRegex;

    /// 测试资源位置是否匹配
    [[nodiscard]] bool matches(const ResourceLocation& loc) const;

    /// 从 filter source 的 "pattern" JSON 对象解析
    [[nodiscard]] static Result<IdentifierPattern> parse(const nlohmann::json& j);
};

} // namespace mc::client::resource::atlas
