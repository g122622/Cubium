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

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Entity;

namespace blocks {

/**
 * @brief 气泡柱方块
 *
 * 由岩浆块或灵魂沙产生的水下气泡柱。
 * 可以推动实体向上或向下。
 *
 * ## 状态属性
 * - DRAG: 是否为下拖
 *   - false: 向上推动（灵魂沙产生）
 *   - true: 向下拖拽（岩浆块产生）
 *
 * ## 推动机制
 * - 灵魂沙: 产生上升气泡柱 (DRAG=false)，推动速度 +0.1
 * - 岩浆块: 产生下降气泡柱 (DRAG=true)，拖拽速度 -0.03
 *
 * ## 延伸机制
 * - 气泡柱会向上延伸直到水面或空气
 * - tick 方法处理向上传播
 */
class BubbleColumnBlock : public Block {
public:
    explicit BubbleColumnBlock(const BlockProperties& properties);
    ~BubbleColumnBlock() override = default;

    // ========== 静态方法 ==========

    /**
     * @brief 放置气泡柱方块
     *
     * 检查目标位置是否可以放置气泡柱，如果可以则设置气泡柱方块。
     *
     * @param world 世界引用
     * @param pos 目标位置
     * @param drag 是否为下拖模式（true=岩浆块产生，false=灵魂沙产生）
     */
    static void placeBubbleColumn(IWorld& world, const BlockPos& pos, bool drag);

    /**
     * @brief 检查位置是否可以放置气泡柱
     *
     * 条件：是水方块 + 是水源方块
     *
     * @param world 世界引用
     * @param pos 检查位置
     * @return 如果可以放置气泡柱返回 true
     */
    static bool canHoldBubbleColumn(const IWorld& world, const BlockPos& pos);

    /**
     * @brief 获取气泡柱的 DRAG 状态
     *
     * 根据下方方块类型决定 DRAG 状态：
     * - 下方是气泡柱：继承其 DRAG 状态
     * - 下方是灵魂沙：返回 false（上推）
     * - 其他（包括岩浆块）：返回 true（下拖）
     *
     * @param world 世界引用
     * @param pos 当前位置（下方方块的位置）
     * @return DRAG 状态
     */
    static bool getDrag(const IWorld& world, const BlockPos& pos);

    // ========== 状态属性 ==========

    /**
     * @brief 检查是否为下拖模式
     * @param state 方块状态
     * @return true 下拖（岩浆块产生），false 上推（灵魂沙产生）
     */
    [[nodiscard]] bool isDrag(const BlockState& state) const;

    // ========== 放置逻辑 ==========

    /**
     * @brief 方块被添加到世界时
     *
     * 气泡柱被添加时，在上方放置气泡柱
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 实体交互 ==========

    /**
     * @brief 实体碰撞时推动实体
     *
     * 逻辑：
     * - DRAG=false: 向上推动 +0.1 Y 速度
     * - DRAG=true: 向下拖拽 -0.03 Y 速度
     * - 重置摔落距离
     *
     * @param state 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param entity 碰撞的实体
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    /**
     * @brief 方块 tick，处理气泡柱向上延伸
     *
     * 逻辑：
     * - 检查上方是否为水源方块
     * - 如果是水，将其转换为气泡柱并继承 DRAG 状态
     * - 如果上方已是气泡柱，更新其 DRAG 状态
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @param random 随机数生成器
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 客户端动画 ==========

    /**
     * @brief 客户端方块动画 tick
     *
     * 在气泡柱中生成粒子和播放环境音效：
     * - 下拖模式 (DRAG=true): 生成 CURRENT_DOWN 粒子，1/200 概率播放漩涡环境音
     * - 上推模式 (DRAG=false): 生成 BUBBLE_COLUMN_UP 粒子，1/200 概率播放上升环境音
     */
    void animateTick(IBlockAnimateContext& context,
        const BlockPos& pos,
        const BlockState& state,
        math::IRandom& random) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

private:
    /**
     * @brief 检查下方是否产生气泡
     *
     * 检查下方方块类型来确定 DRAG 状态：
     * - 岩浆块: return true (下拖)
     * - 灵魂沙: return false (上推)
     * - 气泡柱: 继承其 DRAG 状态
     *
     * @param world 世界引用
     * @param pos 当前位置
     * @return bool DRAG 状态
     */
    [[nodiscard]] bool _checkSource(const IWorld& world, const BlockPos& pos) const;
};

} // namespace blocks
} // namespace mc
