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
 * @brief 幽匿尖啸体
 *
 * 被激活多次后可召唤监守者。状态属性：SHRIEKING, CAN_SUMMON, WATERLOGGED
 *
 * 激活流程：
 * 1. 实体踩上方块或接收到 SHRIEK 振动事件时触发 tryShriek()
 * 2. tryShriek() 检查 CAN_SUMMON、难度、游戏规则、附近监守者、玩家冷却等条件
 * 3. 条件通过后调用 shriek() 设置 SHRIEKING=true、播放声音、发射 SHRIEK 事件
 * 4. 90 tick 后 tick() 将 SHRIEKING 设回 false 并调用 tryRespond()
 * 5. tryRespond() 播放警告声音、应用黑暗效果、在警告等级 >= 4 时尝试召唤监守者
 *
 * 服务端逻辑（tryShriek, tryRespond, _trySummonWarden 等）位于
 * SculkShriekerHelper（server/world/blockentity/sculk/），因为它们依赖 ServerWorld。
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

    // 与 vanilla 一致：碰撞形状与遮挡形状都是 8 像素高的台子，但 getShape 仍为整格。
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getOcclusionShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 天空光直通判定（对齐 vanilla 默认公式，即按 getShape 求值）
     *
     * Cubium 的默认公式基于 getOcclusionShape，而这个方块的遮挡形状是 8 像素台子、
     * getShape 才是整格。原版拿到整格形状后判「不透传」，默认公式会给出相反的结果，
     * 因此这里按原版语义直接按 getShape 求值。
     */
    [[nodiscard]] bool propagatesSkylightDown(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

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
    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;
    [[nodiscard]] BlockEntityType getBlockEntityType() const;

    /**
     * @brief 实体踩上方块时触发
     *
     * 非潜行实体踩上时发出 SHRIEK 游戏事件，触发附近的幽匿尖啸体。
     */
    void onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    /**
     * @brief 方块 tick 回调
     *
     * 处理 SHRIEKING→!SHRIEKING 的状态转换，并通知服务端执行响应逻辑。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 方块移除时的清理
     *
     * 如果方块正在 SHRIEKING 状态时被移除，仍需执行 tryRespond()
     * 以确保警告效果（黑暗效果、监守者召唤检查）被触发。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston) override;

    // ========== 静态方法 ==========

    /**
     * @brief 播放尖啸声音和粒子效果，设置 SHRIEKING 状态
     *
     * 此方法仅处理视觉效果和状态转换，不涉及警告等级或召唤逻辑。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param sourceEntity 触发实体（用于游戏事件上下文，可为nullptr）
     */
    static void shriek(IWorld& world, const BlockPos& pos, const BlockState& state, const Entity* sourceEntity);

    /// SHRIEKING 状态持续tick数（90 ticks = 4.5秒）
    static constexpr i32 SHRIEKING_TICKS = 90;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
