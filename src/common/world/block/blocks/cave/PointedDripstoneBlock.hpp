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
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/IWaterLoggable.hpp"
#include <optional>
#include <unordered_map>

namespace mc {

namespace fluid {
class Fluid;
} // namespace fluid

namespace blocks {

/**
 * @brief 滴石尖锥方块
 *
 * 可上下放置的钟乳石/石笋方块，支持含水。
 * 根据厚度属性有不同的碰撞箱形状。
 * 支持随机刻生长、滴水、支撑失效掉落等机制。
 */
class PointedDripstoneBlock : public Block, public IWaterLoggable {
public:
    explicit PointedDripstoneBlock(const BlockProperties& properties);

    ~PointedDripstoneBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void onFallenUpon(
        IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance) override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

public:
    // ========== 静态辅助方法 ==========

    /// 判断方块状态是否为指定方向的滴石
    [[nodiscard]] static bool isPointedDripstoneWithDirection(const BlockState* state, Direction direction);

    /// 判断是否为钟乳石（朝下）
    [[nodiscard]] static bool isStalactite(const BlockState& state);

    /// 判断是否为石笋（朝上）
    [[nodiscard]] static bool isStalagmite(const BlockState& state);

    /// 判断是否为尖端（allowMerge=true时TIP_MERGE也算尖端）
    [[nodiscard]] static bool isTip(const BlockState* state, bool allowMerge);

    /// 判断是否为未合并的指定方向尖端
    [[nodiscard]] static bool isUnmergedTipWithDirection(const BlockState* state, Direction direction);

    /// 判断是否为钟乳石起点位置（朝下且上方不是滴石）
    [[nodiscard]] static bool isStalactiteStartPos(const BlockState& state, IWorld& world, const BlockPos& pos);

    /// 判断尖端是否可以滴水
    [[nodiscard]] static bool canDrip(const BlockState& state);

    /// 判断尖端是否可以生长
    [[nodiscard]] static bool canTipGrow(const BlockState& state, IWorld& world, const BlockPos& pos);

    /// 判断生长条件：上方是滴水石块且上方2格是水源
    [[nodiscard]] static bool canGrow(IWorld& world, const BlockPos& pos);

    /// 检查位置是否可以放置滴石（支撑检查）
    [[nodiscard]] static bool isValidPointedDripstonePlacement(IWorld& world, const BlockPos& pos, Direction direction);

    /// 计算放置时的尖端方向
    [[nodiscard]] static Direction calculateTipDirection(IWorld& world, const BlockPos& pos, Direction preferredDir);

    /// 计算滴石厚度
    [[nodiscard]] static BlockStateProperties::DripstoneThickness calculateDripstoneThickness(
        IWorld& world, const BlockPos& pos, Direction tipDirection, bool isTipMerge);

    /// 沿方向寻找尖端（返回空表示未找到）
    [[nodiscard]] static std::optional<BlockPos> findTip(
        const BlockState& state, IWorld& world, const BlockPos& pos, i32 maxDistance, bool allowMerge);

    /// 向下寻找炼药锅（返回空表示未找到）
    [[nodiscard]] static std::optional<BlockPos> findFillableCauldronBelow(
        IWorld& world, const BlockPos& tipPos, const fluid::Fluid& fluid);

    /// 从炼药锅向上搜索可滴水的钟乳石尖端
    [[nodiscard]] static std::optional<BlockPos> findStalactiteTipAboveCauldron(IWorld& world, const BlockPos& pos);

    /// 获取钟乳石可填充炼药锅的流体类型
    [[nodiscard]] static const fluid::Fluid* getCauldronFillFluidType(IWorld& world, const BlockPos& tipPos);

    /// 判断方块是否可被滴水穿透
    [[nodiscard]] static bool canDripThrough(IWorld& world, const BlockPos& pos, const BlockState* state);

    /// 沿钟乳石向上寻找根方块（返回空表示未找到）
    [[nodiscard]] static std::optional<BlockPos> findRootBlock(
        IWorld& world, const BlockPos& pos, const BlockState& state, i32 maxDistance);

