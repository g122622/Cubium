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

#include "Beardifier.hpp"
#include "common/util/math/MathUtils.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace mc::world::gen::density {

// ============================================================================
// BeardifierMarker 静态实例
// ============================================================================

const BeardifierMarker BeardifierMarker::INSTANCE;

// ============================================================================
// BEARD_KERNEL 预计算表 — MC 1.21 Beardifier.BEARD_KERNEL
// ============================================================================

/**
 * MC 1.21.11 预计算 BEARD_KERNEL[24][24][24] 查找表。
 *
 * BEARD_KERNEL[k][i][j] = computeBeardContribution(j - 12, i - 12, k - 12)
 * 其中 k = Y 索引, i = X 索引, j = Z 索引。
 *
 * 用于 getBeardContribution(dx, dy, dz, baseDy) 在 BeardThin/BeardBox
 * 模式下（baseDy == dy 的情况）的快速查找。
 * 当 dx, dy, dz 都在 [-12, 11] 范围内时，直接查表避免 exp() 计算。
 */
static const std::array<f32, 24 * 24 * 24> BEARD_KERNEL = []() {
    std::array<f32, 24 * 24 * 24> kernel{};
    for (i32 i = 0; i < 24; ++i) {
        for (i32 j = 0; j < 24; ++j) {
            for (i32 k = 0; k < 24; ++k) {
                kernel[i * 24 * 24 + j * 24 + k] =
                    static_cast<f32>(Beardifier::computeBeardContribution(j - 12, i - 12, k - 12));
            }
        }
    }
    return kernel;
}();

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
    , m_affectedBox(computeAffectedBox())
{}

// ============================================================================
// Beardifier::compute — MC 1.21 Beardifier.compute
// ============================================================================

f64 Beardifier::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    // MC 1.21: affectedBox 早期退出优化
    // 如果查询点在所有结构的影响范围之外，直接返回 0
    if (m_affectedBox.has_value()) {
        const auto& box = *m_affectedBox;
        if (blockX < box.minX() || blockX > box.maxX() || blockY < box.minY() || blockY > box.maxY() ||
            blockZ < box.minZ() || blockZ > box.maxZ()) {
            return 0.0;
        }
    } else {
        // 无结构数据
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
    //
    // BeardThin 模式下 baseDy == dy，可以直接使用 BEARD_KERNEL 查表。
    // BeardBox 模式下 baseDy != dy（baseDy 替换为到 Y 边界的距离），
    // 需要使用动态计算。
    //
    // 当 dx/dy/dz 在 [-12, 11] 范围内且 baseDy == dy 时，使用预计算表加速。

    // 尝试 BEARD_KERNEL 查表（仅 BeardThin/baseDy==dy 场景）
    if (baseDy == dy && dx >= -12 && dx < 12 && dy >= -12 && dy < 12 && dz >= -12 && dz < 12) {
        const i32 k = dy + 12; // Y 索引 (MC: i)
        const i32 i = dx + 12; // X 索引 (MC: j)
        const i32 j = dz + 12; // Z 索引 (MC: k)
        return static_cast<f64>(BEARD_KERNEL[k * 24 * 24 + i * 24 + j]);
    }

    // Fallback: 动态计算
    const f64 adjustedY = static_cast<f64>(baseDy) + 0.5;
    const f64 distSq = static_cast<f64>(dx * dx) + adjustedY * adjustedY + static_cast<f64>(dz * dz);
    const f64 gaussian = std::exp(-distSq / 16.0);
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
    // 独立计算，不使用 BEARD_KERNEL 查表。
    // BEARD_KERNEL 初始化时调用此函数，因此不能委托给 getBeardContribution。
    const f64 adjustedY = static_cast<f64>(dy) + 0.5;
    const f64 distSq = static_cast<f64>(dx * dx) + adjustedY * adjustedY + static_cast<f64>(dz * dz);
    const f64 gaussian = std::exp(-distSq / 16.0);
    const f64 invSqrt = math::fastInverseSqrt(static_cast<f32>(distSq / 2.0));
    const f64 beard = -adjustedY * invSqrt / 2.0;
    return beard * gaussian;
}

// ============================================================================
// Beardifier::computeAffectedBox
// ============================================================================

std::optional<Beardifier::StructureBoundingBox> Beardifier::computeAffectedBox() const
{
    if (m_pieces.empty() && m_junctions.empty()) {
        return std::nullopt;
    }

    i32 minX = std::numeric_limits<i32>::max();
    i32 minY = std::numeric_limits<i32>::max();
    i32 minZ = std::numeric_limits<i32>::max();
    i32 maxX = std::numeric_limits<i32>::min();
    i32 maxY = std::numeric_limits<i32>::min();
    i32 maxZ = std::numeric_limits<i32>::min();

    for (const auto& rigid : m_pieces) {
        const auto& box = rigid.box;
        minX = std::min(minX, box.minX());
        minY = std::min(minY, box.minY());
        minZ = std::min(minZ, box.minZ());
        maxX = std::max(maxX, box.maxX());
        maxY = std::max(maxY, box.maxY());
        maxZ = std::max(maxZ, box.maxZ());
    }

    for (const auto& junction : m_junctions) {
        minX = std::min(minX, junction.getSourceX());
        minY = std::min(minY, junction.getSourceGroundY());
        minZ = std::min(minZ, junction.getSourceZ());
        maxX = std::max(maxX, junction.getSourceX());
        maxY = std::max(maxY, junction.getSourceGroundY());
        maxZ = std::max(maxZ, junction.getSourceZ());
    }

    // 膨胀 BEARD_KERNEL_RADIUS (12) 格
    minX -= BEARD_KERNEL_RADIUS;
    minY -= BEARD_KERNEL_RADIUS;
    minZ -= BEARD_KERNEL_RADIUS;
    maxX += BEARD_KERNEL_RADIUS;
    maxY += BEARD_KERNEL_RADIUS;
    maxZ += BEARD_KERNEL_RADIUS;

    return StructureBoundingBox(minX, minY, minZ, maxX, maxY, maxZ);
}

} // namespace mc::world::gen::density
