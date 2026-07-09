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
 *
 */

#pragma once

#include "common/core/Result.hpp"
#include <nlohmann/json_fwd.hpp>

namespace mc {

class BlockState;

namespace world {
namespace gen {
namespace feature {
namespace parser {

/**
 * @brief 方块状态 JSON 解析器
 *
 * 解析数据包中形如 `{"Name":"minecraft:oak_log","Properties":{"axis":"y"}}` 的方块状态对象，
 * 返回对应 BlockState 指针。无 Properties 时返回方块默认状态。
 *
 * 这是 BlockStateProviderParser / RuleTestParser 等更上层解析器复用的基础工具。
 */
namespace BlockStateParser {

/**
 * @brief 解析方块状态对象
 * @param stateObj JSON 对象，必须含 "Name" 字符串字段，可选 "Properties" 对象字段
 * @return 方块状态指针；Name 缺失或方块未注册时返回 Error
 */
[[nodiscard]] Result<const BlockState*> parse(const nlohmann::json& stateObj);

} // namespace BlockStateParser

} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