    /// 在指定位置创建滴石方块
    static void createDripstone(
        IWorld& world, const BlockPos& pos, Direction direction, BlockStateProperties::DripstoneThickness thickness);

    /// 合并两个对向尖端
    static void createMergedTips(IWorld& world, const BlockPos& pos, const BlockState& upState);

    /// 单步生长
    static void grow(IWorld& world, const BlockPos& tipPos, Direction direction);

    /// 尝试在钟乳石尖端下方生长石笋
    static void growStalagmiteBelow(IWorld& world, const BlockPos& tipPos);

    /// 尝试生长钟乳石或石笋
    static void growStalactiteOrStalagmiteIfPossible(
        const BlockState& state, IWorld& world, const BlockPos& pos, math::IRandom& random);

    /// 流体传输逻辑
    static void maybeTransferFluid(const BlockState& state, IWorld& world, const BlockPos& pos, f32 chance);

    /**
     * @brief 获取钟乳石滴水粒子位置
     *
     * Y = blockPos.y + STALACTITE_DRIP_START_PIXEL - 0.0625 = blockPos.y + 0.25
     * X/Z = blockPos.x/z + 0.5（中心对齐）
     *
     * @param pos 钟乳石尖端方块位置
     * @return 粒子生成位置（世界坐标）
     */
    [[nodiscard]] static Vector3 getDripParticlePosition(const BlockPos& pos);

private:
    /// 碰撞形状，按厚度和方向索引
    std::unordered_map<BlockStateProperties::DripstoneThickness, CollisionShape> m_shapes;
    /// 朝下尖端的额外形状
    CollisionShape m_tipDownShape;

    // ========== 常量 ==========

    /// 搜索流体时最大搜索距离
    static constexpr i32 MAX_SEARCH_LENGTH_WHEN_CHECKING_DRIP_TYPE = 11;
    /// 钟乳石掉落延迟(tick)
    static constexpr i32 DELAY_BEFORE_FALLING = 2;
    /// 钟乳石尖端滴水起始 Y 偏移（像素），对应 SHAPE_TIP_DOWN.min(Y) = 5.0
    static constexpr f64 STALACTITE_DRIP_START_PIXEL = 5.0 / 16.0;
    /// 水传输概率
    static constexpr f32 WATER_TRANSFER_PROBABILITY_PER_RANDOM_TICK = 0.17578125F;
    /// 岩浆传输概率
    static constexpr f32 LAVA_TRANSFER_PROBABILITY_PER_RANDOM_TICK = 0.05859375F;
    /// 每随机刻生长概率
    static constexpr f32 GROWTH_PROBABILITY_PER_RANDOM_TICK = 0.011377778F;
    /// 最大生长长度
    static constexpr i32 MAX_GROWTH_LENGTH = 7;
    /// 向下搜索石笋生长位置的最大距离
    static constexpr i32 MAX_STALAGMITE_SEARCH_RANGE_WHEN_GROWING = 10;
    /// 尖端到炼药锅最大搜索距离
    static constexpr i32 MAX_SEARCH_LENGTH_BETWEEN_TIP_AND_CAULDRON = 11;
    /// 石笋摔落伤害距离偏移。对齐 Java 1.21.11 PointedDripstoneBlock#fallOn：
    /// causeFallDamage(fallDistance + 2.5, 2.0F, stalagmite())。此前误为 2.0（致测试期望 9.0 实得 8.0）。
    /// 伤害 = (fallDistance + 2.5 - 3.0) * 2.0。
    static constexpr f32 STALAGMITE_FALL_DISTANCE_OFFSET = 2.5F;
    /// 石笋摔落伤害倍率
    static constexpr i32 STALAGMITE_FALL_DAMAGE_MODIFIER = 2;
    /// 坠落钟乳石每格伤害系数
    static constexpr f32 FALLING_STALACTITE_FALL_DAMAGE_PER_DISTANCE = 1.0F;
    /// 坠落钟乳石最大伤害
    static constexpr i32 FALLING_STALACTITE_MAX_DAMAGE = 40;

    /// 钟乳石失去支撑时生成掉落方块实体
    static void _spawnFallingStalactite(IWorld& world, const BlockPos& pos, const BlockState& state);
};

} // namespace blocks
} // namespace mc
