/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to any of the conditions:
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
 */

#pragma once

#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::world::gen::aquifer {
class Aquifer;
} // namespace mc::world::gen::aquifer

namespace mc::world::gen::density {

/**
 * @brief 三线性插值器 — MC 1.21 NoiseChunk.NoiseInterpolator
 *
 * 每个 Interpolated 密度函数拥有一个 NoiseInterpolator 实例。
 * 在 cell 角点之间进行三线性插值。
 * 使用双 slice 缓冲区避免重复计算：每列 X 只计算一次。
 *
 * 插值顺序: Y → X → Z（与 MC 一致）
 */
class NoiseInterpolator final : public DensityFunction {
public:
    /**
     * @brief 构造插值器
     * @param filler 被插值的密度函数
     * @param cellCountZ Z 方向 cell 数量
     * @param cellCountY Y 方向 cell 数量
     */
    NoiseInterpolator(std::unique_ptr<DensityFunction> filler, i32 cellCountZ, i32 cellCountY);

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override;
    [[nodiscard]] f64 minValue() const override { return m_filler->minValue(); }
    [[nodiscard]] f64 maxValue() const override { return m_filler->maxValue(); }

    /** 被包装的原始密度函数 */
    [[nodiscard]] const DensityFunction& filler() const { return *m_filler; }

    /**
     * @brief 采样密度函数填充指定 X 列的 slice 数据
     * @param cellX X 方向 cell 索引（角点坐标）
     * @param firstCellZ Z 方向起始角点坐标
     * @param firstCellY Y 方向起始角点坐标
     * @param cellWidthXZ X/Z 方向 cell 宽度（方块数）
     * @param cellHeightY Y 方向 cell 高度（方块数）
     * @param contextProvider 用于批量填充的上下文提供者
     */
    void fillSlice(bool isSlice0, i32 cellX, i32 firstCellZ, i32 firstCellY, i32 cellWidthXZ, i32 cellHeightY);

    /**
     * @brief 选中当前 cell 的 8 个角点值
     * @param cellY Y 方向 cell 索引（0-based）
     * @param cellZ Z 方向 cell 索引（0-based）
     */
    void selectCellYZ(i32 cellY, i32 cellZ);

    /**
     * @brief 更新 Y 方向插值（第一步）
     * @param delta Y 方向插值因子 [0, 1]
     */
    void updateForY(f64 delta);

    /**
     * @brief 更新 X 方向插值（第二步）
     * @param delta X 方向插值因子 [0, 1]
     */
    void updateForX(f64 delta);

    /**
     * @brief 更新 Z 方向插值（第三步，得到最终值）
     * @param delta Z 方向插值因子 [0, 1]
     * @return 插值后的密度值
     */
    [[nodiscard]] f64 updateForZ(f64 delta);

    /**
     * @brief 直接计算指定位置的插值值（不增量更新，用于 CacheAllInCell 填充）
     */
    [[nodiscard]] f64 interpolate(f64 deltaX, f64 deltaY, f64 deltaZ) const;

    /**
     * @brief 交换两个 slice 缓冲区
     */
    void swapSlices();

private:
    std::unique_ptr<DensityFunction> m_filler;

    /// slice0: 当前 X 列左侧角点数据 [z][y]
    std::vector<std::vector<f64>> m_slice0;
    /// slice1: 当前 X 列右侧角点数据 [z][y]
    std::vector<std::vector<f64>> m_slice1;

    /// 当前 cell 的 8 个角点值（命名: noise_XYZ, X=slice0/1, Y=low/high, Z=front/back）
    f64 m_noise000 = 0.0, m_noise010 = 0.0, m_noise001 = 0.0, m_noise011 = 0.0;
    f64 m_noise100 = 0.0, m_noise110 = 0.0, m_noise101 = 0.0, m_noise111 = 0.0;

    /// Y 插值后的 4 个值
    f64 m_valueXZ00 = 0.0, m_valueXZ10 = 0.0, m_valueXZ01 = 0.0, m_valueXZ11 = 0.0;

    /// X 插值后的 2 个值
    f64 m_valueZ0 = 0.0, m_valueZ1 = 0.0;

    /// 最终插值结果
    f64 m_value = 0.0;

    i32 m_cellCountZ;
    i32 m_cellCountY;
};

/**
 * @brief CacheAllInCell 包装器 — MC 1.21 NoiseChunk.CacheAllInCell
 *
 * 在 selectCellYZ 时预计算整个 cell 内所有位置的值，
 * 然后在 cell 内直接查表，避免重复计算。
 * finalDensity 就被 CacheAllInCell 包装。
 */
class CellCache final : public DensityFunction {
public:
    /**
     * @param filler 被包装的密度函数
     * @param cellWidth X/Z 方向 cell 宽度
     * @param cellHeight Y 方向 cell 高度
     */
    CellCache(std::unique_ptr<DensityFunction> filler, i32 cellWidth, i32 cellHeight);

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override;
    [[nodiscard]] f64 minValue() const override { return m_filler->minValue(); }
    [[nodiscard]] f64 maxValue() const override { return m_filler->maxValue(); }

