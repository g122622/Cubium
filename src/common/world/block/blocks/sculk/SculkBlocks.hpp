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
 *
 */

#pragma once

#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../IWaterLoggable.hpp"

namespace mc {

class Entity;
class BlockEntity;
enum class BlockEntityType : u16;

namespace blocks {

/**
 * @brief 幽匿块
 *
 * 深暗之域的主要构成方块，可通过幽匿催化体蔓延。
 * 当被精确采集时掉落经验。
 *
 * 参考: net.minecraft.block.SculkBlock
 */
class SculkBlock : public Block {
public:
    explicit SculkBlock(const BlockProperties& properties)
        : Block(properties)
    {}

    ~SculkBlock() override = default;
};

/**
 * @brief 幽匿脉络
 *
 * 可附着在方块表面的幽匿蔓延物，可被骨粉催生。
 *
 * 参考: net.minecraft.block.SculkVeinBlock
 */
class SculkVeinBlock : public Block {
public:
    explicit SculkVeinBlock(const BlockProperties& properties)
        : Block(properties)
    {}

    ~SculkVeinBlock() override = default;
};

/**
 * @brief 幽匿感测体
 *
 * 检测振动并输出红石信号的方块。
 * 状态属性：SCULK_SENSOR_PHASE, POWER, WATERLOGGED
 *
 * 参考: net.minecraft.block.SculkSensorBlock
 */
class SculkSensorBlock : public Block, public IWaterLoggable {
public:
    explicit SculkSensorBlock(const BlockProperties& properties);

    ~SculkSensorBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override { return true; }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    /**
     * @brief 方块 tick 回调
     *
     * 处理 Active→Cooldown 和 Cooldown→Inactive 的状态转换。
     * 由 scheduleBlockTick 调度触发。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 方块移除时通知邻居红石信号变化
     *
     * 如果移除时处于 Active 状态，需要通知邻居更新红石信号，
     * 否则邻居可能仍认为有信号输入。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /// 方块实体支持
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;
    [[nodiscard]] BlockEntityType getBlockEntityType() const;

    /// 活跃阶段持续tick数
    static constexpr i32 ACTIVE_TICKS = 30;
    /// 冷却阶段持续tick数
    static constexpr i32 COOLDOWN_TICKS = 10;

    // ========== 激活/失活/状态查询静态方法 ==========

    /**
     * @brief 检查幽匿感测体是否可以被激活
     *
     * 只有当前 Phase 为 Inactive 时才能被激活。
     * 对齐 MC Java: SculkSensorBlock.canActivate()
     */
    [[nodiscard]] static bool canActivate(const BlockState& state);

    /**
     * @brief 激活幽匿感测体
     *
     * 设置方块状态为 Active，设置红石信号强度，调度 tick，通知邻居红石更新，
     * 触发共振事件，发出 SCULK_SENSOR_TENDRILS_CLICKING 游戏事件和声音。
     *
     * @param sourceEntity 触发振动的源实体（可为nullptr）
     * @param world 世界引用
     * @pos 方块位置
     * @param state 当前方块状态
     * @param redstoneStrength 红石信号强度 (1-15)，基于振动距离计算
     * @param frequency 振动频率 (1-15)，用于共振和比较器输出
     */
    static void activate(const Entity* sourceEntity,
        IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        i32 redstoneStrength,
        i32 frequency);

    /**
     * @brief 停用幽匿感测体（Active→Cooldown）
     *
     * 设置方块状态为 Cooldown，红石信号归零，调度 tick，通知邻居红石更新。
     * 对齐 MC Java: SculkSensorBlock.deactivate()
     */
    static void deactivate(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 获取当前相位
     */
    [[nodiscard]] static BlockStateProperties::SculkSensorPhase getPhase(const BlockState& state);

    /**
     * @brief 获取活跃阶段的tick数（子类可覆盖）
     *
     * 普通感测体为30tick，校准感测体为10tick。
     */
    [[nodiscard]] virtual i32 getActiveTicks() const { return ACTIVE_TICKS; }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_shape;
};

/**
 * @brief 校准幽匿感测体
 *
 * 可通过红石信号过滤振动频率的高级感测体。
 * 状态属性：FACING, SCULK_SENSOR_PHASE, POWER, WATERLOGGED
 *
 * 参考: net.minecraft.block.CalibratedSculkSensorBlock
 */
class CalibratedSculkSensorBlock : public SculkSensorBlock {
public:
    explicit CalibratedSculkSensorBlock(const BlockProperties& properties);

    ~CalibratedSculkSensorBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 校准幽匿感测体仅在非输入面方向输出红石信号
     *
     * FACING 方向是输入面（从该方向读取红石信号频率过滤），
     * 红石信号只在非 FACING 方向输出。
     */
    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    /// 校准感测体活跃阶段更短（10 tick）
    static constexpr i32 CALIBRATED_ACTIVE_TICKS = 10;
    [[nodiscard]] i32 getActiveTicks() const override { return CALIBRATED_ACTIVE_TICKS; }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

/**
 * @brief 幽匿催化体
 *
 * 生物在此附近死亡时生成幽匿块。
 * 状态属性：BLOOM
 *
 * 参考: net.minecraft.block.SculkCatalystBlock
 */
class SculkCatalystBlock : public Block {
public:
    explicit SculkCatalystBlock(const BlockProperties& properties);

    ~SculkCatalystBlock() override = default;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_shape;
};

/**
 * @brief 幽匿尖啸体
 *
 * 被激活多次后可召唤监守者。
 * 状态属性：SHRIEKING, CAN_SUMMON, WATERLOGGED
 *
 * 参考: net.minecraft.block.SculkShriekerBlock
 */
class SculkShriekerBlock : public Block, public IWaterLoggable {
public:
    explicit SculkShriekerBlock(const BlockProperties& properties);

    ~SculkShriekerBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

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

    /// 方块实体支持
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;
    [[nodiscard]] BlockEntityType getBlockEntityType() const;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
