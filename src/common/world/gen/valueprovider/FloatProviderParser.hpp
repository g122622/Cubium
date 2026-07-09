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
#include "common/world/gen/valueprovider/FloatProvider.hpp"

#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace valueprovider {

/**
 * @brief FloatProvider JSON 反序列化工具
 *
 * 从数据包 JSON 解析 FloatProvider，支持 MC 1.21.11 数据包格式。
 *
 * 支持的格式：
 * - 裸数字简写: 0.5 -> ConstantFloat(0.5)
 * - 常量: { "type": "minecraft:constant", "value": 0.5 }
 * - 均匀分布: { "type": "minecraft:uniform",
 *               "value": { "min_inclusive": 0.7, "max_exclusive": 1.4 } }
 *   或扁平: { "type": "minecraft:uniform", "min_inclusive": 0.7, "max_exclusive": 1.4 }
 * - 梯形分布: { "type": "minecraft:trapezoid",
 *               "value": { "min": 0.0, "max": 6.0, "plateau": 2.0 } }
 *   或扁平: { "type": "minecraft:trapezoid", "min": 0.0, "max": 6.0, "plateau": 2.0 }
 *
 * 命名空间前缀 "minecraft:" 可选。
 */
namespace FloatProviderParser {

/**
 * @brief 从 JSON 解析 FloatProvider
 *
 * @param json JSON 值，可以是裸数字或类型分派对象
 * @return 解析后的 FloatProvider，或错误
 */
[[nodiscard]] Result<std::unique_ptr<FloatProvider>> parse(const nlohmann::json& json);

} // namespace FloatProviderParser

} // namespace valueprovider
} // namespace gen
} // namespace world
} // namespace mc
