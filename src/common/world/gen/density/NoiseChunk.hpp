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

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include "common/world/biome/climate/Sampler.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/aquifer/Aquifer.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::world::gen::density {

// 前向声明（NoiseInterpolator/CellCache/CacheOnce 的方法引用 NoiseChunk）
class NoiseChunk;

} // namespace mc::world::gen::density

namespace mc::world::gen {
class RandomState; // 方案X 阶段5-7：NoiseChunk 构造从 RandomState 取维度级编译产物
} // namespace mc::world::gen

namespace mc::world::gen::density {

// ============================================================================
// BlockStateFiller — MC 1.21 NoiseChunk.BlockStateFiller
// ============================================================================

/**
 * @brief 方块状态填充器接口
 *
 * MC 1.21 对应 NoiseChunk.BlockStateFiller。
 * 在 NoiseChunk 的插值循环中，对每个方块位置调用 calculate() 确定最终方块状态。
 * 链式调用：MaterialRuleList 依次调用 AquiferFiller → OreVeinifier → default block。
 */
class BlockStateFiller {
public:
    virtual ~BlockStateFiller() = default;

    /**
     * @brief 计算指定位置的方块状态
     * @param blockX 方块 X 坐标
     * @param blockY 方块 Y 坐标
     * @param blockZ 方块 Z 坐标
     * @param density 当前方块的最终密度值（已含 beardifier 贡献）
     * @return 方块状态指针，nullptr 表示不替换（使用默认方块）
     */
    [[nodiscard]] virtual const BlockState* calculate(i32 blockX, i32 blockY, i32 blockZ, f64 density) = 0;
};

/**
 * @brief 方块状态规则链 — MC 1.21 MaterialRuleList
 *
 * 持有多个 BlockStateFiller，依次调用直到返回非 nullptr。
 * 对应 MC 的 DensityAquiferFiller + OreVeinifier 链。
 */
class MaterialRuleList final : public BlockStateFiller {
public:
    explicit MaterialRuleList(std::vector<std::unique_ptr<BlockStateFiller>> rules)
        : m_rules(std::move(rules))
    {}

