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

#include "client/resource/atlas/AtlasSource.hpp"
#include "common/core/Result.hpp"
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::client::resource::atlas {

/**
 * @brief atlas source JSON 解析器
 *
 * 对齐原版 SpriteSources.CODEC 的 type dispatch。
 * 解析 atlas JSON 的 "sources" 数组，每个 source 对象用 "type" 字段
 * dispatch 到 5 种具体 source。
 */
struct AtlasSourceParser {
    /// 解析单个 source JSON 对象（含 "type" 字段 dispatch）
    [[nodiscard]] static Result<std::unique_ptr<AtlasSource>> parseSource(const nlohmann::json& j);

    /// 解析整个 atlas JSON：{"sources": [...]}
    [[nodiscard]] static Result<std::vector<std::unique_ptr<AtlasSource>>> parseAtlasJson(const nlohmann::json& j);

    /// 从 JSON 文本解析整个 atlas JSON
    [[nodiscard]] static Result<std::vector<std::unique_ptr<AtlasSource>>> parseAtlasText(std::string_view jsonText);
};

} // namespace mc::client::resource::atlas
