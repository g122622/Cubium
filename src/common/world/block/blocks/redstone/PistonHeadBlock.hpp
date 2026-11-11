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
 * LIABILITY, WHETHER IN TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"

namespace mc {

namespace util {
template <typename T>
class EnumProperty;
}

namespace blocks {

/**
 * @brief 活塞头方块
 *
 * 活塞头是活塞伸出时显示的方块部分，与活塞主体（PistonBlock）关联。
 * 当活塞主体不存在或未伸出时，活塞头应自动消失。
 *
 * ## 特性
 * - 仅在活塞伸出时显示
 * - 与活塞主体关联（通过 FACING 和 TYPE 属性）
 * - 活塞主体不存在或未伸出时自动变为空气
 * - 被移除时级联销毁匹配的活塞主体
 */
class PistonHeadBlock : public Block {
public:
    /**
     * @brief 活塞头类型
     */
    enum class Type : u8 {
        Normal = 0, ///< 普通活塞头
        Sticky = 1  ///< 粘性活塞头
    };

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit PistonHeadBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston) override;

    /**
     * @brief 玩家即将破坏活塞头时调用
     *
     * 在创造模式下，破坏活塞头时应同时销毁匹配的活塞基座且不产生掉落物。
     * 在生存模式下，掉落物由 onBlockRemoved 中的一般逻辑处理。
     *
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态（破坏前的状态）
     * @param player 破坏方块的玩家
     */
    void playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player) override;

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Block;
    }

    // ========== 活塞头特有方法 ==========

    /**
     * @brief 获取活塞头朝向
     *
     * @param state 方块状态
     * @return Direction 朝向方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state) noexcept;

    /**
     * @brief 获取活塞头类型
     *
     * @param state 方块状态
     * @return Type 活塞头类型
     */
    [[nodiscard]] static Type getType(const BlockState& state) noexcept;

    /**
     * @brief 设置活塞头类型
     *
     * @param state 方块状态
     * @param type 类型
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withType(BlockState state, Type type) noexcept;

    /**
     * @brief 获取活塞头类型属性
     *
     * 用于 MovingPistonBlock 共享相同的属性。
     *
     * @return 类型属性的引用
     */
    [[nodiscard]] static const EnumProperty<Type>& getTypeProperty() noexcept;

    /**
     * @brief 检查给定的方块状态是否是匹配的已伸出活塞基座
     *
     * 当活塞头的 TYPE 为 Normal 时，检查对方是否为已伸出的 PISTON；
     * 当活塞头的 TYPE 为 Sticky 时，检查对方是否为已伸出的 STICKY_PISTON。
     * 同时检查 EXTENDED 属性为 true 且 FACING 方向一致。
     *
     * @param headState 活塞头状态
     * @param baseState 待检查的方块状态
     * @return true 如果是匹配的已伸出活塞基座
     */
    [[nodiscard]] static bool isFittingBase(const BlockState& headState,
        const BlockState&
            baseState) noexcept; /**
                                  * @brief 使用形状进行光照遮挡（对齐原版 `PistonHeadBlock` 的同名重写）
                                  *
                                  * 原版这个方块的重写返回 true，光照引擎据此按实际形状做面遮挡，而不是当作整格挡光。
                                  */

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 活塞头形状（对齐 vanilla `PistonHeadBlock#SHAPES`）
     *
     * vanilla：
     *   SHAPE_PLATFORM = boxZ(16.0, 0.0, 4.0)                       → 平台，贴活塞外侧 4 像素厚
     *   SHAPES_SHORT   = SHAPES.or(SHAPE_PLATFORM, boxZ(4.0, 4.0, 16.0))
     *   SHAPES         = SHAPES.or(SHAPE_PLATFORM, boxZ(4.0, 4.0, 20.0))
     * 再按 FACING 旋转。它不是整格，所以天空光可穿过（lightBlock=0）。早期未重写 getShape，
     * 退回整格立方体，活塞头被误判为完全挡光。
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;
    /// 寻路通行判定。
    [[nodiscard]] bool isPathfindable(const BlockState& state, PathComputationType type) const override;
};

} // namespace blocks
} // namespace mc