    [[nodiscard]] const BlockState* calculate(i32 blockX, i32 blockY, i32 blockZ, f64 density) override
    {
        for (auto& rule : m_rules) {
            if (const BlockState* state = rule->calculate(blockX, blockY, blockZ, density)) {
                return state;
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<BlockStateFiller>> m_rules;
};

/**
 * @brief 含水层方块状态填充器 — MC 1.21 DensityAquiferFiller
 *
 * 在 density <= 0 时查询含水层系统确定流体/空气。
 * 在 density > 0 时返回 nullptr（保持默认方块）。
 */
class AquiferFiller final : public BlockStateFiller {
public:
    AquiferFiller(aquifer::Aquifer& aquifer)
        : m_aquifer(aquifer)
    {}

    [[nodiscard]] const BlockState* calculate(i32 blockX, i32 blockY, i32 blockZ, f64 density) override
    {
        return m_aquifer.computeSubstance(blockX, blockY, blockZ, density);
    }

private:
    aquifer::Aquifer& m_aquifer;
};

/** disabled aquifer filler — density > 0 → nullptr, density <= 0 → fluid or air */
class DisabledAquiferFiller final : public BlockStateFiller {
public:
    DisabledAquiferFiller(const BlockState* defaultFluid, i32 seaLevel)
        : m_defaultFluid(defaultFluid)
        , m_seaLevel(seaLevel)
    {}

    [[nodiscard]] const BlockState* calculate(i32 blockX, i32 blockY, i32 blockZ, f64 density) override
    {
        (void)blockX;
        (void)blockZ;
        if (density > 0.0) {
            return nullptr; // 固体（由调用方替换为默认方块）
        }
        // density <= 0: 空间 — 海平面以下返回流体，以上返回空气
        if (m_defaultFluid && blockY < m_seaLevel) {
            return m_defaultFluid;
        }
        // MC 1.21: 海平面以上返回空气 BlockState（非 nullptr）
        return VanillaBlocks::getState(VanillaBlocks::AIR);
    }

private:
    const BlockState* m_defaultFluid;
    i32 m_seaLevel;
};

/**
 * @brief 三线性插值器 — MC 1.21 NoiseChunk.NoiseInterpolator
 *
 * 每个 Interpolated 密度函数拥有一个 NoiseInterpolator 实例。
 * 在 cell 角点之间进行三线性插值。
 * 使用双 slice 缓冲区避免重复计算：每列 X 只计算一次。
 *
 * 插值顺序: Y → X → Z（与 MC 一致）
 *
 * MC 1.21 compute() 行为：
 * - 如果不在 NoiseChunk 上下文中：委托给原始函数
 * - 如果不在插值循环中：报错（不应发生）
 * - 如果 fillingCell == true：直接做三线性插值（lerp3），使用 NoiseChunk 的 inCellX/Y/Z
 * - 否则：返回 updateForZ() 递增更新的 m_value
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

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newFiller = m_filler->mapAll(visitor);
        return visitor.apply(std::make_unique<NoiseInterpolator>(std::move(newFiller), m_cellCountZ, m_cellCountY));
    }

    /**
     * @brief 绑定到所属的 NoiseChunk
     * 在 NoiseChunk::apply() 注册插值器后调用。
     */
    void bindNoiseChunk(const class NoiseChunk* noiseChunk, i32 cellWidth, i32 cellHeight)
    {
        m_noiseChunk = noiseChunk;
        m_cellWidth = cellWidth;
        m_cellHeight = cellHeight;
    }

    /**
     * @brief 采样密度函数填充指定 X 列的 slice 数据
     * MC 1.21: 由 NoiseChunk.fillSlice() 调用，使用 fillArray 模式。
     * @param noiseChunk 所属的 NoiseChunk（用于设置上下文和计数器）
     * @param isSlice0 true 填充 slice0，false 填充 slice1
     * @param cellX X 方向 cell 索引
     */
    void fillSlice(class NoiseChunk& noiseChunk, bool isSlice0, i32 cellX);

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
     * @brief 更新 Z 方向插值（第三步）
     * @param delta Z 方向插值因子 [0, 1]
     * @return 插值后的密度值
     */
    f64 updateForZ(f64 delta);

    /**
     * @brief 交换两个 slice 缓冲区
     */
    void swapSlices();

    /**
     * @brief 重置 m_valueReady 标志
     * 在 fillSlice 之前调用，确保 fillSlice 期间 NoiseInterpolator::compute()
     * 委托给原始函数计算角点值，而非返回上一个 cell 的过期 m_value。
     */
    void resetValueReady() { m_valueReady = false; }

private:
    std::unique_ptr<DensityFunction> m_filler;

    /// 所属的 NoiseChunk（用于 fillingCell 检查和 inCellX/Y/Z 访问）
    const class NoiseChunk* m_noiseChunk = nullptr;
    i32 m_cellWidth = 0;
    i32 m_cellHeight = 0;

    /// slice0: 当前 X 列左侧角点数据，扁平布局 [z * m_yPoints + y]
    /// （原 vector<vector<f64>> [z][y] 扁平化，消除每 interpolator ~10 次内层 vector 堆分配）
    std::vector<f64> m_slice0;
    /// slice1: 当前 X 列右侧角点数据，扁平布局 [z * m_yPoints + y]
    std::vector<f64> m_slice1;
    /// Y 方向角点数（= cellCountY + 1），扁平索引步长
    i32 m_yPoints = 0;

    /// 当前 cell 的 8 个角点值（命名: noise_XYZ, X=slice0/1, Y=low/high, Z=front/back）
    f64 m_noise000 = 0.0, m_noise010 = 0.0, m_noise001 = 0.0, m_noise011 = 0.0;
    f64 m_noise100 = 0.0, m_noise110 = 0.0, m_noise101 = 0.0, m_noise111 = 0.0;

    /// Y 插值后的 4 个值
    f64 m_valueXZ00 = 0.0, m_valueXZ10 = 0.0, m_valueXZ01 = 0.0, m_valueXZ11 = 0.0;

    /// X 插值后的 2 个值
    f64 m_valueZ0 = 0.0, m_valueZ1 = 0.0;

    /// 最终插值结果
    f64 m_value = 0.0;

    /// MC 1.21: m_value 是否已通过 updateForZ 设置有效值
    /// 在 fillSlice 期间，selectCellYZ 还没被调用，m_value 无效，
    /// compute() 应委托给 m_filler->compute() 而非返回 m_value
    bool m_valueReady = false;

    i32 m_cellCountZ;
    i32 m_cellCountY;
};

/**
 * @brief CacheAllInCell 包装器 — MC 1.21 NoiseChunk.CacheAllInCell
 *
 * 在 selectCellYZ 时预计算整个 cell 内所有位置的值，
 * 然后在 cell 内直接查表，避免重复计算。
 * finalDensity 就被 CacheAllInCell 包装。
 *
 * MC 1.21 compute() 行为：
 * - 如果不在 NoiseChunk 上下文中：委托给原始函数
 * - 如果不在插值循环中：报错
 * - 否则：使用 inCellX/Y/Z 查表，越界时委托给原始函数
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
     * @brief 绑定到所属的 NoiseChunk
     * 在 NoiseChunk::apply() 注册 CellCache 时调用。
     */
    void bindNoiseChunk(class NoiseChunk* noiseChunk) { m_noiseChunk = noiseChunk; }

    /**
     * @brief 重置缓存状态
     * MC 1.21: 在 fillSlice 之前调用，防止 fillSlice 期间 CellCache
     * 查表返回上一个 cell 的过期缓存值。
     * MC Java 通过 fillArray 机制绕过缓存，C++ 简化为重置标志。
     */
    void invalidate() { m_filled = false; }

    /**
     * @brief 预填充当前 cell 的所有值
     * MC 1.21: 由 NoiseChunk.selectCellYZ() 调用，使用 fillArray/fillAllDirectly 模式。
     * @param noiseChunk 所属的 NoiseChunk
     */
    void fillCell(class NoiseChunk& noiseChunk);

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newFiller = m_filler->mapAll(visitor);
        return visitor.apply(std::make_unique<CellCache>(std::move(newFiller), m_cellWidth, m_cellHeight));
    }

private:
    std::unique_ptr<DensityFunction> m_filler;
    i32 m_cellWidth;
    i32 m_cellHeight;
    std::vector<f64> m_values;

