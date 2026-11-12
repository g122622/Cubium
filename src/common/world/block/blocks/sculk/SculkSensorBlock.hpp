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

#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/capability/IWaterLoggable.hpp"
#include "common/world/block/core/Block.hpp"

#include <memory>

namespace mc {

class Entity;
class BlockEntity;
enum class BlockEntityType : u16;

namespace blocks {

/**
 * @brief 幽匿感测体
 *
 * 检测振动并输出红石信号的方块。
 * 状态属性：SCULK_SENSOR_PHASE, POWER, WATERLOGGED
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

    /**
     * @brief 幽匿感测体固有发光：原版 getLightEmission 恒为 1
     *
     * 原版 SculkSensorBlock 在构造里 .lightLevel(1)，与相位无关（相位只决定红石输出）。
     * 这里覆盖 getLightLevel 而不是靠注册值，是为了让默认状态与派生状态都拿到 1。
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    // 注意：这两个必须成对出现。只实现 getComparatorInputOverride 而没有
    // hasComparatorInputOverride 时，原版与本项目的比较器都会先问「这格能不能读出信号」，
    // 得到默认的 false 就直接跳过——实现写的读数永远不会被调用。
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

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
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston) override;

    /**
     * @brief 实体踩踏（原版 `SculkSensorBlock#stepOn`）
     *
     * 感测体不只靠监听周围的事件，自己被踩也会产生一次 STEP 振动。
     * 判定条件：服务端、相位为 Inactive、踩踏者不是监守者
     * （监守者与感测体同源，不该自激）。
     * 满足时直接把 STEP 振动塞进选择器，跳过「找最近的振动」这一步。
     */
    void onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

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
    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

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
    [[nodiscard]] virtual i32 getActiveTicks() const
    {
        return ACTIVE_TICKS;
    }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_shape;
    /// 寻路通行判定：形状不完整不等于可以穿过。
    [[nodiscard]] bool isPathfindable(const BlockState& state, PathComputationType type) const override;
};

} // namespace blocks
} // namespace mc
