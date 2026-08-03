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

#include "IcebergFeature.hpp"

#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace mc::world::gen::feature::cave {

namespace {

/// MC Blocks.SNOW_BLOCK / AIR / WATER default state 访问器。
const BlockState* snowBlockState()
{
    return &VanillaBlocks::SNOW_BLOCK->defaultState();
}
const BlockState* airState()
{
    return &VanillaBlocks::AIR->defaultState();
}
const BlockState* waterState()
{
    return &VanillaBlocks::WATER->defaultState();
}

} // namespace

// ============================================================================
// ConfiguredIcebergFeature
// ============================================================================

ConfiguredIcebergFeature::ConfiguredIcebergFeature(std::unique_ptr<IcebergConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredIcebergFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    if (m_config == nullptr || m_config->state == nullptr) {
        return false;
    }
    return m_feature.place(region, generator, random, pos, *m_config);
}

// ============================================================================
// IcebergFeature::place
// ============================================================================

bool IcebergFeature::place(IWorld& world,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& origin,
    const IcebergConfig& config)
{
    // origin 强制对齐到海平面高度（MC: new BlockPos(x, seaLevel, z)）。
    BlockPos blockpos(origin.x, generator.seaLevel(), origin.z);
    const bool flag = random.nextDouble() > 0.7; // 偶尔的开凿/雪块开关
    const BlockState* blockstate = config.state;
    const double d0 = random.nextDouble() * 2.0 * math::PI_DOUBLE;
    const i32 i = 11 - random.nextInt(5);
    const i32 j = 3 + random.nextInt(3);
    const bool flag1 = random.nextDouble() > 0.7;
    const i32 k = 11;
    i32 l = flag1 ? random.nextInt(6) + 6 : random.nextInt(15) + 3;
    if (!flag1 && random.nextDouble() > 0.9) {
        l += random.nextInt(19) + 7;
    }

    const i32 i1 = std::min(l + random.nextInt(11), 18);
    const i32 j1 = std::min(l + random.nextInt(7) - random.nextInt(5), 11);
    const i32 k1 = flag1 ? i : 11;

    // 上半部分（海平面以上）：逐层生成主体。
    for (i32 l1 = -k1; l1 < k1; ++l1) {
        for (i32 i2 = -k1; i2 < k1; ++i2) {
            for (i32 j2 = 0; j2 < l; ++j2) {
                const i32 k2 =
                    flag1 ? heightDependentRadiusEllipse(j2, l, j1) : heightDependentRadiusRound(random, j2, l, j1);
                if (flag1 || l1 < k2) {
                    generateIcebergBlock(
                        world, random, blockpos, l, l1, j2, i2, k2, k1, flag1, j, d0, flag, blockstate);
                }
            }
        }
    }

    smooth(world, blockpos, j1, l, flag1, i);

    // 下半部分（海平面以下）：更陡的半径向下延伸。
    for (i32 i3 = -k1; i3 < k1; ++i3) {
        for (i32 j3 = -k1; j3 < k1; ++j3) {
            for (i32 k3 = -1; k3 > -i1; --k3) {
                const i32 l3 = flag1
                    ? math::ceilTo<i32>(static_cast<f32>(k1) * (1.0F - static_cast<f32>(k3 * k3) / (i1 * 8.0F)))
                    : k1;
                const i32 l2 = heightDependentRadiusSteep(random, -k3, i1, j1);
                if (i3 < l2) {
                    generateIcebergBlock(
                        world, random, blockpos, i1, i3, k3, j3, l2, l3, flag1, j, d0, flag, blockstate);
                }
            }
        }
    }

    const bool flag2 = flag1 ? random.nextDouble() > 0.1 : random.nextDouble() > 0.7;
    if (flag2) {
        generateCutOut(random, world, j1, l, blockpos, flag1, i, d0, j);
    }

    return true;
}

// ============================================================================
// generateCutOut — 在水线附近开凿椭圆形融洞
// ============================================================================