    /// 所属的 NoiseChunk（用于访问 inCellX/Y/Z 等上下文）
    class NoiseChunk* m_noiseChunk = nullptr;

    bool m_filled = false;
};

/**
 * @brief CacheOnce 包装器 — MC 1.21 NoiseChunk.CacheOnce
 *
 * 在同一次插值步骤内缓存计算结果。
 * 使用 NoiseChunk 的 interpolationCounter 检测是否在同一插值位置。
 *
 * MC 1.21 两级缓存：
 * 1. 数组级缓存（arrayInterpolationCounter）：在 fillSlice/selectCellYZ 期间，
 *    同一个 arrayInterpolationCounter 值意味着相同的 slice 位置，可以复用整个数组。
 * 2. 位置级缓存（interpolationCounter）：在 updateForZ 期间，
 *    同一个 interpolationCounter 值意味着同一个方块位置，返回缓存值。
 */
class CacheOnce final : public DensityFunction {
public:
    explicit CacheOnce(std::unique_ptr<DensityFunction> input);

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override;
    [[nodiscard]] f64 minValue() const override { return m_input->minValue(); }
    [[nodiscard]] f64 maxValue() const override { return m_input->maxValue(); }

    [[nodiscard]] const DensityFunction& input() const { return *m_input; }

    /**
     * @brief 绑定到 NoiseChunk 的插值计数器和数组计数器
     * 在 NoiseChunk::apply() 中替换 Marker::CacheOnce 时调用。
     */
    void bindInterpolationCounter(const u64* counter, const u64* arrayCounter, const i32* arrayIndex)
    {
        m_interpolationCounter = counter;
        m_arrayInterpolationCounter = arrayCounter;
        m_arrayIndex = arrayIndex;
    }

