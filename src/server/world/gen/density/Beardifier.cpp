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
#include "server/world/gen/jigsaw/JigsawJunction.hpp"
#include "server/world/gen/structure/Structure.hpp"
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
 * @brief BEARD_KERNEL 的第 (dx, dy, dz) 个高斯分量，偏移已限制在核的 24³ 范围内
 *
 * 即 exp(-(dx² + (dy+0.5)² + dz²) / 16)，其中 +0.5 加在 Y 轴（与竖直采样点落在格中心
 * 的约定一致）。核表与「无表」路径都从这里取值，两条路径因此逐位相同。
 */
static f64 kernelGaussian(i32 dx, i32 dy, i32 dz)
{
    const f64 adjustedY = static_cast<f64>(dy) + 0.5;
    const f64 distSq = static_cast<f64>(dx) * dx + adjustedY * adjustedY + static_cast<f64>(dz) * dz;
    return std::exp(-distSq / 16.0);
}

/**
 * @brief 预计算的高斯核查找表（数组是扁平的，按 [[[i]][j]][k] 索引）
 *
 * 布局与查找方式必须保持同步：原版按 `BEARD_KERNEL[k * 24 * 24 + i * 24 + j]` 取值，
 * 其中 i 是 X 偏移、j 是 Y 偏移、k 是 Z 偏移，而填表时写入的是
 * `kernel[i][j][k] = gaussian(j-12, k-12, i-12)`，两边的轴次序不同；填表必须逐字复刻，
 * 否则各个偏移会读到别的轴上的值（表现为结构边缘的密度贡献整体对不上）。
 *
 * 核值是 f32：原版把它存成 float 数组，取值时再拓宽成 double 参与后续乘除。
 */
static const std::array<f32, 24 * 24 * 24> BEARD_KERNEL = []() {
    std::array<f32, 24 * 24 * 24> kernel{};
    for (i32 i = 0; i < 24; ++i) {
        for (i32 j = 0; j < 24; ++j) {
            for (i32 k = 0; k < 24; ++k) {
                kernel[i * 24 * 24 + j * 24 + k] = static_cast<f32>(kernelGaussian(j - 12, k - 12, i - 12));
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

Beardifier Beardifier::fromRigidPieces(std::vector<Rigid> pieces,
    std::vector<jigsaw::JigsawJunction> junctions,
    std::optional<StructureBoundingBox> affectedBox)
{
    Beardifier result(std::move(pieces), std::move(junctions));
    result.m_affectedBox = std::move(affectedBox);
    return result;
}

// ============================================================================
// Beardifier::compute — 结构地形平滑的密度贡献
// ============================================================================

f64 Beardifier::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    // affectedBox 早期退出：查询点在所有结构的影响范围之外时直接返回 0。
    // 包围盒为空（没有任何片段与连接点）同样恒为 0。
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

        // 计算到包围盒边界的水平距离（盒内为 0）
        const i32 dx = std::max(0, std::max(box.minX() - blockX, blockX - box.maxX()));
        const i32 dz = std::max(0, std::max(box.minZ() - blockZ, blockZ - box.maxZ()));
        // 地面线：片段最低点 + 地面高度偏移；查询点相对地面线的竖直偏移就是胡须曲线的基准。
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
                // 核的竖直偏移：薄胡须直接用查询点相对地面线的偏移；方盒胡须用「地面线」
                // 与包围盒顶部构成竖直范围，盒外只按超出该范围的那部分计。下限取地面线
                // （包围盒最低点 + 地面高度偏移）而非包围盒最低点本身，两者相差
                // groundLevelDelta。
                const i32 vertical = (rigid.terrainAdaptation == TerrainAdaptation::BeardBox)
                    ? std::max(0, std::max(baseY - blockY, blockY - box.maxY()))
                    : dy;
                d0 += getBeardContribution(dx, vertical, dz, dy) * 0.8;
                break;
            }

            case TerrainAdaptation::Encapsulate: {
                // 完全包裹：以包围盒自身的竖直范围（不含地面高度偏移）算竖直偏移，
                // 再按半分辨率做埋入衰减。
                const i32 vertical = std::max(0, std::max(box.minY() - blockY, blockY - box.maxY()));
                d0 += getBuryContribution(
                          static_cast<f64>(dx) / 2.0, static_cast<f64>(vertical) / 2.0, static_cast<f64>(dz) / 2.0) *
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
    // 使用 BEARD_KERNEL 查找高斯分量（以 dy 为 Y 索引），
    // 然后乘以 beard 因子（以 baseDy 计算）。
    //
    // 核查找范围: dx, dy, dz 都在 [-12, 11] 时直接查表，
    // 否则返回 0.0（BeardThin/BeardBox 模式下核值在范围外衰减为 0）。
    //
    //   i = dx + 12, j = dy + 12, k = dz + 12
    //   if (isInKernelRange(i) && isInKernelRange(j) && isInKernelRange(k)):
    //     d0 = baseDy + 0.5
    //     d1 = lengthSquared(dx, d0, dz)
    //     d2 = -d0 * invSqrt(d1 / 2.0) / 2.0
    //     return d2 * BEARD_KERNEL[k * 24 * 24 + i * 24 + j]
    //   else:
    //     return 0.0
    //
    // invSqrt 这一项是 Mth.fastInvSqrt：一次位模式初猜 + 一次牛顿迭代的近似，相对误差约
    // 1e-4 量级，**不是**正确舍入的 1/sqrt。原版用近似值参与乘除、结果全程是 f64，
    // 不能换成精确逆平方根——两者在结构边缘的贡献上会相差千分之一量级。
    //
    // 注意 d1 里的 0.5 是核中心在 Y 方向的半个格偏移，与核表无关：核表查的是 dy 处的
    // 高斯分量，beard 因子才用 baseDy 计算。两者在 BeardThin 下相同，在 BeardBox 下不同。

    const i32 i = dx + 12;
    const i32 j = dy + 12;
    const i32 k = dz + 12;

    if (i >= 0 && i < 24 && j >= 0 && j < 24 && k >= 0 && k < 24) {
        const f64 adjustedY = static_cast<f64>(baseDy) + 0.5;
        const f64 distSq = static_cast<f64>(dx) * dx + adjustedY * adjustedY + static_cast<f64>(dz) * dz;
        const f64 gaussian = static_cast<f64>(BEARD_KERNEL[static_cast<size_t>(k * 24 * 24 + i * 24 + j)]);
        const f64 beard = -adjustedY * math::fastInvSqrt(distSq / 2.0) / 2.0;
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
