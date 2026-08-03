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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <array>
#include <memory>

namespace mc {

class World;
class BlockItemUseContext;
class HopperEntity;

namespace blocks {

/**
 * @brief 漏斗方块
 *
 * 方块状态属性：
 * - FACING: 输出方向（下/北/南/东/西，不能向上）
 * - ENABLED: 是否启用（红石控制）
 *
 * 功能：
 * - 自动从上方容器或物品实体拉取物品
 * - 向下方容器输出物品
 * - 红石信号可禁用
 * - 红石比较器可检测填充度
 * - 与物品实体碰撞时自动收集
 */
class HopperBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit HopperBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~HopperBlock() override = default;

    // ========== 放置和更新 ==========

    /**
     * @brief 获取放置时的方块状态
     * @param context 放置上下文
     * @return 方块状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 方块被添加到世界时调用
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居方块更新
     * @param world 世界
     * @param pos 当前位置
     * @param neighborBlock 邻居方块
     * @param neighborPos 邻居位置
     * @param isMoving 是否正在移动
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    // ========== 形状 ==========

    /**
     * @brief 获取碰撞形状
     * @param state 方块状态
     * @return 碰撞形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取射线追踪形状
     * @param state 方块状态
     * @return 射线追踪形状
     */
    [[nodiscard]] const CollisionShape& getRaytraceShape(const BlockState& state) const;

    // ========== 方块实体 ==========

    /**
     * @brief 检查是否有方块实体
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    /**
     * @brief 创建方块实体
     * @param pos 方块位置
     * @return 方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    /**
     * @brief 获取方块实体类型
     * @return 方块实体类型
     */
    [[nodiscard]] BlockEntityType getBlockEntityType() const { return BlockEntityType::Hopper; }

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

    // ========== 红石 ==========

    /**
     * @brief 检查是否可以提供红石信号
     */
    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override { return false; }

    /**
     * @brief 检查是否有红石比较器输入覆盖
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override { return true; }

    /**
     * @brief 获取红石比较器信号
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 实体碰撞 ==========

    /**
     * @brief 实体碰撞时调用
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param entity 碰撞的实体
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

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

    // ========== 静态工具方法 ==========

    /**
     * @brief 获取漏斗输出方向
     * @param state 方块状态
     * @return 输出方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 检查漏斗是否启用
     * @param state 方块状态
     * @return 如果启用返回true
     */
    [[nodiscard]] static bool isEnabled(const BlockState& state);

private:
    /**
     * @brief 更新漏斗启用状态（响应红石信号）
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    void _updateState(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 初始化形状
     */
    void _initShapes();

    // 形状缓存（按方向索引）
    // 0: DOWN, 1: UP (unused), 2: NORTH, 3: SOUTH, 4: WEST, 5: EAST
    std::array<CollisionShape, 6> m_shapes;
    std::array<CollisionShape, 6> m_raytraceShapes;
};

} // namespace blocks
} // namespace mc