void IcebergFeature::generateCutOut(
    math::Random& random, IWorld& world, i32 j1, i32 l, const BlockPos& blockpos, bool flag1, i32 i, double d0, i32 j)
{
    i32 signX = random.nextBoolean() ? -1 : 1;
    i32 signZ = random.nextBoolean() ? -1 : 1;
    i32 k = random.nextInt(std::max(j1 / 2 - 2, 1));
    if (random.nextBoolean()) {
        k = j1 / 2 + 1 - random.nextInt(std::max(j1 - j1 / 2 - 1, 1));
    }

    i32 lvar = random.nextInt(std::max(j1 / 2 - 2, 1));
    if (random.nextBoolean()) {
        lvar = j1 / 2 + 1 - random.nextInt(std::max(j1 - j1 / 2 - 1, 1));
    }

    if (flag1) {
        k = lvar = random.nextInt(std::max(i - 5, 1));
    }

    const BlockPos offset(signX * k, 0, signZ * lvar);
    const double angle = flag1 ? d0 + (math::PI_DOUBLE / 2.0) : random.nextDouble() * 2.0 * math::PI_DOUBLE;

    for (i32 i1 = 0; i1 < l - 3; ++i1) {
        const i32 radius = heightDependentRadiusRound(random, i1, l, j1);
        carve(radius, i1, blockpos, world, false, angle, offset, i, j);
    }

    for (i32 k1 = -1; k1 > -l + random.nextInt(5); --k1) {
        const i32 radius = heightDependentRadiusSteep(random, -k1, l, j1);
        carve(radius, k1, blockpos, world, true, angle, offset, i, j);
    }
}

// ============================================================================
// carve — 椭圆范围内清成 AIR（上半）或 WATER（下半）
// ============================================================================

void IcebergFeature::carve(i32 radius,
    i32 y,
    const BlockPos& blockpos,
    IWorld& world,
    bool water,
    double angle,
    const BlockPos& offset,
    i32 i,
    i32 j)
{
    const i32 a = radius + 1 + i / 3;
    const i32 b = std::min(radius - 3, 3) + j / 2 - 1;

    for (i32 k = -a; k < a; ++k) {
        for (i32 lvar = -a; lvar < a; ++lvar) {
            const double d0 = signedDistanceEllipse(k, lvar, offset, a, b, angle);
            if (d0 < 0.0) {
                const BlockPos pos(blockpos.x + k, blockpos.y + y, blockpos.z + lvar);
                const BlockState* state = world.getBlockState(pos);
                if (state == nullptr) {
                    continue;
                }
                if (isIcebergState(*state) || state->is(VanillaBlocks::SNOW_BLOCK)) {
                    if (water) {
                        world.setBlockState(pos, waterState());
                    } else {
                        world.setBlockState(pos, airState());
                        removeFloatingSnowLayer(world, pos);
                    }
                }
            }
        }
    }
}

void IcebergFeature::removeFloatingSnowLayer(IWorld& world, const BlockPos& pos)
{
    const BlockState* above = world.getBlockState(pos.up());
    if (above != nullptr && above->is(VanillaBlocks::SNOW)) {
        world.setBlockState(pos.up(), airState());
    }
}

// ============================================================================
// generateIcebergBlock / setIcebergBlock — 主体方块放置
// ============================================================================

void IcebergFeature::generateIcebergBlock(IWorld& world,
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
    const BlockState* blockstate)
{
    const double d0dist = flag1 ? signedDistanceEllipse(l1, i2, BlockPos(0, 0, 0), k1, getEllipseC(j2, l, j), d0)
                                : signedDistanceCircle(l1, i2, BlockPos(0, 0, 0), k2, random);
    if (d0dist < 0.0) {
        const BlockPos pos(blockpos.x + l1, blockpos.y + j2, blockpos.z + i2);
        const double d1 = flag1 ? -0.5 : -6 - random.nextInt(3);
        if (d0dist > d1 && random.nextDouble() > 0.9) {
            return;
        }

        setIcebergBlock(pos, world, random, l - j2, l, flag1, flag, blockstate);
    }
}

void IcebergFeature::setIcebergBlock(const BlockPos& pos,
    IWorld& world,
    math::Random& random,
    i32 depthFromTop,
    i32 totalHeight,
    bool flag1,
    bool flag,
    const BlockState* blockstate)
{
    const BlockState* existing = world.getBlockState(pos);
    const bool isAir = (existing == nullptr) || existing->isAir();
    if (!isAir &&
        !(existing != nullptr &&
            (existing->is(VanillaBlocks::SNOW_BLOCK) || existing->is(VanillaBlocks::ICE) ||
                existing->is(VanillaBlocks::WATER)))) {
        return;
    }

    const bool flag2 = !flag1 || random.nextDouble() > 0.05;
    const i32 divisor = flag1 ? 3 : 2;
    if (flag && existing != nullptr && !existing->is(VanillaBlocks::WATER) &&
        depthFromTop <= random.nextInt(std::max(1, totalHeight / divisor)) + totalHeight * 0.6 && flag2) {
        world.setBlockState(pos, snowBlockState());
    } else {
        world.setBlockState(pos, blockstate);
    }
}

// ============================================================================
// 椭圆/圆形有符号距离与高度依赖半径
// ============================================================================

int IcebergFeature::getEllipseC(int p_66019_, int p_66020_, int p_66021_) const
{
    int i = p_66021_;
    if (p_66019_ > 0 && p_66020_ - p_66019_ <= 3) {
        i = p_66021_ - (4 - (p_66020_ - p_66019_));
    }
    return i;
}

