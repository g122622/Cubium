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
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/IWaterLoggable.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 光源方块
 *
 * 不可见的、可调亮度的光源方块，仅创造模式管理员可用（GameMasterBlock）。
 * 右键点击循环切换亮度等级（0-15），持有光源方块物品时显示完整碰撞轮廓。
 *
 * 状态属性：
 * - LEVEL: 光照等级 (0-15)，默认 15
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.world.level.block.LightBlock
 */
class LightBlock : public Block, public IWaterLoggable {
public:
    /// 最大光照等级
    static constexpr i32 MAX_LEVEL = 15;

    explicit LightBlock(const BlockProperties& properties);
    ~LightBlock() override = default;

    // ========== 标记接口 ==========

    [[nodiscard]] bool isGameMaster() const noexcept override { return true; }

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return false; }

    // ========== 光照 ==========

    /**
     * @brief 动态光照等级：返回 LEVEL 属性的值
     *
     * 对齐 vanilla LightBlock.LIGHT_EMISSION = state -> state.getValue(LEVEL)。
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    // ========== 形状 ==========

    /**
     * @brief 获取渲染形状
     *
     * 对齐 vanilla：持有光源方块物品时返回完整方块形状，否则返回空形状。
     * 注：本项目 Block::getShape 无实体上下文，无法查询玩家手持物品，
     * 故此处返回空形状（不可见），轮廓显示留待渲染层按持有物品判定。
     * TODO: 渲染层实现"持有 minecraft:light 时显示完整碰撞轮廓"（对齐 vanilla
     *   LightBlock#getShape 的 CollisionContext.isHoldingItem(Items.LIGHT) 分支）。
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 交互 ==========

    /**
     * @brief 右键循环切换光照等级
     *
     * 对齐 vanilla LightBlock#useWithoutItem：服务端且玩家具备管理员权限时
     * cycle(LEVEL) 并更新方块，返回 SUCCESS；否则返回 CONSUME。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 含水 ==========

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /**
     * @brief 透传天空光：对齐 vanilla LightBlock#propagatesSkylightDown
     *
     * vanilla: `return state.getFluidState().isEmpty()`。光源方块无水时透传天空光。
     */
    [[nodiscard]] bool propagatesSkylightDown(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    /**
     * @brief 邻居方块更新：含水时调度水流体刻
     *
     * 对齐 vanilla LightBlock#updateShape 中 `if (WATERLOGGED) scheduleTick(WATER)` 分支。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;
};

} // namespace blocks
} // namespace mc
