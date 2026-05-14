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

#include "FoliagePlacers.hpp"
#include "../../../../block/BlockRegistry.hpp"
#include "../../../../block/VanillaBlocks.hpp"
#include "../../../chunk/IChunkGenerator.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// PineFoliagePlacer 实现
// ============================================================================

PineFoliagePlacer::PineFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{}

i32 PineFoliagePlacer::getFoliageHeight(math::Random& /*random*/, i32 /*trunkHeight*/) const
{
    return m_height;
}

void PineFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 从下到上逐层放置树叶，半径逐渐减小
    for (i32 y = 0; y <= foliageHeight; ++y) {
        i32 layerRadius = getRadiusAtHeight(y, foliageHeight);

        for (i32 dx = -layerRadius; dx <= layerRadius; ++dx) {
            for (i32 dz = -layerRadius; dz <= layerRadius; ++dz) {
                if (shouldSkip(random, dx, y, dz, layerRadius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool PineFoliagePlacer::shouldSkip(
    math::Random& random, i32 dx, i32 /*dy*/, i32 dz, i32 radius, bool /*trunkTop*/) const
{
    // 跳过角落
    i32 dist = std::abs(dx) + std::abs(dz);
    if (dist > radius) {
        return true;
    }

    // 随机跳过边缘
    if (dist == radius && random.nextFloat() < 0.2f) {
        return true;
    }

    return false;
}

i32 PineFoliagePlacer::getRadiusAtHeight(i32 height, i32 foliageHeight) const
{
    // 锥形：底部大，顶部小
    if (foliageHeight <= 0) {
        return 1;
    }
    return std::max(0, m_height / 2 - height / 2);
}

std::unique_ptr<FoliagePlacer> PineFoliagePlacer::clone() const
{
    return std::make_unique<PineFoliagePlacer>(m_radius, m_offset, m_height);
}

// ============================================================================
// SpruceFoliagePlacer 实现 - 参考 MC SpruceFoliagePlacer.java
// ============================================================================

SpruceFoliagePlacer::SpruceFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{}

i32 SpruceFoliagePlacer::getFoliageHeight(math::Random& random, i32 trunkHeight) const
{
    // MC 第49-51行: return Math.max(4, trunkHeight - trunkHeightSpread)
    return std::max(4, trunkHeight - m_height);
}

void SpruceFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 offset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 参考 MC SpruceFoliagePlacer.func_230372_a_ (第30-46行)
    // 云杉树叶从顶部向下层叠，半径从顶部开始逐渐增大

    i32 currentRadius = random.nextInt(2); // MC 第32行: int i = random.nextInt(2);
    i32 minRadius = 1;                     // MC 第33行: int j = 1;
    i32 nextRadius = 0;                    // MC 第34行: int k = 0;

    // MC 第36-45行: 从offset向下遍历到-foliageHeight
    for (i32 y = offset; y >= -foliageHeight; --y) {
        // 放置当前层的树叶
        placeFoliageLayer(
            world, random, foliagePos, currentRadius, y, foliageBlocks, foliageBlock, foliagePos.trunkTop);

        // MC 第38-44行: 半径递增逻辑
        if (currentRadius >= minRadius) {
            currentRadius = nextRadius;
            nextRadius = 1;
            minRadius = std::min(minRadius + 1, radius + foliagePos.radiusBonus);
        } else {
            ++currentRadius;
        }
    }
}

void SpruceFoliagePlacer::placeFoliageLayer(WorldGenRegion& world,
    math::Random& random,
    const FoliagePosition& foliagePos,
    i32 radius,
    i32 yOffset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock,
    bool trunkTop)
{
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            if (shouldSkip(random, dx, yOffset, dz, radius, trunkTop)) {
                continue;
            }

            BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + yOffset, foliagePos.pos.z + dz);
            foliageBlocks.insert(pos);
        }
    }
}

bool SpruceFoliagePlacer::shouldSkip(
    math::Random& /*random*/, i32 dx, i32 /*dy*/, i32 dz, i32 radius, bool /*trunkTop*/) const
{
    // MC 第53-55行: 只跳过角落且半径>0的情况
    return dx == radius && dz == radius && radius > 0;
}

std::unique_ptr<FoliagePlacer> SpruceFoliagePlacer::clone() const
{
    return std::make_unique<SpruceFoliagePlacer>(m_radius, m_offset, m_height);
}

// ============================================================================
// AcaciaFoliagePlacer 实现 - 参考 MC AcaciaFoliagePlacer.java
// ============================================================================

AcaciaFoliagePlacer::AcaciaFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset)
    : FoliagePlacer(radius, offset)
{}

i32 AcaciaFoliagePlacer::getFoliageHeight(math::Random& /*random*/, i32 /*trunkHeight*/) const
{
    // MC 第34-36行: return 0
    return 0;
}

void AcaciaFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 offset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 参考 MC AcaciaFoliagePlacer.func_230372_a_ (第26-32行)
    // 金合欢有3层树叶，不同的半径和Y偏移

    bool trunkTop = foliagePos.trunkTop;

    // MC 第29行: 第一层，半径=radius+radiusBonus，Y偏移=-1-foliageHeight
    placeFoliageLayer(world,
        random,
        foliagePos,
        radius + foliagePos.radiusBonus,
        -1 - foliageHeight,
        foliageBlocks,
        foliageBlock,
        trunkTop);

    // MC 第30行: 第二层，半径=radius-1，Y偏移=-foliageHeight
    placeFoliageLayer(world, random, foliagePos, radius - 1, -foliageHeight, foliageBlocks, foliageBlock, trunkTop);

    // MC 第31行: 第三层，半径=radius+radiusBonus-1，Y偏移=0
    placeFoliageLayer(
        world, random, foliagePos, radius + foliagePos.radiusBonus - 1, 0, foliageBlocks, foliageBlock, trunkTop);
}

void AcaciaFoliagePlacer::placeFoliageLayer(WorldGenRegion& world,
    math::Random& random,
    const FoliagePosition& foliagePos,
    i32 radius,
    i32 yOffset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock,
    bool trunkTop)
{
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            if (shouldSkip(random, dx, yOffset, dz, radius, trunkTop)) {
                continue;
            }

            BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + yOffset, foliagePos.pos.z + dz);
            foliageBlocks.insert(pos);
        }
    }
}

bool AcaciaFoliagePlacer::shouldSkip(math::Random& /*random*/, i32 dx, i32 dy, i32 dz, i32 radius, bool trunkTop) const
{
    // MC 第38-44行
    if (dy == 0) {
        // 第一层（dy=0时，实际是y=-1-foliageHeight层）
        // 跳过角落，但保留边缘
        return (dx > 1 || dz > 1) && dx != 0 && dz != 0;
    } else {
        // 其他层：跳过角落
        return dx == radius && dz == radius && radius > 0;
    }
}

std::unique_ptr<FoliagePlacer> AcaciaFoliagePlacer::clone() const
{
    return std::make_unique<AcaciaFoliagePlacer>(m_radius, m_offset);
}

// ============================================================================
// DarkOakFoliagePlacer 实现 - 参考 MC DarkOakFoliagePlacer.java
// ============================================================================

DarkOakFoliagePlacer::DarkOakFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{}

i32 DarkOakFoliagePlacer::getFoliageHeight(math::Random& /*random*/, i32 /*trunkHeight*/) const
{
    // MC 第43-45行: return 4
    return 4;
}

void DarkOakFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 offset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 参考 MC DarkOakFoliagePlacer.func_230372_a_ (第26-40行)
    // 深色橡树有多层不同半径的树叶

    bool trunkTop = foliagePos.trunkTop;

    if (trunkTop) {
        // MC 第29-35行: trunkTop为true时的层叠
        // 第一层，半径=radius+2，Y偏移=-1
        placeFoliageLayer(world, random, foliagePos, radius + 2, -1, foliageBlocks, foliageBlock, trunkTop);
        // 第二层，半径=radius+3，Y偏移=0
        placeFoliageLayer(world, random, foliagePos, radius + 3, 0, foliageBlocks, foliageBlock, trunkTop);
        // 第三层，半径=radius+2，Y偏移=1
        placeFoliageLayer(world, random, foliagePos, radius + 2, 1, foliageBlocks, foliageBlock, trunkTop);
        // 可选第四层，半径=radius，Y偏移=2
        if (random.nextBoolean()) {
            placeFoliageLayer(world, random, foliagePos, radius, 2, foliageBlocks, foliageBlock, trunkTop);
        }
    } else {
        // MC 第36-39行: trunkTop为false时的层叠
        // 第一层，半径=radius+2，Y偏移=-1
        placeFoliageLayer(world, random, foliagePos, radius + 2, -1, foliageBlocks, foliageBlock, trunkTop);
        // 第二层，半径=radius+1，Y偏移=0
        placeFoliageLayer(world, random, foliagePos, radius + 1, 0, foliageBlocks, foliageBlock, trunkTop);
    }
}

void DarkOakFoliagePlacer::placeFoliageLayer(WorldGenRegion& world,
    math::Random& random,
    const FoliagePosition& foliagePos,
    i32 radius,
    i32 yOffset,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock,
    bool trunkTop)
{
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            if (shouldSkip(random, dx, yOffset, dz, radius, trunkTop)) {
                continue;
            }

            BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + yOffset, foliagePos.pos.z + dz);
            foliageBlocks.insert(pos);
        }
    }
}

bool DarkOakFoliagePlacer::shouldSkip(math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool trunkTop) const
{
    // MC 第51-59行
    if (dy == -1 && !trunkTop) {
        // 第一层且非trunkTop：跳过角落
        return dx == radius && dz == radius;
    } else if (dy == 1) {
        // 第三层：跳过角落附近
        return dx + dz > radius * 2 - 2;
    } else {
        // 其他情况调用基类
        return FoliagePlacer::shouldSkip(random, dx, dy, dz, radius, trunkTop);
    }
}