    /** 被包装的原始密度函数 */
    [[nodiscard]] const DensityFunction& filler() const { return *m_filler; }

    /**
     * @brief 预填充当前 cell 的所有值
     * @param blockX0 cell 起始 X 方块坐标
     * @param blockY0 cell 起始 Y 方块坐标
     * @param blockZ0 cell 起始 Z 方块坐标
     */
    void fillCell(i32 blockX0, i32 blockY0, i32 blockZ0);

    /**
     * @brief 设置 cell 内的查询位置
     * @param inCellX cell 内 X 偏移 [0, cellWidth)
     * @param inCellY cell 内 Y 偏移 [0, cellHeight)
     * @param inCellZ cell 内 Z 偏移 [0, cellWidth)
     */
    void setInCellPos(i32 inCellX, i32 inCellY, i32 inCellZ);

    /**
     * @brief 获取当前 cell 内位置的缓存值
     * 必须在 fillCell 和 setInCellPos 之后调用。
     */
    [[nodiscard]] f64 getCachedValue() const;

private:
    std::unique_ptr<DensityFunction> m_filler;
    i32 m_cellWidth;
    i32 m_cellHeight;
    std::vector<f64> m_values;

    /// 当前查询位置在 m_values 中的索引
    i32 m_currentIndex = -1;
    bool m_filled = false;
};

/**
 * @brief CacheOnce 包装器 — MC 1.21 NoiseChunk.CacheOnce
 *
 * 在同一次插值步骤内缓存计算结果。
 * 使用 interpolationCounter 检测是否在同一插值位置。
 */
class CacheOnce final : public DensityFunction {
public:
    explicit CacheOnce(std::unique_ptr<DensityFunction> input);

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override;
    [[nodiscard]] f64 minValue() const override { return m_input->minValue(); }
    [[nodiscard]] f64 maxValue() const override { return m_input->maxValue(); }

    [[nodiscard]] const DensityFunction& input() const { return *m_input; }

private:
    std::unique_ptr<DensityFunction> m_input;
    mutable u64 m_lastCounter = 0;
    mutable f64 m_lastValue = 0.0;
};

/**
 * @brief 区块噪声采样单元 — MC 1.21 NoiseChunk
 *
 * 将区块划分为 cell 网格，在 cell 角点采样密度函数，
 * 通过三线性插值得到每个方块位置的密度值。
 *
 * NoiseChunk 同时充当:
 * - DensityFunction::FunctionContext: 提供当前方块坐标
 * - 密度函数的包装/缓存管理器
 *
 * 主世界 cell 大小: 4×8×4 (X×Y×Z 方块)
 * 末地 cell 大小: 8×4×8
 *
 * 工作流程：
 * 1. 构造时通过 wrap() 包装所有 Marker 类型的密度函数
 * 2. initializeForFirstCellX() — 填充第一个 X 列的 slice0
 * 3. advanceCellX() — 填充下一个 X 列到 slice1
 * 4. selectCellYZ() — 选择当前 cell，预填充 CellCache
 * 5. updateForY/X/Z() — 增量式三线性插值
 * 6. swapSlices() — 切换 slice 缓冲区
 */
class NoiseChunk {
public:
    /**
     * @brief cell 配置参数
     */
    struct CellConfig {
        i32 cellWidth;   ///< X/Z 方向 cell 宽度（方块数）
        i32 cellHeight;  ///< Y 方向 cell 高度（方块数）
        i32 cellCountXZ; ///< X/Z 方向 cell 数量
        i32 cellCountY;  ///< Y 方向 cell 数量
    };

    /**
     * @brief 构造 NoiseChunk
     * @param router 噪声路由器
     * @param cellWidth X/Z 方向 cell 宽度（方块数，通常 4 或 8）
     * @param cellHeight Y 方向 cell 高度（方块数，通常 8 或 4）
     * @param startBlockX 区块起始 X 方块坐标
     * @param startBlockY 区块起始 Y 方块坐标
     * @param startBlockZ 区块起始 Z 方块坐标
     */
    NoiseChunk(
        const NoiseRouter& router, i32 cellWidth, i32 cellHeight, i32 startBlockX, i32 startBlockY, i32 startBlockZ);

