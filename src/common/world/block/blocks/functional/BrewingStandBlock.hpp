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

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 酿造台方块
 *
 * 用于酿造药水的功能方块。
 * 具有三个药水瓶槽位，每个槽位都有独立的"是否有瓶子"状态。
 *
 * 状态属性：
 * - HAS_BOTTLE_0: 第一个槽位是否有瓶子
 * - HAS_BOTTLE_1: 第二个槽位是否有瓶子
 * - HAS_BOTTLE_2: 第三个槽位是否有瓶子
 */
class BrewingStandBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BrewingStandBlock(const BlockProperties& properties);
    ~BrewingStandBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 光照 ==========

    /**
     * @brief 获取光照等级
     *
     * 酿造台始终发出1级光照。
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 光照等级 (1)
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return 1;
    }

    // ========== 红石 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] int getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 方块实体 ==========

    /**
     * @brief 检查是否有方块实体
     * @return true（酿造台有方块实体）
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    /**
     * @brief 创建酿造台方块实体
     * @param pos 方块位置
     * @return 新创建的BrewingStandEntity
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 交互 ==========

    /**
     * @brief 处理玩家右键交互
     *
     * 打开酿造台GUI。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 方块放置后回调
     *
     * 设置方块实体（酿造台方块实体在createBlockEntity中创建）。
     * 当放置物品携带自定义名称时（铁砧重命名后），将该名称传递给方块实体。
     *
     * 对应 MC Java 的 BaseContainerBlockEntity.applyImplicitComponents 机制：
     * 物品通过 setPlacedBy 传递给方块实体，自定义名称通过 Component 引用拷贝。
     */
    void onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack) override;

    /**
     * @brief 方块移除时回调
     *
     * 掉落酿造台内的物品。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 寻路 ==========

    /**
     * @brief 检查是否允许路径寻找
     * @return false（酿造台阻挡寻路）
     */
    [[nodiscard]] bool allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return false;
    }

protected:
    /// 酿造台形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
