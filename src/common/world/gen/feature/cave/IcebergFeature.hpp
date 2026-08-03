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

#include "../BlockBlobFeature.hpp"
#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"

#include <memory>
#include <string>

namespace mc::world::gen::feature::cave {

/**
 * @brief 冰山特征配置（MC BlockStateConfiguration）
 *
 * 仅含一个目标方块状态（packed_ice 或 blue_ice），决定冰山主体填充方块。
 * 复用通用 BlockStateConfig（定义于 BlockBlobFeature.hpp）。
 */
using IcebergConfig = ::mc::world::gen::feature::BlockStateConfig;

/**
 * @brief 冰山特征（MC IcebergFeature）
 *
 * 在 origin 的海平面高度生成冰山。算法分四步：
 * 1. 上半部分（海平面以上）：用椭圆/圆形高度依赖半径逐层生成主体方块，
 *    偶尔替换为雪块（SNOW_BLOCK）。
 * 2. smooth：清除下方悬空、周围 ≥3 邻居非冰山的孤立冰山方块。
 * 3. 下半部分（海平面以下）：用更陡的高度依赖半径向下延伸冰山。
 * 4. generateCutOut：在水线附近开凿一个椭圆形空洞（模拟冰山内部融洞），
 *    上半空洞清成 AIR，下半空洞清成 WATER。
 *
 * 装饰阶段为 SurfaceStructures。
 */
class IcebergFeature {
public:
    bool place(IWorld& world,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const IcebergConfig& config);

private:
    void generateCutOut(math::Random& random,
        IWorld& world,
        i32 j1,
        i32 l,
        const BlockPos& blockpos,
        bool flag1,
        i32 i,
        double d0,
        i32 j);
    void carve(i32 radius,
        i32 y,
        const BlockPos& blockpos,
        IWorld& world,
        bool water,
        double angle,
        const BlockPos& offset,
        i32 i,
        i32 j);
    void removeFloatingSnowLayer(IWorld& world, const BlockPos& pos);
    void generateIcebergBlock(IWorld& world,
        math::Random& random,
        const BlockPos& blockpos,
        i32 l,
        i32 l1,
        i32 j2,
        i32 i2,
        i32 k2,
        i32 k1,
        bool flag1,
        i32 j,
        double d0,
        bool flag,
        const BlockState* blockstate);
    void setIcebergBlock(const BlockPos& pos,
        IWorld& world,
        math::Random& random,
        i32 p_225128_,
        i32 p_225129_,
        bool flag1,
        bool flag,
        const BlockState* blockstate);
    [[nodiscard]] int getEllipseC(int p_66019_, int p_66020_, int p_66021_) const;
    [[nodiscard]] double signedDistanceCircle(
        int x, int z, const BlockPos& center, int radius, math::Random& random) const;
    [[nodiscard]] double signedDistanceEllipse(int x, int z, const BlockPos& center, int a, int b, double angle) const;
    [[nodiscard]] int heightDependentRadiusRound(math::Random& random, int y, int height, int radius) const;
    [[nodiscard]] int heightDependentRadiusEllipse(int y, int height, int radius) const;
    [[nodiscard]] int heightDependentRadiusSteep(math::Random& random, int y, int height, int radius) const;
    void smooth(IWorld& world, const BlockPos& blockpos, int j1, int l, bool flag1, int i);
    [[nodiscard]] static bool isIcebergState(const BlockState& state);
    [[nodiscard]] bool belowIsAir(IWorld& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化冰山特征
 */
class ConfiguredIcebergFeature : public ConfiguredFeatureBase {
public:
    ConfiguredIcebergFeature(std::unique_ptr<IcebergConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::SurfaceStructures; }
    [[nodiscard]] const IcebergConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<IcebergConfig> m_config;
    std::string m_name;
    mutable IcebergFeature m_feature;
};

} // namespace mc::world::gen::feature::cave
