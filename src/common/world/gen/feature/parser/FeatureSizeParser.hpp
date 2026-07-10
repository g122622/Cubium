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
#include "common/world/gen/feature/tree/featuresize/FeatureSize.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {

/**
 * @brief FeatureSize JSON 解析器
 *
 * 解析树木配置 minimum_size 字段，按 JSON "type" 分派到对应子类。
 *
 * 对应 MC 1.21.11 net.minecraft.world.level.levelgen.feature.featuresize.FeatureSizeType
 * 注册表：
 *   - "minecraft:two_layers_feature_size" → TwoLayersFeatureSize
 *   - "minecraft:three_layers_feature_size" → ThreeLayersFeatureSize
 *
 * 字段映射：
 *   two_layers_feature_size:
 *     {"type":"minecraft:two_layers_feature_size",
 *      "limit":1,"lower_size":0,"upper_size":2,
 *      "min_clipped_height":可选}
 *
 *   three_layers_feature_size:
 *     {"type":"minecraft:three_layers_feature_size",
 *      "limit":1,"lower_size":0,"middle_size":1,
 *      "upper_limit":1,"upper_size":2,
 *      "min_clipped_height":可选}
 *
 * min_clipped_height 为可选整数，缺失时返回 std::nullopt（表示不允许裁剪）。
 */
namespace FeatureSizeParser {

[[nodiscard]] Result<std::unique_ptr<FeatureSize>> parse(const nlohmann::json& sizeObj);

} // namespace FeatureSizeParser

} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
