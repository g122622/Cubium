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

#include "../../../block/blocks/coral/CoralBlock.hpp"
#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include <memory>

namespace mc {

/**
 * @brief 珊瑚特征配置
 */
struct CoralFeatureConfig : public IFeatureConfig {
    /// 珊瑚颜色
    blocks::CoralColor color;

    /// 是否生成墙上的珊瑚扇
    bool includeWallFan = true;

    /// 是否生成失活珊瑚
    bool isDead = false;

    CoralFeatureConfig() = default;

    explicit CoralFeatureConfig(blocks::CoralColor coralColor, bool wallFan = true, bool dead = false)
        : color(coralColor)
        , includeWallFan(wallFan)
        , isDead(dead)
    {}
};

// 珊瑚辅助函数（公共，供 CoralTree/Mushroom/Claw 使用）
[[nodiscard]] const BlockState* getCoralBlockState(blocks::CoralColor color, bool isDead);
[[nodiscard]] const BlockState* getCoralFanState(blocks::CoralColor color, bool isDead);
[[nodiscard]] const BlockState* getCoralWallFanState(blocks::CoralColor color, Direction supportDirection, bool isDead);
[[nodiscard]] bool isWaterAt(WorldGenRegion& world, const BlockPos& pos);
[[nodiscard]] i32 findOceanFloorY(WorldGenRegion& world, i32 x, i32 z);
[[nodiscard]] bool placeCoralBase(WorldGenRegion& world, const BlockPos& pos, blocks::CoralColor color, bool isDead);
void placeCoralDecorations(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    bool isDead,
    bool includeDecorations);
[[nodiscard]] bool placeCoralWithDecorations(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    bool isDead,
    bool includeDecorations);

/**
 * @brief 珊瑚特征基类
 *
 * 在水下生成珊瑚结构。
 * 有多种变体：树形、蘑菇形、爪形。
 */
class CoralFeature {
public:
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config);

protected:
    [[nodiscard]] bool _canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const;
    [[nodiscard]] bool _isWater(WorldGenRegion& world, const BlockPos& pos) const;
    void _placeCoralBlock(WorldGenRegion& world, const BlockPos& pos, blocks::CoralColor color) const;
    void _placeCoralFan(
        WorldGenRegion& world, const BlockPos& pos, blocks::CoralColor color, Direction direction) const;
};

/**
 * @brief 配置化珊瑚特征
 */
class ConfiguredCoralFeature : public ConfiguredFeatureBase {
public:
    ConfiguredCoralFeature(std::unique_ptr<CoralFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const CoralFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<CoralFeatureConfig> m_config;
    std::string m_name;
    mutable CoralFeature m_feature;
};

} // namespace mc
