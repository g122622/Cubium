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
#include "common/world/gen/valueprovider/IntProvider.hpp"

#include <memory>
#include <optional>
#include <nlohmann/json.hpp>

namespace mc {
namespace world {
namespace gen {
namespace valueprovider {

/**
 * @brief IntProvider JSON 反序列化工具
 *
 * 从 JSON 数据解析 IntProvider 对象，支持 MC 1.21.11 数据包格式。
 *
 * 支持的格式：
 * - 裸整数简写: 5 -> ConstantInt(5)
 * - 常量: { "type": "minecraft:constant", "value": 5 }
 * - 均匀分布: { "type": "minecraft:uniform", "value": { "min_inclusive": 0, "max_inclusive": 10 } }
 * - 偏向底部: { "type": "minecraft:biased_to_bottom", "value": { "min_inclusive": 0, "max_inclusive": 10 } }
 * - 钳位: { "type": "minecraft:clamped", "value": { "source": {...}, "min_inclusive": 0, "max_inclusive": 10 } }
 * - 正态钳位: { "type": "minecraft:clamped_normal", "value": { "mean": 5.0, "deviation": 2.0, "min_inclusive": 0,
 *              "max_inclusive": 10 } }
 * - 加权列表: { "type": "minecraft:weighted_list", "value": { "distribution": [{"data": 5, "weight": 3}, ...] } }
 *
 * 命名空间前缀 "minecraft:" 可选。
 */
namespace IntProviderParser {

/**
 * @brief 从 JSON 解析 IntProvider
 *
 * @param json JSON 值，可以是裸整数或类型分派对象
 * @param minInclusive 最小值校验（nullopt 表示不校验；支持负值下界如 random_offset 的 -16）
 * @param maxInclusive 最大值校验（nullopt 表示不校验）
 * @return 解析后的 IntProvider，或错误
 */
[[nodiscard]] Result<std::unique_ptr<IntProvider>> parse(const nlohmann::json& json,
    std::optional<i32> minInclusive = std::nullopt,
    std::optional<i32> maxInclusive = std::nullopt);

} // namespace IntProviderParser

} // namespace valueprovider
} // namespace gen
} // namespace world
} // namespace mc
