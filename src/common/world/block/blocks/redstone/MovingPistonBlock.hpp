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

#include "PistonHeadBlock.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/Material.hpp"
#include <memory>

namespace mc {

class BlockState;

namespace blocks {

/**
 * @brief 移动中的活塞方块
 *
 * 当活塞正在伸出或收回时，被移动的方块位置会暂时显示为 MovingPistonBlock。
 * 这个方块会创建一个 PistonBlockEntity 来处理动画和实体推动。
 *
 * ## 特性
 * - 创建 PistonBlockEntity 管理移动动画
 * - 碰撞箱由 PistonBlockEntity 决定
 * - 移动完成后转换为最终方块
 * - 与 PistonHeadBlock 共享 FACING 和 TYPE 属性
 *
 * ## 容易踩的坑
 * - 不要在此方块上创建常规逻辑，只是动画代理
 * - 碰撞箱是动态的，由 PistonBlockEntity 计算
 * - 方块移除时必须清理 PistonBlockEntity
 *
 * 参考: net.minecraft.block.MovingPistonBlock
 */
class MovingPistonBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit MovingPistonBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    /**
     * @brief 静态形状恒为空
     *
     * 对齐原版 `MovingPistonBlock#getShape`：
     *
     * ```java
     * protected VoxelShape getShape(BlockState state, BlockGetter level, BlockPos pos, CollisionContext ctx) {
     *     return Shapes.empty();
     * }
     * ```
     *
     * 移动中的活塞本体没有任何稳定的静态形状——它渲染什么完全取决于相邻方块实体
     * 记录的运动进度。缺失此覆写时会退化成完整方块，导致 `isFaceSturdy` 的全部六个面
     * 都误判为坚固（差分能力 face_sturdy 查出 18 处差异）。
     *
     * 注意：原版的**碰撞**形状来自 `PistonMovingBlockEntity#getCollisionShape`，与静态形状
     * 不是一回事。
     *
     * TODO: Cubium 目前尚未实现 `MovingPistonBlockEntity` 的碰撞形状转发，因此
     * `getCollisionShape` 仍需单独对齐；本覆写只修正静态 `getShape`。
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 方块被移除时调用
     *
     * 清理 PistonBlockEntity。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston) override;

    /**
     * @brief 获取推动反应
     *
     * 移动中的活塞不能被推动。
     */
    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Block;
    }

    /**
     * @brief 检查是否有方块实体
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

    /**
     * @brief 创建方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== MovingPistonBlock 特有方法 ==========

    /**
     * @brief 获取活塞朝向
     * @param state 方块状态
     * @return Direction 朝向方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 获取活塞类型
     * @param state 方块状态
     * @return Type 活塞类型（普通/粘性）
     */
    [[nodiscard]] static PistonHeadBlock::Type getType(const BlockState& state);

    /**
     * @brief 设置活塞类型
     * @param state 方块状态
     * @param type 类型
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withType(BlockState state, PistonHeadBlock::Type type);

private:
    /// 活塞头类型属性（与 PistonHeadBlock 共享）
    [[nodiscard]] static const EnumProperty<PistonHeadBlock::Type>& _getTypeProperty();
};

} // namespace blocks
} // namespace mc