    ~NoiseChunk();
    NoiseChunk(const NoiseChunk&) = delete;
    NoiseChunk& operator=(const NoiseChunk&) = delete;
    NoiseChunk(NoiseChunk&&) noexcept;
    NoiseChunk& operator=(NoiseChunk&&) = delete;

    // ========== 坐标查询（充当 FunctionContext）==========

    [[nodiscard]] i32 blockX() const { return m_blockX; }
    [[nodiscard]] i32 blockY() const { return m_blockY; }
    [[nodiscard]] i32 blockZ() const { return m_blockZ; }

    // ========== 区块生成主循环接口 ==========

    /**
     * @brief 初始化第一个 X 列的 slice 数据
     * 在开始遍历 cell 前调用一次。
     */
    void initializeForFirstCellX();

    /**
     * @brief 推进到下一个 X 列
     * 填充 slice1 并准备交换。
     * @param cellX 当前 cell 的 X 索引
     */
    void advanceCellX(i32 cellX);

    /**
     * @brief 选中当前 cell 的 XYZ 位置
     * 加载 8 个角点的密度值到所有插值器，
     * 并预填充所有 CellCache。
     * @param cellX X 方向 cell 索引
     * @param cellY Y 方向 cell 索引
     * @param cellZ Z 方向 cell 索引
     */
    void selectCellXYZ(i32 cellX, i32 cellY, i32 cellZ);

    /**
     * @brief 更新 Y 方向插值
     * @param delta Y 方向插值因子 [0, 1]
     */
    void updateForY(f64 delta);

    /**
     * @brief 更新 X 方向插值
     * @param delta X 方向插值因子 [0, 1]
     */
    void updateForX(f64 delta);

    /**
     * @brief 更新 Z 方向插值并获取最终密度值
     * @param delta Z 方向插值因子 [0, 1]
     * @return 插值后的 finalDensity 值
     */
    [[nodiscard]] f64 updateForZ(f64 delta);

    /**
     * @brief 交换 slice 缓冲区
     * 在完成一个 X 列的所有 cell 后调用。
     */
    void swapSlices();

    // ========== 直接采样接口 ==========

    /**
     * @brief 直接采样 finalDensity 在指定方块坐标
     * 不使用插值，直接计算。用于精确查询。
     */
    [[nodiscard]] f64 sampleFinalDensity(i32 blockX, i32 blockY, i32 blockZ) const;

    /**
     * @brief 采样预备表面高度（MC 1.21 NoiseChunk.preliminarySurfaceLevel）
     *
     * 用于含水层系统判断地下水位高度。
     * 直接调用路由器的 preliminarySurfaceLevel 密度函数。
     *
     * @param blockX 方块 X 坐标
     * @param blockZ 方块 Z 坐标
     * @return 预估表面高度
     */
    [[nodiscard]] i32 samplePreliminarySurfaceLevel(i32 blockX, i32 blockZ) const;

    /**
     * @brief 采样范围内最大的预备表面高度。
     *
     * 含水层用它决定地表以上跳过详细采样的高度边界。
     */
    [[nodiscard]] i32 maxPreliminarySurfaceLevel(i32 minBlockX, i32 minBlockZ, i32 maxBlockX, i32 maxBlockZ) const;

    // ========== 访问器 ==========

    [[nodiscard]] const NoiseRouter& router() const { return m_router; }
    [[nodiscard]] const CellConfig& cellConfig() const { return m_cellConfig; }
    [[nodiscard]] i32 startBlockX() const { return m_startBlockX; }
    [[nodiscard]] i32 startBlockZ() const { return m_startBlockZ; }
    [[nodiscard]] i32 firstCellX() const { return m_firstCellX; }
    [[nodiscard]] i32 firstCellY() const { return m_firstCellY; }
    [[nodiscard]] i32 firstCellZ() const { return m_firstCellZ; }

    /**
     * @brief 获取包装后的 finalDensity（包含 Interpolated/CacheAllInCell 包装）
     */
    [[nodiscard]] const DensityFunction& wrappedFinalDensity() const { return *m_wrappedFinalDensity; }

    /**
     * @brief 获取插值计数器（用于 CacheOnce）
     */
    [[nodiscard]] u64 interpolationCounter() const { return m_interpolationCounter; }

    /**
     * @brief 递增插值计数器
     */
    void incrementInterpolationCounter() { ++m_interpolationCounter; }

    /**
     * @brief 是否正在填充 cell（CacheAllInCell 使用）
     */
    [[nodiscard]] bool fillingCell() const { return m_fillingCell; }

    /**
     * @brief 是否正在插值循环中
     */
    [[nodiscard]] bool interpolating() const { return m_interpolating; }

    /**
     * @brief 设置当前方块坐标（插值过程中使用）
     */
    void setBlockPos(i32 x, i32 y, i32 z)
    {
        m_blockX = x;
        m_blockY = y;
        m_blockZ = z;
    }

