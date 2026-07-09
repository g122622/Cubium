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
 * LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

/**
 * @file ChorusPlantFeature.hpp
 * @brief 紫颂树特征
 *
 * 在末地高地生物群系中生成紫颂树。
 *
 * 紫颂树的生成逻辑委托给 ChorusFlowerBlock::generatePlant()，
 * 该方法会递归地放置紫颂植物茎干和紫颂花。
 */

#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include <memory>
#include <vector>

namespace mc {

class WorldGenRegion;
class IChunkGenerator;

/**
 * @brief 紫颂树特征
 *
 * 在末地石上方的空气中放置紫颂树。
 * 特征检查起始位置是否为空气且下方为末地石，
 * 然后调用 ChorusFlowerBlock::generatePlant() 递归生成紫颂树。
 */
class ChorusPlantFeature {
public:
    /**
     * @brief 放置紫颂树
     *
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @return true 如果成功放置
     */
    static bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos);
};

/**
 * @brief 配置化紫颂树特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有并在 place() 前走完，
 * 本类只负责在已确定的 pos 处放置紫颂树。
 */
class ConfiguredChorusPlantFeature : public ConfiguredFeatureBase {
public:
    explicit ConfiguredChorusPlantFeature(const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::string m_name;
};

} // namespace mc
