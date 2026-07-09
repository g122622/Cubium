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
namespace fluid {
class FluidState;
}
namespace world {
namespace gen {
namespace feature {
namespace parser {

/**
 * @brief 流体状态 JSON 解析器
 *
 * 解析数据包中形如 `{"Name":"minecraft:water","Properties":{"falling":"true"}}` 的流体状态对象，
 * 返回对应 FluidState 指针。无 Properties 时返回流体默认状态。
 *
 * 用于 spring_feature 的 state 字段（MC 中 SpringConfiguration.state 为 FluidState，而非 BlockState）。
 * 流体的 falling 等属性是流体属性，不能用 BlockStateParser 解析（方块没有这些属性）。
 */
namespace FluidStateParser {

/**
 * @brief 解析流体状态对象
 * @param stateObj JSON 对象，必须含 "Name" 字符串字段，可选 "Properties" 对象字段
 * @return 流体状态指针；Name 缺失或流体未注册时返回 Error
 */
[[nodiscard]] Result<const fluid::FluidState*> parse(const nlohmann::json& stateObj);

} // namespace FluidStateParser

} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
