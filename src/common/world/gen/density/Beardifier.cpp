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
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/gen/jigsaw/JigsawJunction.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

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
 * MC Java 的 BEARD_KERNEL 存储布局：
 *   BEARD_KERNEL[i * 24 * 24 + j * 24 + k] = computeBeardContribution(j-12, k-12, i-12)
 * 其中 computeBeardContribution 的第二个参数 (k-12) 获得 +0.5 偏移。
 *
 * 在 getBeardContribution 的查找中：
 *   i = dx+12 (X), j = dy+12 (Y), k = dz+12 (Z)
 *   BEARD_KERNEL[k * 24 * 24 + i * 24 + j]  →  [Z+12][X+12][Y+12]
 *
 * 因此预计算循环变量映射为：
 *   外循环 i → 对应查找的 k → Z 轴
 *   中循环 j → 对应查找的 i → X 轴
 *   内循环 k → 对应查找的 j → Y 轴
 *
 * MC 的 computeBeardContribution(j-12, k-12, i-12) 对第二个参数 (k-12) +0.5，
 * 即对 Y 轴偏移加 0.5。在内循环变量中 k 对应 Y 轴，因此 +0.5 应加在 dz (=k-12) 上。
 */
static const std::array<f32, 24 * 24 * 24> BEARD_KERNEL = []() {
    std::array<f32, 24 * 24 * 24> kernel{};
    for (i32 i = 0; i < 24; ++i) {
        for (i32 j = 0; j < 24; ++j) {
            for (i32 k = 0; k < 24; ++k) {
                const i32 dx = j - 12;     // X 偏移（中循环 j → 查找 i → X）
                const i32 dz_pre = i - 12; // Z 偏移（外循环 i → 查找 k → Z）
                const i32 dy_pre = k - 12; // Y 偏移（内循环 k → 查找 j → Y）
                // MC: +0.5 应用于第二个参数 (k-12)，即 Y 轴偏移
                const f64 adjustedY = static_cast<f64>(dy_pre) + 0.5;
                const f64 distSq =
                    static_cast<f64>(dx * dx) + adjustedY * adjustedY + static_cast<f64>(dz_pre * dz_pre);
                kernel[i * 24 * 24 + j * 24 + k] = static_cast<f32>(std::exp(-distSq / 16.0));
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
    // MC 1.21: Beardifier.getBeardContribution(dx, dy, dz, baseDy)
    //
    // 使用 BEARD_KERNEL 查找高斯分量（以 dy 为 Y 索引），
    // 然后乘以 beard 因子（以 baseDy 计算）。
    //
    // 核查找范围: dx, dy, dz 都在 [-12, 11] 时直接查表，
    // 否则返回 0.0（BeardThin/BeardBox 模式下核值在范围外衰减为 0）。
    //
    // MC Java 逻辑:
    //   i = dx + 12, j = dy + 12, k = dz + 12
    //   if (isInKernelRange(i) && isInKernelRange(j) && isInKernelRange(k)):
    //     d0 = baseDy + 0.5
    //     d1 = lengthSquared(dx, d0, dz)
    //     d2 = -d0 * fastInvSqrt(d1 / 2.0) / 2.0
    //     return d2 * BEARD_KERNEL[k * 24 * 24 + i * 24 + j]
    //   else:
    //     return 0.0

    const i32 i = dx + 12;
    const i32 j = dy + 12;
    const i32 k = dz + 12;

    if (i >= 0 && i < 24 && j >= 0 && j < 24 && k >= 0 && k < 24) {
        const f64 adjustedY = static_cast<f64>(baseDy) + 0.5;
        const f64 distSq = static_cast<f64>(dx * dx) + adjustedY * adjustedY + static_cast<f64>(dz * dz);
        const f64 gaussian = static_cast<f64>(BEARD_KERNEL[static_cast<size_t>(k * 24 * 24 + i * 24 + j)]);
        const f64 beard = -adjustedY * math::fastInverseSqrt(static_cast<f32>(distSq / 2.0)) / 2.0;
        return beard * gaussian;
    }

    return 0.0;
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
    // MC 1.21: Beardifier.computeBeardContribution(dx, dy, dz)
    // computeBeardContribution(int, int, int) 调用 computeBeardContribution(int, double, int)
    // 对第二个参数（Y 轴）加 0.5
    // 注意：此方法现在仅作为备用，BEARD_KERNEL 已直接在初始化时计算。
    const f64 adjustedY = static_cast<f64>(dy) + 0.5;
    const f64 distSq = static_cast<f64>(dx * dx) + adjustedY * adjustedY + static_cast<f64>(dz * dz);
    return std::exp(-distSq / 16.0);
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

    // MC 1.21: boundingBox.inflatedBy(24) — 膨胀 24 格（2 * BEARD_KERNEL_RADIUS）
    // MC 使用 inflatedBy(24) 而非 BEARD_KERNEL_RADIUS(12)，
    // 因为 getBeardContribution 在核范围外返回 0，但 BeardBox 模式下的
    // getBuryContribution 在更大范围内仍有非零贡献
    minX -= 24;
    minY -= 24;
    minZ -= 24;
    maxX += 24;
    maxY += 24;
    maxZ += 24;

    return StructureBoundingBox(minX, minY, minZ, maxX, maxY, maxZ);
}

} // namespace mc::world::gen::density
