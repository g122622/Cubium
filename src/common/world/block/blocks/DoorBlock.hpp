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

#include "../../../util/assert/AssertAll.hpp"
#include "../../../util/property/Properties.hpp"
#include "../Block.hpp"
#include "../BlockPos.hpp"
#include "../Material.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockState.hpp"
#include <array>
#include <cstddef>
#include <memory>

namespace mc {

class World;
class BlockItemUseContext;
class Player;
class BlockRaycastResult;

namespace blocks {

/**
 * @brief 门方块
 *
 * 双方块结构（上半部分和下半部分），可被玩家或红石控制开关。
 *
 * 状态属性：
 * - FACING: 水平朝向 (NORTH, SOUTH, EAST, WEST)
 * - OPEN: 是否打开
 * - HINGE: 铰链位置 (LEFT, RIGHT)
 * - POWERED: 是否被充能
 * - HALF: 方块半部分 (UPPER, LOWER)
 *
 * 木门可以手动开关，铁门只能通过红石控制。
 *
 * 参考: net.minecraft.block.DoorBlock
 */
class DoorBlock : public Block {
public:
    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param isIron 是否为铁门（只能红石控制）
     */
    explicit DoorBlock(const BlockProperties& properties, bool isIron = false);

    /**
     * @brief 析构函数
     */
    ~DoorBlock() override = default;

    // ========== 放置和更新 ==========

    /**
     * @brief 获取放置时的方块状态
     * @param context 放置上下文
     * @return 方块状态，如果无法放置返回nullptr
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 方块放置后的处理
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @param stack 放置该方块的物品堆（可能携带自定义名称等组件）
     */
    void onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack) override;

    /**
     * @brief 邻居方块更新
     * @param world 世界
     * @param pos 当前方块位置
     * @param neighborBlock 邻居方块
     * @param neighborPos 邻居位置
     * @param isMoving 是否正在移动
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    /**
     * @brief 方块更新后处理
     * @param state 当前方块状态
     * @param facing 更新的方向
     * @param facingState 邻居状态
     * @param world 世界
     * @param currentPos 当前方块位置
     * @param facingPos 邻居位置
     * @return 更新后的状态
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 检查是否可以放置
     * @param state 方块状态
     * @param world 世界读取器
     * @param pos 方块位置
     * @return 如果可以放置返回true
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 交互 ==========

    /**
     * @brief 玩家右键点击
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     * @param hand 手
     * @param hit 射线检测结果
     * @return 交互结果
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 切换门的开关状态
     * @param world 世界
     * @param state 方块状态
     * @param pos 方块位置
     * @param open 是否打开
     */
    void toggleDoor(IWorld& world, const BlockPos& pos, bool open);

    // ========== 形状 ==========

    /**
     * @brief 获取渲染形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 旋转和镜像 ==========

    /**
     * @brief 旋转方块状态
     * @param state 原状态
     * @param rotation 旋转
     * @return 旋转后的状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块状态
     * @param state 原状态
     * @param mirror 镜像
     * @return 镜像后的状态
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 推动反应 ==========

    /**
     * @brief 获取推动反应
     * @param state 方块状态
     * @return 推动反应类型
     */
    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Destroy;
    }

    // ========== 静态工具方法 ==========

    /**
     * @brief 检查门是否打开
     * @param state 方块状态
     * @return 如果打开返回true
     */
    [[nodiscard]] static bool isOpen(const BlockState& state) noexcept;

    /**
     * @brief 检查是否为铁门
     * @return 如果是铁门返回true
     */
    [[nodiscard]] bool isIronDoor() const noexcept { return m_isIron; }

    /**
     * @brief 检查是否为木门
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @return 如果是木门返回true
     */
    [[nodiscard]] static bool isWooden(const BlockState& state) noexcept;

private:
    /**
     * @brief 计算铰链位置
     * @param context 放置上下文
     * @return 铰链位置
     */
    [[nodiscard]] BlockStateProperties::DoorHinge _calculateHingeSide(BlockItemUseContext& context);

    /**
     * @brief 播放开关门音效
     * @param world 世界
     * @param pos 方块位置
     * @param isOpening 是否正在打开
     */
    void _playSound(IWorld& world, const BlockPos& pos, bool isOpening);

    /**
     * @brief 获取开门音效ID
     * @return 音效ID
     */
    [[nodiscard]] i32 _getOpenSound() const;

    /**
     * @brief 获取关门音效ID
     * @return 音效ID
     */
    [[nodiscard]] i32 _getCloseSound() const;

    /**
     * @brief 根据状态获取形状
     * @param facing 朝向
     * @param open 是否打开
     * @param hingeRight 铰链是否在右边
     * @return 形状索引
     */
    [[nodiscard]] static size_t _getShapeIndex(Direction facing, bool open, bool hingeRight) noexcept;

    /// 是否为铁门
    bool m_isIron;

    /// 方块形状（根据朝向和状态缓存）
    std::array<CollisionShape, 8> m_shapes;
};

} // namespace blocks
} // namespace mc
