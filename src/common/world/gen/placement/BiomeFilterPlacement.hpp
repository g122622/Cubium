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
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {

/**
 * @brief 生物群系过滤放置配置
 *
 * 检查当前位置的生物群系是否包含指定的 placed_feature。
 * 与 BiomePlacement（白名单模式）不同，BiomeFilterPlacement
 * 通过反向查询生物群系的生成设置来判断是否允许放置。
 *
 * 无 config 字段，运行时需要知道"自己属于哪个 placed_feature"——通过
 * PlacedFeatureLoader 在构造完放置链后回填 placedFeatureId 实现。
 */
struct BiomeFilterConfig : public IPlacementConfig {
    /// 当前 placed_feature 的 ResourceLocation（回填）
    ResourceLocation placedFeatureId;

    explicit BiomeFilterConfig(ResourceLocation id)
        : placedFeatureId(std::move(id))
    {}

    /// 默认构造：构造链时占位，之后由 setId 回填
    BiomeFilterConfig() = default;

    /// 回填 placed_feature id（PlacedFeatureLoader 在解析完整条链后调用）
    void setPlacedFeatureId(ResourceLocation id) { placedFeatureId = std::move(id); }
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