    /**
     * @brief 绑定到所属的 NoiseChunk
     * 在 NoiseChunk::apply() 注册 CacheOnce 时调用。
     * 用于复刻原版 Java CacheOnce.compute 的 FunctionContext != NoiseChunk.this 身份检查：
     * 非 NoiseChunk 采样上下文（如 FlatCache 预计算、generateBiomes/buildSurface/applyCarvers
     * 等非插值路径）时委托给 m_input，绕过缓存。
     */
    void bindNoiseChunk(class NoiseChunk* noiseChunk) { m_noiseChunk = noiseChunk; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newInput = m_input->mapAll(visitor);
        return visitor.apply(std::make_unique<CacheOnce>(std::move(newInput)));
    }

private:
    std::unique_ptr<DensityFunction> m_input;
    const u64* m_interpolationCounter = nullptr;
    const u64* m_arrayInterpolationCounter = nullptr;
    const i32* m_arrayIndex = nullptr;
    /// 所属的 NoiseChunk（用于 interpolating() 判断是否处于采样上下文）
    class NoiseChunk* m_noiseChunk = nullptr;
    mutable u64 m_lastCounter = 0;
    mutable f64 m_lastValue = 0.0;
    mutable u64 m_lastArrayCounter = 0;
    mutable std::vector<f64> m_lastArray;
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
 * 工作流程（方案X 阶段5-7：维度级编译产物 newInstance 组装区块级 router）：
 * 1. 构造时从 RandomState 维度级编译产物 newInstance 把 MARKER 占位替换为 per-chunk 缓存对象，
 *    CompiledDensityFunctionAdapter 包装组装 m_router（finalDensity OOP 组装 CellCache(Add(...))）
 * 2. initializeForFirstCellX() — 填充第一个 X 列的 slice0
 * 3. advanceCellX() — 填充下一个 X 列到 slice1
 * 4. selectCellYZ() — 选择当前 cell，预填充 CellCache
 * 5. updateForY/X/Z() — 增量式三线性插值
 * 6. swapSlices() — 切换 slice 缓冲区
 */
class NoiseChunk {
public:
    // NoiseInterpolator 和 CellCache 需要访问 NoiseChunk 的私有字段
    // （cellStartBlockX/Y/Z, inCellX/Y/Z, arrayInterpolationCounter, interpolationCounter, arrayIndex）
    friend class NoiseInterpolator;
    friend class CellCache;
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
     * @param randomState 维度级随机状态（方案X 阶段5-7：取 compiledRouter 维度级编译产物，
     *                    每 slot newInstance 得区块级求值器，CompiledDensityFunctionAdapter 包装组装 m_router）
     * @param cellWidth X/Z 方向 cell 宽度（方块数，通常 4 或 8）
     * @param cellHeight Y 方向 cell 高度（方块数，通常 8 或 4）
     * @param cellCountY Y 方向 cell 数量（由 noiseHeight/cellHeight 计算得出）
     * @param startBlockX 区块起始 X 方块坐标
     * @param startBlockY 区块起始 Y 方块坐标（= noiseSettings.minY）
     * @param startBlockZ 区块起始 Z 方块坐标
     * @param beardifier Beardifier 密度函数（结构地形贡献），区块生成传真实 Beardifier，
     *                   高度查询传 BeardifierMarker（零贡献），nullptr 兜底为零贡献
     * @param cellCountXZ X/Z 方向 cell 数量（默认 CHUNK_WIDTH/cellWidth=4 用于区块生成，
     *                    传入 1 用于单列查询 getBaseColumn/getHeight）
     *
     * 方案X 阶段5-7：不再每区块整树深拷贝。改为从 RandomState 维度级编译产物
     * newInstance（含 Marker 的 slot 把占位替换为 per-chunk 缓存对象）+ Adapter 组装新 m_router。
     * finalDensity 区块级 OOP 组装为 CellCache(Add(Adapter(finalDensity区块级), beardifier))，
     * 对齐原 Marker(CacheAllInCell, Add(finalDensity, BeardifierMarker))→apply 替换语义。
     * Beardifier 始终区块期 OOP 注入（不进编译产物）。
     */
    NoiseChunk(const ::mc::world::gen::RandomState& randomState,
        i32 cellWidth,
        i32 cellHeight,
        i32 cellCountY,
        i32 startBlockX,
        i32 startBlockY,
        i32 startBlockZ,
        std::unique_ptr<DensityFunction> beardifier = nullptr,
        i32 cellCountXZ = -1);

