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
#include "common/core/Types.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "server/world/gen/jigsaw/JigsawJunction.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

namespace mc {
namespace world::gen::structure {
class Structure;
class StructureStart;
} // namespace world::gen::structure

namespace world::gen::density {

/**
 * @brief Beardifier 密度函数 — 结构地形平滑
 *
 * 计算结构片段和 JigsawJunction 对地形密度的影响。
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
     * @brief 结构片段数据（对应原版 Beardifier.Rigid）
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
     *
     * 影响范围包围盒由构造器按片段与连接点推导（并集向各方向膨胀 24 格）；没有任何数据时
     * 包围盒为空，任何查询点都返回 0。
     *
     * @param pieces 结构片段列表
     * @param junctions JigsawJunction 列表
     */
    Beardifier(std::vector<Rigid> pieces, std::vector<jigsaw::JigsawJunction> junctions);

    /**
     * @brief 用给定的影响范围包围盒构建 Beardifier
     *
     * 与区块构建路径共用同一套采样逻辑，区别只在包围盒由调用方给出：原版允许用任意的
     * 包围盒构造（测试与差分语料都依赖这一点），区块路径用的则是推导出的那一个。
     *
     * @param pieces 结构片段列表
     * @param junctions JigsawJunction 列表
     * @param affectedBox 影响范围包围盒；为空表示任何查询点都返回 0
     */
    static Beardifier fromRigidPieces(std::vector<Rigid> pieces,
        std::vector<jigsaw::JigsawJunction> junctions,
        std::optional<StructureBoundingBox> affectedBox);

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override;

    [[nodiscard]] f64 minValue() const override
    {
        return -std::numeric_limits<f64>::infinity();
    }

    [[nodiscard]] f64 maxValue() const override
    {
        return std::numeric_limits<f64>::infinity();
    }

    /** 是否有结构数据 */
    [[nodiscard]] bool isEmpty() const
    {
        return m_pieces.empty() && m_junctions.empty();
    }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        return visitor.apply(std::make_unique<Beardifier>(m_pieces, m_junctions));
    }

    // ========== 静态工具方法 ==========

    /**
     * @brief 计算 Beard 贡献值（高斯核 + 胡须曲线）
     *
     * @param dx 查询点到片段包围盒的水平 X 距离（包围盒外为正值，内为 0）
     * @param dy 查询点相对包围盒最低点（含 groundLevelDelta）的竖直偏移
     * @param dz 查询点到片段包围盒的水平 Z 距离
     * @param baseDy 胡须曲线的基准竖直偏移（BeardThin 时等于 dy）
     */
    [[nodiscard]] static f64 getBeardContribution(i32 dx, i32 dy, i32 dz, i32 baseDy);

    /**
     * @brief 计算 Bury 贡献值（线性距离衰减）
     *
     * 距离在 0..6 内从 1 线性降到 0，超出 6 恒为 0。
     */
    [[nodiscard]] static f64 getBuryContribution(f64 dx, f64 dy, f64 dz);

    /**
     * @brief 影响范围包围盒（所有 pieces/junctions 的并集向各方向膨胀 24 格）
     *
     * 供探针与调试代码对照「采样点是否落在提前退出范围之外」；为空表示没有结构数据，
     * 此时任何查询点都返回 0。
     */
    [[nodiscard]] const std::optional<StructureBoundingBox>& affectedBox() const noexcept
    {
        return m_affectedBox;
    }

    /** @brief 结构片段列表（按加入顺序保留，累加顺序参与数值结果）。 */
    [[nodiscard]] const std::vector<Rigid>& pieces() const noexcept
    {
        return m_pieces;
    }

    /** @brief JigsawJunction 列表（按加入顺序保留，累加顺序参与数值结果）。 */
    [[nodiscard]] const std::vector<jigsaw::JigsawJunction>& junctions() const noexcept
    {
        return m_junctions;
    }

private:
    /// 高斯核半径（格）
    static constexpr i32 BEARD_KERNEL_RADIUS = 12;

    std::vector<Rigid> m_pieces;
    std::vector<jigsaw::JigsawJunction> m_junctions;

    /// 影响范围包围盒（所有 pieces/junctions 的并集 + 2 × BEARD_KERNEL_RADIUS 膨胀）
    /// 用于早期退出：查询点在包围盒外时直接返回 0
    std::optional<StructureBoundingBox> m_affectedBox;

    /**
     * @brief 计算所有 pieces 和 junctions 的膨胀包围盒
     * @return 膨胀后的包围盒，如果无数据则返回 nullopt
     */
    [[nodiscard]] std::optional<StructureBoundingBox> computeAffectedBox() const;
};

/**
 * @brief 从结构起点筛出的地形适配输入
 *
 * 只包含真正参与计算的片段与连接点：结构的地形适配为「无」时整条结构被跳过，
 * 离目标区块超过核半径的片段与连接点也被剔除。
 */
struct BeardifierData {
    std::vector<Beardifier::Rigid> pieces;
    std::vector<jigsaw::JigsawJunction> junctions;
};

/**
 * @brief 从一批结构起点中收集地形适配输入
 *
 * 片段与连接点只有在与目标区块的距离不超过核半径（12 格）时才参与；距离更远时核权重恒为
 * 0，提前剔除只是少做无用功。连接点按片段顺序逐个收集，累加顺序因此与原版一致。
 *
 * @param starts 与目标区块相交的结构起点（来自 STRUCTURE_REFERENCES 阶段的引用），
 *               每项附带该起点对应的结构定义（决定地形适配模式）
 * @param chunkMinX 目标区块最小 X 方块坐标
 * @param chunkMinZ 目标区块最小 Z 方块坐标
 */
[[nodiscard]] BeardifierData collectBeardifierData(
    const std::vector<std::pair<const structure::StructureStart*, const structure::Structure*>>& starts,
    i32 chunkMinX,
    i32 chunkMinZ);

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

    [[nodiscard]] f64 compute(i32, i32, i32) const override
    {
        return 0.0;
    }

    [[nodiscard]] f64 minValue() const override
    {
        return 0.0;
    }

    [[nodiscard]] f64 maxValue() const override
    {
        return 0.0;
    }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        return visitor.apply(std::make_unique<BeardifierMarker>());
    }
};

} // namespace world::gen::density
} // namespace mc