bool DarkOakFoliagePlacer::shouldSkipBase(math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool trunkTop) const
{
    // MC 第47-49行: 基类方法
    return FoliagePlacer::shouldSkip(random, dx, dy, dz, radius, trunkTop);
}

std::unique_ptr<FoliagePlacer> DarkOakFoliagePlacer::clone() const
{
    return std::make_unique<DarkOakFoliagePlacer>(m_radius, m_offset, m_height);
}

// ============================================================================
// JungleFoliagePlacer 实现
// ============================================================================

JungleFoliagePlacer::JungleFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{}

i32 JungleFoliagePlacer::getFoliageHeight(math::Random& random, i32 /*trunkHeight*/) const
{
    return m_height + random.nextInt(2);
}

void JungleFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 丛林木：较稀疏的单层树叶
    for (i32 y = 0; y < foliageHeight; ++y) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            for (i32 dz = -radius; dz <= radius; ++dz) {
                if (shouldSkip(random, dx, y, dz, radius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool JungleFoliagePlacer::shouldSkip(math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool /*trunkTop*/) const
{
    i32 dist = std::abs(dx) + std::abs(dz);
    if (dist > radius) {
        return true;
    }

    // 丛林木树叶较稀疏
    if (dist == radius && random.nextFloat() < 0.4f) {
        return true;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> JungleFoliagePlacer::clone() const
{
    return std::make_unique<JungleFoliagePlacer>(m_radius, m_offset, m_height);
}

// ============================================================================
// MegaPineFoliagePlacer 实现
// ============================================================================

MegaPineFoliagePlacer::MegaPineFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{}

i32 MegaPineFoliagePlacer::getFoliageHeight(math::Random& random, i32 /*trunkHeight*/) const
{
    return m_height + random.nextInt(4);
}

void MegaPineFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 巨型松树：更大的锥形
    for (i32 y = 0; y <= foliageHeight; ++y) {
        // 从底部到顶部半径逐渐减小
        i32 layerRadius = std::max(1, radius - y / 3);

        for (i32 dx = -layerRadius; dx <= layerRadius; ++dx) {
            for (i32 dz = -layerRadius; dz <= layerRadius; ++dz) {
                if (shouldSkip(random, dx, y, dz, layerRadius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool MegaPineFoliagePlacer::shouldSkip(
    math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool /*trunkTop*/) const
{
    i32 dist = std::abs(dx) + std::abs(dz);
    if (dist > radius) {
        return true;
    }

    // 边缘稀疏
    if (dist >= radius - 1 && random.nextFloat() < 0.3f) {
        return true;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> MegaPineFoliagePlacer::clone() const
{
    return std::make_unique<MegaPineFoliagePlacer>(m_radius, m_offset, m_height);
}

// ============================================================================
// BushFoliagePlacer 实现
// ============================================================================

BushFoliagePlacer::BushFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset)
    : FoliagePlacer(radius, offset)
{}

i32 BushFoliagePlacer::getFoliageHeight(math::Random& /*random*/, i32 /*trunkHeight*/) const
{
    return 1; // 灌木只有一层
}

void BushFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 灌木：单层球形
    for (i32 y = 0; y < foliageHeight; ++y) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            for (i32 dz = -radius; dz <= radius; ++dz) {
                if (shouldSkip(random, dx, y, dz, radius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool BushFoliagePlacer::shouldSkip(math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool /*trunkTop*/) const
{
    // 球形
    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dz * dz));
    if (dist > radius + 0.5f) {
        return true;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> BushFoliagePlacer::clone() const
{
    return std::make_unique<BushFoliagePlacer>(m_radius, m_offset);
}

// ============================================================================
// FancyFoliagePlacer 实现
// ============================================================================

FancyFoliagePlacer::FancyFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{}

i32 FancyFoliagePlacer::getFoliageHeight(math::Random& random, i32 /*trunkHeight*/) const
{
    return m_height + random.nextInt(3);
}

void FancyFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 精美树叶：更大更密集的球形
    for (i32 y = -1; y <= foliageHeight; ++y) {
        // 中间最大，上下略小
        i32 layerRadius = radius;
        if (y < 0 || y >= foliageHeight - 1) {
            layerRadius = std::max(1, radius - 1);
        }

        for (i32 dx = -layerRadius; dx <= layerRadius; ++dx) {
            for (i32 dz = -layerRadius; dz <= layerRadius; ++dz) {
                if (shouldSkip(random, dx, y, dz, layerRadius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool FancyFoliagePlacer::shouldSkip(math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool /*trunkTop*/) const
{
    // 更密集的球形
    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dy * dy * 0.25f + dz * dz));
    if (dist > radius + 1) {
        return true;
    }

    // 边缘随机跳过
    if (dist > radius - 0.5f && random.nextFloat() < 0.2f) {
        return true;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> FancyFoliagePlacer::clone() const
{
    return std::make_unique<FancyFoliagePlacer>(m_radius, m_offset, m_height);
}

} // namespace mc
