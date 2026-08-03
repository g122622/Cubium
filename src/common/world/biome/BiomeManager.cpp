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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "BiomeManager.hpp"

#include "common/core/Types.hpp"
#include "common/util/crypto/Sha256.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/LinearCongruentialGenerator.hpp"
#include "common/world/biome/BiomeSource.hpp"
#include <limits>

namespace mc::world::biome {

// ============================================================================
// 构造函数
// ============================================================================

BiomeManager::BiomeManager(const IBiomeSource& source, u64 biomeZoomSeed)
    : m_source(&source)
    , m_biomeZoomSeed(biomeZoomSeed)
{}

// ============================================================================
// 静态方法
// ============================================================================

u64 BiomeManager::obfuscateSeed(u64 worldSeed)
{
    // MC 1.21.11: BiomeManager.obfuscateSeed(worldSeed)
    // = Hashing.sha256().hashLong(worldSeed).asLong()
    // 与 Sha256::hashWorldSeed() 完全一致
    return util::crypto::Sha256::hashWorldSeed(worldSeed);
}

BiomeManager BiomeManager::withDifferentSource(const IBiomeSource& source) const
{
    return BiomeManager(source, m_biomeZoomSeed);
}

// ============================================================================
// 核心查询方法
// ============================================================================

BiomeId BiomeManager::getBiome(i32 blockX, i32 blockY, i32 blockZ) const
{
    // MC 1.21.11: BiomeManager.getBiome(BlockPos)
    // 偏移 -2 以避免边界问题
    const i32 i = blockX - 2;
    const i32 j = blockY - 2;
    const i32 k = blockZ - 2;

    // 转换为 quart 坐标（floor division）
    const i32 l = i >> 2;
    const i32 i1 = j >> 2;
    const i32 j1 = k >> 2;

    // 计算子 quart 偏移 [0.0, 0.75]
    const f64 d0 = static_cast<f64>(i & 3) / 4.0;
    const f64 d1 = static_cast<f64>(j & 3) / 4.0;
    const f64 d2 = static_cast<f64>(k & 3) / 4.0;

    // 遍历 2x2x2 的 8 个 quart 角点
    // 选择 fiddled distance 最小的角点
    f64 minDist = std::numeric_limits<f64>::max();
    i32 bestCorner = 0;

    for (i32 corner = 0; corner < 8; ++corner) {
        // bit 2: X 方向 (0=lower, 1=upper)
        // bit 1: Y 方向 (0=lower, 1=upper)
        // bit 0: Z 方向 (0=lower, 1=upper)
        const i32 flag = (corner >> 2) & 1;  // X
        const i32 flag1 = (corner >> 1) & 1; // Y
        const i32 flag2 = corner & 1;        // Z

        const i32 x = l + flag;
        const i32 y = i1 + flag1;
        const i32 z = j1 + flag2;

        // 子 quart 偏移调整：upper 角点需要减 1（偏移量相对于角点）
        const f64 adjX = d0 - static_cast<f64>(flag);
        const f64 adjY = d1 - static_cast<f64>(flag1);
        const f64 adjZ = d2 - static_cast<f64>(flag2);

        const f64 dist = getFiddledDistance(static_cast<i64>(m_biomeZoomSeed), x, y, z, adjX, adjY, adjZ);

        if (dist < minDist) {
            minDist = dist;
            bestCorner = corner;
        }
    }

    // 从最佳角点恢复 quart 坐标
    const i32 bestX = l + ((bestCorner >> 2) & 1);
    const i32 bestY = i1 + ((bestCorner >> 1) & 1);
    const i32 bestZ = j1 + (bestCorner & 1);

    return m_source->getNoiseBiome(bestX, bestY, bestZ);
}

BiomeId BiomeManager::getNoiseBiomeAtQuart(i32 quartX, i32 quartY, i32 quartZ) const
{
    return m_source->getNoiseBiome(quartX, quartY, quartZ);
}

BiomeId BiomeManager::getNoiseBiomeAtPosition(i32 blockX, i32 blockY, i32 blockZ) const
{
    // MC 1.21.11: BiomeManager.getNoiseBiomeAtPosition(double, double, double)
    // floor to block, then convert to quart
    const i32 quartX = blockX >> 2;
    const i32 quartY = blockY >> 2;
    const i32 quartZ = blockZ >> 2;
    return m_source->getNoiseBiome(quartX, quartY, quartZ);
}

// ============================================================================
// Voronoi 缩放核心
// ============================================================================

f64 BiomeManager::getFiddledDistance(u64 seed, i32 x, i32 y, i32 z, f64 fudgeX, f64 fudgeY, f64 fudgeZ)
{
    // MC 1.21.11: BiomeManager.getFiddledDistance()
    // 链式调用 LinearCongruentialGenerator.next() 6 次，输入坐标各两次
    i64 lcgSeed = static_cast<i64>(seed);
    lcgSeed = math::LinearCongruentialGenerator::next(lcgSeed, x);
    lcgSeed = math::LinearCongruentialGenerator::next(lcgSeed, y);
    lcgSeed = math::LinearCongruentialGenerator::next(lcgSeed, z);
    lcgSeed = math::LinearCongruentialGenerator::next(lcgSeed, x);
    lcgSeed = math::LinearCongruentialGenerator::next(lcgSeed, y);
    lcgSeed = math::LinearCongruentialGenerator::next(lcgSeed, z);

    // 提取 X fiddle
    const f64 fiddleX = getFiddle(lcgSeed);

    // 推进 LCG 并提取 Y fiddle
    lcgSeed = math::LinearCongruentialGenerator::next(lcgSeed, static_cast<i64>(seed));
    const f64 fiddleY = getFiddle(lcgSeed);

    // 推进 LCG 并提取 Z fiddle
    lcgSeed = math::LinearCongruentialGenerator::next(lcgSeed, static_cast<i64>(seed));
    const f64 fiddleZ = getFiddle(lcgSeed);

    // 计算 squared distance（fudge + fiddle）
    const f64 dx = fudgeX + fiddleX;
    const f64 dy = fudgeY + fiddleY;
    const f64 dz = fudgeZ + fiddleZ;

    return dx * dx + dy * dy + dz * dz;
}

f64 BiomeManager::getFiddle(i64 seed)
{
    // MC 1.21.11: BiomeManager.getFiddle()
    // Math.floorMod(seed >> 24, 1024) / 1024.0，映射到 [-0.45, 0.45)
    const i64 shifted = seed >> 24;
    const f64 d = static_cast<f64>(math::floorMod(shifted, static_cast<i64>(1024))) / 1024.0;
    return (d - 0.5) * 0.9;
}

} // namespace mc::world::biome