    ~NoiseChunk();
    NoiseChunk(const NoiseChunk&) = delete;
    NoiseChunk& operator=(const NoiseChunk&) = delete;
    NoiseChunk(NoiseChunk&&) noexcept;
    NoiseChunk& operator=(NoiseChunk&&) = delete;

    // ========== 坐标查询（充当 FunctionContext）==========

    /**
     * @brief 当前方块 X 坐标
     * MC 1.21: blockX = cellStartBlockX + inCellX
     */
    [[nodiscard]] i32 blockX() const { return m_cellStartBlockX + m_inCellX; }

    /**
     * @brief 当前方块 Y 坐标
     * MC 1.21: blockY = cellStartBlockY + inCellY
     */
    [[nodiscard]] i32 blockY() const { return m_cellStartBlockY + m_inCellY; }

    /**
     * @brief 当前方块 Z 坐标
     * MC 1.21: blockZ = cellStartBlockZ + inCellZ
     */
    [[nodiscard]] i32 blockZ() const { return m_cellStartBlockZ + m_inCellZ; }

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
     * @brief 选中当前 cell 的 YZ 位置（不更新 X 相关状态）
     *
     * MC 1.21: iterateNoiseColumn 使用此方法而非 selectCellXYZ，
     * 因为 advanceCellX(0) 已经设置了 X 方向的 slice 数据。
     * 与 selectCellXYZ 的区别：不更新 m_selectedCellX 和 m_cellStartBlockX。
     *
     * @param cellY Y 方向 cell 索引
     * @param cellZ Z 方向 cell 索引
     */
    void selectCellYZ(i32 cellY, i32 cellZ);

    /**
     * @brief 更新 Y 方向插值
     * MC 1.21: 接受方块 Y 坐标，计算 inCellY = blockY - cellStartBlockY
     * @param blockY 方块 Y 坐标
     * @param delta Y 方向插值因子 [0, 1]
     */
    void updateForY(i32 blockY, f64 delta);

    /**
     * @brief 更新 X 方向插值
     * MC 1.21: 接受方块 X 坐标，计算 inCellX = blockX - cellStartBlockX
     * @param blockX 方块 X 坐标
     * @param delta X 方向插值因子 [0, 1]
     */
    void updateForX(i32 blockX, f64 delta);

    /**
     * @brief 更新 Z 方向插值
     * MC 1.21: 接受方块 Z 坐标，计算 inCellZ = blockZ - cellStartBlockZ
     *
     * @param blockZ 方块 Z 坐标
     * @param delta Z 方向插值因子 [0, 1]
     */
    void updateForZ(i32 blockZ, f64 delta);