    /**
     * @brief 获取 cell 内的方块偏移
     */
    [[nodiscard]] i32 inCellX() const { return m_inCellX; }
    [[nodiscard]] i32 inCellY() const { return m_inCellY; }
    [[nodiscard]] i32 inCellZ() const { return m_inCellZ; }

    void setInCellPos(i32 inCellX, i32 inCellY, i32 inCellZ);

    /**
     * @brief 获取所有插值器
     */
    [[nodiscard]] const std::vector<std::unique_ptr<NoiseInterpolator>>& interpolators() const
    {
        return m_interpolators;
    }

    /**
     * @brief 获取所有 CellCache
     */
    [[nodiscard]] const std::vector<std::unique_ptr<CellCache>>& cellCaches() const { return m_cellCaches; }

    /**
     * @brief 注册一个插值器
     * 在构造后调用，将 NoiseInterpolator 添加到插值器列表。
     */
    void addInterpolator(std::unique_ptr<NoiseInterpolator> interpolator)
    {
        m_interpolators.push_back(std::move(interpolator));
    }

    /**
     * @brief 注册一个 CellCache
     * 在构造后调用，将 CellCache 添加到缓存列表。
     */
    void addCellCache(std::unique_ptr<CellCache> cache) { m_cellCaches.push_back(std::move(cache)); }

    /**
     * @brief 设置包装后的 finalDensity
     */
    void setWrappedFinalDensity(std::unique_ptr<DensityFunction> density)
    {
        m_wrappedFinalDensity = std::move(density);
    }

    /**
     * @brief 设置包装后的 preliminarySurfaceLevel
     */
    void setWrappedPreliminarySurfaceLevel(std::unique_ptr<DensityFunction> density)
    {
        m_wrappedPreliminarySurfaceLevel = std::move(density);
    }

    // ========== 含水层 ==========

    /**
     * @brief 获取含水层采样器
     * 可能为 nullptr（含水层禁用时）。
     */
    [[nodiscard]] aquifer::Aquifer* aquifer() { return m_aquifer.get(); }
    [[nodiscard]] const aquifer::Aquifer* aquifer() const { return m_aquifer.get(); }

    /**
     * @brief 设置含水层采样器
     */
    void setAquifer(std::unique_ptr<aquifer::Aquifer> aq);

private:
    /**
     * @brief 包装密度函数，将 Marker 类型替换为 NoiseChunk 特定实现
     *
     * 将 Interpolated → NoiseInterpolator
     * 将 CacheAllInCell → CellCache（在 selectCellYZ 时预填充）
     * 将 CacheOnce → CacheOnce（使用 interpolationCounter 缓存）
     * 将 Cache2D → 保持原样（已有实现）
     * 将 FlatCache → 保持原样（已有实现）
     */
    std::unique_ptr<DensityFunction> wrap(std::unique_ptr<DensityFunction> function);

    const NoiseRouter& m_router;
    CellConfig m_cellConfig;

    i32 m_startBlockX;
    i32 m_startBlockZ;
    i32 m_firstCellX;
    i32 m_firstCellY;
    i32 m_firstCellZ;

    /// 当前方块坐标（插值过程中设置）
    i32 m_blockX = 0;
    i32 m_blockY = 0;
    i32 m_blockZ = 0;

    /// Cell 内偏移
    i32 m_inCellX = 0;
    i32 m_inCellY = 0;
    i32 m_inCellZ = 0;

    i32 m_selectedCellX = 0;
    i32 m_selectedCellY = 0;
    i32 m_selectedCellZ = 0;

    /// 状态标志
    bool m_interpolating = false;
    bool m_fillingCell = false;

    /// 插值计数器（CacheOnce 使用）
    u64 m_interpolationCounter = 0;

    /// 所有 NoiseInterpolator 实例（由 wrap() 注册）
    std::vector<std::unique_ptr<NoiseInterpolator>> m_interpolators;

    /// 所有 CellCache 实例（由 wrap() 注册）
    std::vector<std::unique_ptr<CellCache>> m_cellCaches;

    /// 包装后的 finalDensity
    std::unique_ptr<DensityFunction> m_wrappedFinalDensity;

    /// 包装后的 preliminarySurfaceLevel
    std::unique_ptr<DensityFunction> m_wrappedPreliminarySurfaceLevel;

    /// preliminarySurfaceLevel 按 4 方块网格离散化后缓存
    mutable std::unordered_map<i64, i32> m_preliminarySurfaceLevelCache;

    /// 含水层采样器（可能为 nullptr）
    std::unique_ptr<aquifer::Aquifer> m_aquifer;
};

} // namespace mc::world::gen::density
