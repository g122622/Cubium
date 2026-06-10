/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Permission notice
 * be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Beardifier.hpp"
#include "common/util/math/MathUtils.hpp"
#include <algorithm>
#include <cmath>

namespace mc::world::gen::density {

// ============================================================================
// BeardifierMarker 静态实例
// ============================================================================

const BeardifierMarker BeardifierMarker::INSTANCE;

// ============================================================================
// Beardifier 静态常量
// ============================================================================

// Empty Beardifier — 无结构影响时使用
static Beardifier createEmpty()
{
    return Beardifier({}, {});
}

const Beardifier Beardifier::EMPTY = createEmpty();

// ============================================================================
// Beardifier 构造
// ============================================================================

Beardifier::Beardifier(std::vector<Rigid> pieces, std::vector<jigsaw::JigsawJunction> junctions)
    : m_pieces(std::move(pieces))
    , m_junctions(std::move(junctions))
{}

// ============================================================================
// Beardifier::compute — MC 1.21 Beardifier.compute
// ============================================================================

f64 Beardifier::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    if (m_pieces.empty() && m_junctions.empty()) {
        return 0.0;
    }

    f64 d0 = 0.0;

    // 处理结构片段
    for (const auto& rigid : m_pieces) {
        const auto& box = rigid.box;

        // 计算到包围盒边界的距离
        const i32 dx = std::max(0, std::max(box.minX() - blockX, blockX - box.maxX()));
        const i32 dz = std::max(0, std::max(box.minZ() - blockZ, blockZ - box.maxZ()));
        const i32 baseY = box.minY() + rigid.groundLevelDelta;
        const i32 dy = blockY - baseY;

        switch (rigid.terrainAdaptation) {
            case TerrainAdaptation::None:
                break;

            case TerrainAdaptation::Bury:
                d0 += getBuryContribution(static_cast<f64>(dx), static_cast<f64>(dy) / 2.0, static_cast<f64>(dz));
                break;

            case TerrainAdaptation::BeardThin:
            case TerrainAdaptation::BeardBox: {
                const i32 i2 = (rigid.terrainAdaptation == TerrainAdaptation::BeardBox)
                    ? std::max(0, std::max(baseY - blockY, blockY - box.maxY()))
                    : dy;
                d0 += getBeardContribution(dx, i2, dz, dy) * 0.8;
                break;
            }

            case TerrainAdaptation::Encapsulate: {
                const i32 i2 = std::max(0, std::max(box.minY() - blockY, blockY - box.maxY()));
                d0 += getBuryContribution(
                          static_cast<f64>(dx) / 2.0, static_cast<f64>(i2) / 2.0, static_cast<f64>(dz) / 2.0) *
                    0.8;
                break;
            }
        }
    }

    // 处理 JigsawJunction（连接点）
    for (const auto& junction : m_junctions) {
        const i32 jx = blockX - junction.getSourceX();
        const i32 jy = blockY - junction.getSourceGroundY();
        const i32 jz = blockZ - junction.getSourceZ();
        d0 += getBeardContribution(jx, jy, jz, jy) * 0.4;
    }

    return d0;
}

// ============================================================================
// Beardifier 静态工具方法
// ============================================================================

f64 Beardifier::getBeardContribution(i32 dx, i32 dy, i32 dz, i32 baseDy)
{
    // MC 1.21: Beardifier.getBeardContribution
    // 胡须曲线：归一化垂直分量 × 高斯核
    const f64 adjustedY = static_cast<f64>(baseDy) + 0.5;
    const f64 distSq = static_cast<f64>(dx * dx) + adjustedY * adjustedY + static_cast<f64>(dz * dz);

    // 高斯衰减：exp(-(distSq/2 + distXZ/2) / 16) 简化为 exp(-distSq/16)
    const f64 gaussian = std::exp(-distSq / 16.0);

    // 胡须曲线：-adjustedY * fastInvSqrt(distSq/2) / 2
    const f64 invSqrt = math::fastInverseSqrt(static_cast<f32>(distSq / 2.0));
    const f64 beard = -adjustedY * invSqrt / 2.0;

    return beard * gaussian;
}

f64 Beardifier::getBuryContribution(f64 dx, f64 dy, f64 dz)
{
    // MC 1.21: Beardifier.getBuryContribution
    // 线性距离衰减，将结构埋入地下
    const f64 distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    // clampedMap(distance, 0, 6, 1, 0) = clamp((6 - distance) / 6, 0, 1)
    if (distance >= 6.0) {
        return 0.0;
    }
    if (distance <= 0.0) {
        return 1.0;
    }
    return (6.0 - distance) / 6.0;
}

f64 Beardifier::computeBeardContribution(i32 dx, i32 dy, i32 dz)
{
    // 旧版兼容：NoiseChunkGenerator::calculateStructureDensityOffset 的精确复刻
    // 高斯衰减 × 胡须曲线
    const f64 d0 = static_cast<f64>(dx * dx) + static_cast<f64>(dz * dz);
    const f64 d1 = static_cast<f64>(dy) + 0.5;
    const f64 d2 = d1 * d1;

    // 高斯衰减
    const f64 d3 = std::exp(-(d2 / 16.0 + d0 / 16.0));

    // 快速逆平方根
    const f64 d4 = -d1 * math::fastInverseSqrt(static_cast<f32>((d2 / 2.0 + d0 / 2.0))) / 2.0;

    return d4 * d3;
}

} // namespace mc::world::gen::density
