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

#include "ConfiguredFeature.hpp"
#include "common/core/Types.hpp"

namespace mc {

/**
 * @brief 雪和冰冻结顶层特征（freeze_top_layer）
 *
 * 在世界生成阶段的 TopLayerModification 阶段执行，
 * 遍历区块的每个 XZ 列，根据生物群系温度放置雪层和冻结水面。
 *
 * 结冰逻辑：对每个 XZ 列的 MOTION_BLOCKING 高度下方一格检查 shouldFreeze，
 * 如果满足条件则将水替换为冰。checkNeighbors=false（生成阶段所有暴露水面都冻结）。
 *
 * 降雪逻辑：对每个 XZ 列的 MOTION_BLOCKING 高度检查 shouldSnow，
 * 如果满足条件则放置雪层，并设置下方方块的 SNOWY 属性。
 */
class ConfiguredSnowAndFreezeFeature : public ConfiguredFeatureBase {
public:
    /**
     * @brief 构造函数
     * @param featureName 特征名称
     */
    explicit ConfiguredSnowAndFreezeFeature(const char* featureName);

    /**
     * @brief 在区块中放置雪和冰
     *
     * 遍历区块内所有 16x16 列，对每个列：
     * 1. 获取 MOTION_BLOCKING 高度图最高方块 Y
     * 2. 对高度下方一格执行结冰检查（shouldFreeze, checkNeighbors=false）
     * 3. 对高度位置执行降雪检查（shouldSnow）
     *
     * @param region 世界生成区域
     * @param chunk 区块数据
     * @param generator 区块生成器
     * @param random 随机数生成器
     * @param pos 区块原点位置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::TopLayerModification; }

private:
    std::string m_name;
};

} // namespace mc