    /**
     * @brief 交换 slice 缓冲区
     * 在完成一个 X 列的所有 cell 后调用。
     */
    void swapSlices();

    // ========== 直接采样接口 ==========

    /**
     * @brief 直接采样 finalDensity 在指定方块坐标
     * 不使用插值，直接计算。用于精确查询。
     * mapAll 后的 finalDensity 已包含 Interpolated/CacheAllInCell 包装。
     */
    [[nodiscard]] f64 sampleFinalDensity(i32 blockX, i32 blockY, i32 blockZ) const
    {
        return m_router.finalDensity().compute(blockX, blockY, blockZ);
    }

    /**
     * @brief 采样预备表面高度（MC 1.21 NoiseChunk.preliminarySurfaceLevel）
     *
     * 用于含水层系统判断地下水位高度。
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

    /**
     * @brief 获取缓存气候采样器 — MC 1.21 NoiseChunk.cachedClimateSampler()
     *
     * 使用 NoiseChunk 内部经过 mapAll 包装的密度函数创建 Climate::Sampler。
     * 这些密度函数已被 NoiseInterpolator/CacheOnce/CellCache 包装，
     * 在区块生成上下文中采样时使用插值缓存，性能优于原始采样器。
     *
     * @param spawnTarget 生物群系生成目标点（用于 findSpawnPosition，可以为空）
     * @return 缓存的气候采样器
     */
    [[nodiscard]] biome::climate::Sampler cachedClimateSampler(
        const std::vector<biome::climate::ParameterPoint>& spawnTarget = {});

    // ========== 访问器 ==========

    [[nodiscard]] const NoiseRouter& router() const { return m_router; }
    [[nodiscard]] const CellConfig& cellConfig() const { return m_cellConfig; }
    [[nodiscard]] i32 startBlockX() const { return m_startBlockX; }
    [[nodiscard]] i32 startBlockZ() const { return m_startBlockZ; }
    [[nodiscard]] i32 firstCellX() const { return m_firstCellX; }
    [[nodiscard]] i32 firstCellY() const { return m_firstCellY; }
    [[nodiscard]] i32 firstCellZ() const { return m_firstCellZ; }

    // ========== FlatCache 区块级预计算几何（对齐原版 NoiseChunk.java:155-160）==========
    /// MC 1.21: firstNoiseX = QuartPos.fromBlock(firstNoiseBlockX) = floorDiv(startBlockX, 4)
    /// 区块首个 quart X，作为 FlatCache 数组索引基准
    [[nodiscard]] i32 firstNoiseX() const { return math::floorDiv(m_startBlockX, 4); }
    /// 区块首个 quart Z
    [[nodiscard]] i32 firstNoiseZ() const { return math::floorDiv(m_startBlockZ, 4); }
    /// MC 1.21: noiseSizeXZ = cellCountXZ * cellWidth / 4，quart XZ 网格边长
    /// 区块生成 cellCountXZ=4,cellWidth=4 → 4；单列查询 cellCountXZ=1 → 1
    [[nodiscard]] i32 noiseSizeXZ() const { return m_cellConfig.cellCountXZ * m_cellConfig.cellWidth / 4; }

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
     * @brief 停止插值循环
     *
     * MC 1.21.11: NoiseChunk.stopInterpolation()
     * 在噪声填充完成后调用，将 interpolating 标志设为 false。
     * 后续对插值器或 cell 缓存的意外采样将不会执行。
     */
    void stopInterpolation() { m_interpolating = false; }

    /**
     * @brief 获取 cell 起始方块坐标
     * MC 1.21: cellStartBlockX/Y/Z 是当前 cell 左下角的方块坐标。
     * blockX() = cellStartBlockX + inCellX
     */
    [[nodiscard]] i32 cellStartBlockX() const { return m_cellStartBlockX; }
    [[nodiscard]] i32 cellStartBlockY() const { return m_cellStartBlockY; }
    [[nodiscard]] i32 cellStartBlockZ() const { return m_cellStartBlockZ; }

