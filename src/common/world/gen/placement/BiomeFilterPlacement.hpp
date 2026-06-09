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

#include "../../../core/Types.hpp"
#include "Placement.hpp"
#include <memory>

namespace mc {

/**
 * @brief 生物群系过滤放置配置
 *
 * 检查当前位置的生物群系是否包含指定的特征 ID。
 * 与 BiomePlacement（白名单模式）不同，BiomeFilterPlacement
 * 通过反向查询生物群系的生成设置来判断是否允许放置。
 */
struct BiomeFilterConfig : public IPlacementConfig {
    /// 当前配置化特征的 ID
    u32 featureId;

    explicit BiomeFilterConfig(u32 id)
        : featureId(id)
    {}
};

/**
 * @brief 生物群系过滤放置器
 *
 * 检查当前位置的生物群系生成设置中是否包含当前特征。
 * 如果该生物群系没有注册此特征，则不放置。
 */
class BiomeFilterPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "biome_filter"; }
};

} // namespace mc
