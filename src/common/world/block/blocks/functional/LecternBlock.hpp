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

#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <array>
#include <memory>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class BlockRaycastResult;
namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 讲台方块
 *
 * 用于放置和阅读书籍的方块。
 * 可以放置书和笔、成书，支持翻页。
 * 有书时可以发出红石信号。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向 (NORTH, SOUTH, EAST, WEST)
 * - POWERED: 是否发出红石信号
 * - HAS_BOOK: 是否有书
 */
class LecternBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit LecternBlock(const BlockProperties& properties);
    ~LecternBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== Tick ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 红石 ==========

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 交互 ==========

    /**
     * @brief 处理玩家右键交互
     *
     * 有书时打开GUI，无书时尝试放置书。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 方块移除时回调
     *
     * 掉落讲台上的书本。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 工具方法 ==========

    /**
     * @brief 尝试放置书本到讲台
     * @param world 世界引用
     * @param pos 方块位置
     * @param bookStack 要放置的书本物品（将复制一份到讲台）
     * @return 放置成功返回true
     */
    static bool tryPlaceBook(IWorld& world, const BlockPos& pos, const ItemStack& bookStack);

    /**
     * @brief 设置有书状态
     */
    static void setHasBook(IWorld& world, const BlockPos& pos, bool hasBook);

    /**
     * @brief 发出红石脉冲
     *
     * 翻页时调用，将 POWERED 设为 true，通知下方方块红石更新，
     * 并调度 2 tick 后的 tick 事件将 POWERED 设回 false。
     */
    static void pulse(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 切换供电状态并通知下方方块更新红石信号
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param powered 是否供电
     */
    static void changePowered(IWorld& world, const BlockPos& pos, const BlockState& state, bool powered);

    /**
     * @brief 通知讲台下方方块更新红石信号
     *
     * 讲台向下输出强信号，所有方向输出弱信号，
     * 因此红石更新只需通知正下方位置即可。
     *
     * @param world 世界引用
     * @param pos 讲台位置
     * @param block 讲台方块引用
     */
    static void updateBelow(IWorld& world, const BlockPos& pos, Block& block);

protected:
    /// 各朝向的形状缓存
    std::array<CollisionShape, 6> m_shapesByFacing;

    /// 碰撞形状
    CollisionShape m_collisionShape;

private:
    /**
     * @brief 掉落书本
     */
    void _dropBook(IWorld& world, const BlockPos& pos, const BlockState& state);
};

} // namespace blocks
} // namespace mc
