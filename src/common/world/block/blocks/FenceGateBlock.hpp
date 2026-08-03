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
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include <array>
#include <memory>

namespace mc {

class World;
class BlockItemUseContext;
class Player;
class BlockRaycastResult;

namespace blocks {

/**
 * @brief 栅栏门方块
 *
 * 可开关的栅栏门，连接栅栏和墙。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 水平朝向 (NORTH, SOUTH, EAST, WEST)
 * - OPEN: 是否打开
 * - IN_WALL: 是否在墙内（改变碰撞箱）
 * - POWERED: 是否被充能
 */
class FenceGateBlock : public Block {
public:
    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit FenceGateBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~FenceGateBlock() override = default;

    // ========== 放置和更新 ==========

    /**
     * @brief 获取放置时的方块状态
     * @param context 放置上下文
     * @return 方块状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

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

    /**
     * @brief 获取遮挡形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] const CollisionShape& getOcclusionShape(const BlockState& state) const override;

    /**
     * @brief 是否使用形状进行光照遮挡检测
     *
     * 栅栏门有复杂的形状，需要精确的形状遮挡检测。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

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

    // ========== 红石 ==========

    /**
     * @brief 检查是否可以提供红石信号
     */
    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 静态工具方法 ==========

    /**
     * @brief 检查栅栏门是否打开
     * @param state 方块状态
     * @return 如果打开返回true
     */
    [[nodiscard]] static bool isOpen(const BlockState& state);

private:
    /**
     * @brief 检查是否在墙内
     * @param world 世界
     * @param pos 方块位置
     * @param facing 栅栏门朝向
     * @return 如果在墙内返回true
     */
    [[nodiscard]] bool _isWall(const IWorld& world, const BlockPos& pos, Direction facing) const;

    /**
     * @brief 播放开关门音效
     * @param world 世界
     * @param pos 方块位置
     * @param isOpening 是否正在打开
     */
    void _playSound(IWorld& world, const BlockPos& pos, bool isOpening);

    /// 关闭状态碰撞形状（2像素厚）
    CollisionShape m_closedShape;

    /// 打开状态形状（无碰撞）
    CollisionShape m_openShape;

    /// 墙内关闭状态碰撞形状（稍低）
    CollisionShape m_inWallClosedShape;

    /// 关闭状态各朝向形状缓存
    std::array<CollisionShape, 6> m_closedShapes;

    /// 打开状态各朝向形状缓存
    std::array<CollisionShape, 6> m_openShapes;

    /// 墙内关闭状态各朝向形状缓存
    std::array<CollisionShape, 6> m_inWallClosedShapes;
};

} // namespace blocks
} // namespace mc