    /**
     * @brief 获取 cell 内的方块偏移
     */
    [[nodiscard]] i32 inCellX() const { return m_inCellX; }
    [[nodiscard]] i32 inCellY() const { return m_inCellY; }
    [[nodiscard]] i32 inCellZ() const { return m_inCellZ; }

    /**
     * @brief 根据扁平索引计算 inCellX/Y/Z（MC 1.21 forIndex 模式）
     * 用于 CellCache::fillCell() 中遍历 cell 内所有位置时设置上下文。
     * MC 索引顺序: ((cellHeight - 1 - inCellY) * cellWidth + inCellX) * cellWidth + inCellZ
     */
    void setInCellFromIndex(i32 index);

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

    // ========== 区块级newInstance 注册接口（阶段5：CompiledDensityFunction::newInstance 用）==========
    //
    // CompiledDensityFunction::newInstance 把 MARKER 占位替换为 per-chunk 缓存对象，
    // 通过这些方法把缓存对象所有权转入 NoiseChunk 容器并绑定区块上下文，封装对私有成员的访问。

    /**
     * @brief 注册 NoiseInterpolator（所有权转入 m_interpolators），返回裸指针供 RuntimeObject 登记。
     * @param interpolator 已 bindNoiseChunk 的插值器
     * @return 插值器裸指针（存入容器后）
     */
    NoiseInterpolator* registerInterpolator(std::unique_ptr<NoiseInterpolator> interpolator);

    /**
     * @brief 注册 CellCache（所有权转入 m_cellCaches），返回裸指针供 RuntimeObject 登记。
     * @param cellCache 已 bindNoiseChunk 的 cell 缓存
     * @return cell 缓存裸指针（存入容器后）
     */
    CellCache* registerCellCache(std::unique_ptr<CellCache> cellCache);

    /**
     * @brief 绑定 CacheOnce 的插值计数器与 NoiseChunk 上下文
     *
     * CacheOnce::bindInterpolationCounter 需 NoiseChunk 私有成员（m_interpolationCounter/
     * m_arrayInterpolationCounter/m_arrayIndex）的地址，封装此访问供 newInstance 调用。
     * CacheOnce 所有权由 newInstance 的 CompiledDensityFunction::m_ownedCaches 持有（拥有型缓存）。
     *
     * @param cacheOnce 待绑定的 CacheOnce
     */
    void bindCacheOnceCounters(CacheOnce& cacheOnce);

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

    // ========== 方块状态查询 ==========

    /**
     * @brief 获取当前插值位置的方块状态 — MC 1.21 NoiseChunk.getInterpolatedState()
     *
     * 在插值循环中，对每个方块位置调用此方法。
     * 通过 BlockStateFiller 链（AquiferFiller + 矿脉等）确定最终方块状态。
     *
     * @param density 当前方块的最终密度值（已含 beardifier 贡献）
     * @return 方块状态指针，nullptr 表示使用默认方块（石头等）
     */
    [[nodiscard]] const BlockState* getInterpolatedState(f64 density) const
    {
        if (m_blockStateRule) {
            return m_blockStateRule->calculate(blockX(), blockY(), blockZ(), density);
        }
        return nullptr;
    }

    /**
     * @brief 获取最终密度函数（经过 mapAll 包装后的 finalDensity）
     *
     * MC 1.21: 在生成循环中，密度值通过此函数的 compute() 方法获取，
     * 而非通过 updateForZ() 的返回值。updateForZ() 仅更新插值器状态，
     * 最终密度由 densityFunction.compute(context) 计算得到。
     */
    [[nodiscard]] const DensityFunction& finalDensity() const { return m_router.finalDensity(); }