double IcebergFeature::signedDistanceCircle(
    int x, int z, const BlockPos& center, int radius, math::Random& random) const
{
    const f32 f = 10.0F * math::clamp(random.nextFloat(), 0.2F, 0.8F) / static_cast<f32>(radius);
    return static_cast<double>(f) + std::pow(static_cast<double>(x - center.x), 2.0) +
        std::pow(static_cast<double>(z - center.z), 2.0) - std::pow(static_cast<double>(radius), 2.0);
}

double IcebergFeature::signedDistanceEllipse(int x, int z, const BlockPos& center, int a, int b, double angle) const
{
    const double dx = static_cast<double>(x - center.x);
    const double dz = static_cast<double>(z - center.z);
    const double cosA = std::cos(angle);
    const double sinA = std::sin(angle);
    return std::pow((dx * cosA - dz * sinA) / static_cast<double>(a), 2.0) +
        std::pow((dx * sinA + dz * cosA) / static_cast<double>(b), 2.0) - 1.0;
}

int IcebergFeature::heightDependentRadiusRound(math::Random& random, int y, int height, int radius) const
{
    const f32 f = 3.5F - random.nextFloat();
    f32 f1 = (1.0F - static_cast<f32>(y * y) / (static_cast<f32>(height) * f)) * static_cast<f32>(radius);
    if (height > 15 + random.nextInt(5)) {
        const i32 yClamped = y < 3 + random.nextInt(6) ? y / 2 : y;
        f1 = (1.0F - static_cast<f32>(yClamped) / (static_cast<f32>(height) * f * 0.4F)) * static_cast<f32>(radius);
    }
    return math::ceilTo<i32>(f1 / 2.0F);
}

int IcebergFeature::heightDependentRadiusEllipse(int y, int height, int radius) const
{
    const f32 f1 = (1.0F - static_cast<f32>(y * y) / static_cast<f32>(height)) * static_cast<f32>(radius);
    return math::ceilTo<i32>(f1 / 2.0F);
}

int IcebergFeature::heightDependentRadiusSteep(math::Random& random, int y, int height, int radius) const
{
    const f32 f = 1.0F + random.nextFloat() / 2.0F;
    const f32 f1 = (1.0F - static_cast<f32>(y) / (static_cast<f32>(height) * f)) * static_cast<f32>(radius);
    return math::ceilTo<i32>(f1 / 2.0F);
}

bool IcebergFeature::isIcebergState(const BlockState& state)
{
    return state.is(VanillaBlocks::PACKED_ICE) || state.is(VanillaBlocks::SNOW_BLOCK) ||
        state.is(VanillaBlocks::BLUE_ICE);
}

bool IcebergFeature::belowIsAir(IWorld& world, const BlockPos& pos) const
{
    const BlockState* below = world.getBlockState(pos.down());
    return below == nullptr || below->isAir();
}

// ============================================================================
// smooth — 清除悬空与孤立冰山方块
// ============================================================================

void IcebergFeature::smooth(IWorld& world, const BlockPos& blockpos, i32 j1, i32 l, bool flag1, i32 i)
{
    const i32 range = flag1 ? i : j1 / 2;
    for (i32 dx = -range; dx <= range; ++dx) {
        for (i32 dz = -range; dz <= range; ++dz) {
            for (i32 y = 0; y <= l; ++y) {
                const BlockPos pos(blockpos.x + dx, blockpos.y + y, blockpos.z + dz);
                const BlockState* state = world.getBlockState(pos);
                if (state == nullptr) {
                    continue;
                }
                const bool isIceberg = isIcebergState(*state);
                if (isIceberg || state->is(VanillaBlocks::SNOW)) {
                    if (belowIsAir(world, pos)) {
                        world.setBlockState(pos, airState());
                        world.setBlockState(pos.up(), airState());
                    } else if (isIceberg) {
                        const BlockState* west = world.getBlockState(pos.west());
                        const BlockState* east = world.getBlockState(pos.east());
                        const BlockState* north = world.getBlockState(pos.north());
                        const BlockState* south = world.getBlockState(pos.south());
                        i32 nonIceberg = 0;
                        if (west == nullptr || !isIcebergState(*west)) ++nonIceberg;
                        if (east == nullptr || !isIcebergState(*east)) ++nonIceberg;
                        if (north == nullptr || !isIcebergState(*north)) ++nonIceberg;
                        if (south == nullptr || !isIcebergState(*south)) ++nonIceberg;
                        if (nonIceberg >= 3) {
                            world.setBlockState(pos, airState());
                        }
                    }
                }
            }
        }
    }
}

} // namespace mc::world::gen::feature::cave
