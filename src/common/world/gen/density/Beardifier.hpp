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

#pragma once

#include "common/core/Constants.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/jigsaw/JigsawJunction.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace mc::world::gen::density {

/**
 * @brief Beardifier 密度函数 — MC 1.21 结构地形平滑
 *
 * 计算结构片段和 JigsawJunction 对地形密度的影响。
 * 替代旧版 _collectStructureData() + 内联密度偏移计算。
 *
 * 核心算法：
 * - Bury: 线性距离衰减，将结构埋入地下
 * - BeardThin/BeardBox: 高斯核 + 胡须曲线，平滑结构周围地形
 * - Encapsulate: 半分辨率 Bury + 0.8 衰减，完全包裹结构
 * - JigsawJunction: 胡须曲线 + 0.4 衰减，连接结构片段
 *
 * 预计算的 BEARD_KERNEL[24x24x24] = computeBeardContribution(j-12, k-12, i-12)
 */
class Beardifier final : public DensityFunction {
public:
    using StructureBoundingBox = world::gen::structure::StructureBoundingBox;

    /**
     * @brief 结构片段数据（MC Beardifier.Rigid）
     */
    struct Rigid {
        StructureBoundingBox box;            ///< 片段包围盒
        TerrainAdaptation terrainAdaptation; ///< 地形适配类型
        i32 groundLevelDelta;                ///< 地面高度偏移
    };

    /**
     * @brief 空的 Beardifier（无结构影响）
     */
    static const Beardifier EMPTY;

    /**
     * @brief 从区块中的结构数据构建 Beardifier
     * @param pieces 结构片段列表
     * @param junctions JigsawJunction 列表
     */
    Beardifier(std::vector<Rigid> pieces, std::vector<jigsaw::JigsawJunction> junctions);

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override;
    [[nodiscard]] f64 minValue() const override { return -std::numeric_limits<f64>::infinity(); }
    [[nodiscard]] f64 maxValue() const override { return std::numeric_limits<f64>::infinity(); }

    /** 是否有结构数据 */
    [[nodiscard]] bool isEmpty() const { return m_pieces.empty() && m_junctions.empty(); }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        return visitor.apply(std::make_unique<Beardifier>(m_pieces, m_junctions));
    }

    // ========== 静态工具方法 ==========

    /**
     * @brief 计算 Beard 贡献值（高斯核 + 胡须曲线）
     * MC 1.21: Beardifier.getBeardContribution
     */
    [[nodiscard]] static f64 getBeardContribution(i32 dx, i32 dy, i32 dz, i32 baseDy);

    /**
     * @brief 计算 Bury 贡献值（线性距离衰减）
     * MC 1.21: Beardifier.getBuryContribution
     */
    [[nodiscard]] static f64 getBuryContribution(f64 dx, f64 dy, f64 dz);

    /**
     * @brief 计算结构密度偏移（兼容旧版 calculateStructureDensityOffset）
     */
    [[nodiscard]] static f64 computeBeardContribution(i32 dx, i32 dy, i32 dz);

private:
    /// MC 1.21: Beardifier.BEARD_KERNEL_RADIUS = 12
    static constexpr i32 BEARD_KERNEL_RADIUS = 12;

    std::vector<Rigid> m_pieces;
    std::vector<jigsaw::JigsawJunction> m_junctions;

    /// 影响范围包围盒（所有 pieces/junctions 的并集 + BEARD_KERNEL_RADIUS 膨胀）
    /// 用于早期退出：查询点在包围盒外时直接返回 0
    std::optional<StructureBoundingBox> m_affectedBox;

    /**
     * @brief 计算所有 pieces 和 junctions 的膨胀包围盒
     * @return 膨胀后的包围盒，如果无数据则返回 nullopt
     */
    [[nodiscard]] std::optional<StructureBoundingBox> computeAffectedBox() const;
};

/**
 * @brief Beardifier 标记密度函数 — MC 1.21 BeardifierMarker
 *
 * BeardifierMarker 在密度函数树中作为占位符（由 DensityFunctionTypeRegistry createBeardifier 构造）。
 * 当 NoiseChunk 创建时，会被替换为实际的 Beardifier 实例。
 * 如果区块中无结构，则替换为零值常量。
 */
class BeardifierMarker final : public DensityFunction {
public:
    static const BeardifierMarker INSTANCE;

    [[nodiscard]] f64 compute(i32, i32, i32) const override { return 0.0; }
    [[nodiscard]] f64 minValue() const override { return 0.0; }
    [[nodiscard]] f64 maxValue() const override { return 0.0; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        return visitor.apply(std::make_unique<BeardifierMarker>());
    }
};

} // namespace mc::world::gen::density