    /**
     * @brief 设置方块状态规则链
     *
     * 在 NoiseChunk 构造后、插值循环前调用。
     * 构建 MaterialRuleList（AquiferFiller + OreVeinifier 等）。
     */
    void setBlockStateRule(std::unique_ptr<BlockStateFiller> rule) { m_blockStateRule = std::move(rule); }

private:
    /// 方案X 阶段5-7：从 RandomState 维度级编译产物 newInstance + Adapter 组装区块级 NoiseRouter。
    /// 在构造函数成员初始化列表调用（this 的 m_cellConfig/m_interpolators/m_cellCaches/m_beardifier
    /// 均在 m_router 之前声明，已初始化）。finalDensity OOP 组装 CellCache(Add(Adapter, beardifier))。
    /// beardifier 为空时兜底为 BeardifierMarker（零贡献）。
    [[nodiscard]] NoiseRouter buildCompiledRouter(
        const ::mc::world::gen::RandomState& randomState, std::unique_ptr<DensityFunction> beardifier);

    CellConfig m_cellConfig;

    i32 m_startBlockX;
    i32 m_startBlockZ;
    i32 m_firstCellX;
    i32 m_firstCellY;
    i32 m_firstCellZ;

    /// MC 1.21: cell 起始方块坐标 — blockX/Y/Z() = cellStartBlock* + inCell*
    i32 m_cellStartBlockX = 0;
    i32 m_cellStartBlockY = 0;
    i32 m_cellStartBlockZ = 0;

    /// Cell 内偏移（MC 1.21: inCellX/Y/Z）
    i32 m_inCellX = 0;
    i32 m_inCellY = 0;
    i32 m_inCellZ = 0;

    /// MC 1.21: 数组填充时的扁平索引（CacheOnce 通过此字段访问）
    i32 m_arrayIndex = 0;

    i32 m_selectedCellX = 0;
    i32 m_selectedCellY = 0;
    i32 m_selectedCellZ = 0;

    /// 状态标志
    bool m_interpolating = false;
    bool m_fillingCell = false;

    /// 插值计数器（CacheOnce 使用，updateForZ 和 fillSlice 中递增）
    u64 m_interpolationCounter = 0;

    /// MC 1.21: 数组插值计数器（CacheOnce 的数组级缓存使用）
    u64 m_arrayInterpolationCounter = 0;

    /// 所有 NoiseInterpolator 实例（由 apply() 注册）
    std::vector<std::unique_ptr<NoiseInterpolator>> m_interpolators;

    /// 所有 CellCache 实例（由 apply() 注册）
    std::vector<std::unique_ptr<CellCache>> m_cellCaches;

    /// 噪声路由器（放在 interpolators/cellCaches 之后，确保析构顺序正确：
    /// router 中的 DensityFunctionReference 引用 interpolators/cellCaches 中的对象，
    /// 必须先销毁 router 再销毁这些容器。方案X 阶段5-7：m_router 由 buildCompiledRouter
    /// 从维度级编译产物 newInstance + Adapter 组装，finalDensity 的 CellCache(Add(Adapter,beardifier))
    /// 中 beardifier 所有权在 m_cellCaches 的 CellCache 内）
    NoiseRouter m_router;

    /// preliminarySurfaceLevel 按 4 方块网格离散化后缓存
    mutable std::unordered_map<i64, i32> m_preliminarySurfaceLevelCache;

    /// 含水层采样器（可能为 nullptr）
    std::unique_ptr<aquifer::Aquifer> m_aquifer;

    /// 方块状态规则链（AquiferFiller + 矿脉等）
    std::unique_ptr<BlockStateFiller> m_blockStateRule;

    /// 缓存气候采样器（首次调用 cachedClimateSampler 时创建）
    std::unique_ptr<biome::climate::Sampler> m_cachedSampler;
};

} // namespace mc::world::gen::density
