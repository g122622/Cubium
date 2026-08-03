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
#include "common/world/block/Block.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;

namespace blocks {

/**
 * @brief 潮涌核心
 *
 * 水下的信标类方块，需要潮涌框架激活。
 * 激活后提供潮涌能量效果（水下呼吸、挖掘加速）。
 *
 * 状态属性：
 * - WATERLOGGED: 是否含水
 *
 * MC ID: minecraft:conduit
 *
 * 激活条件：
 * - 中心周围3x3x3必须全部是水
 * - 需要海晶石框架（16-42个方块）
 * - 42个以上框架方块时可以攻击敌对生物
 */
class ConduitBlock : public Block {
public:
    /**
     * @brief 构造潮涌核心
     */
    explicit ConduitBlock(BlockProperties properties);

    // ========== 状态属性 ==========

    /**
     * @brief 检查是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const;

    /**
     * @brief 创建方块实体
     * 潮涌核心需要方块实体来管理效果
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    // ========== 放置逻辑 ==========

    /**
     * @brief 获取放置状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 方块被添加时
     * 创建方块实体并检测周围框架
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 方块被移除时
     * 清除方块实体和效果
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居更新
     * 检测框架变化
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 是否透明
     */
    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 获取光照等级
     *
     * 潮涌核心始终发出15级光照。
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 光照等级 (15)
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return 15;
    }

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;
};

} // namespace blocks
} // namespace mc
