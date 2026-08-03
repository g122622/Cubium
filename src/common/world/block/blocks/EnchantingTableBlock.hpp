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
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <memory>

namespace mc {

class World;
class BlockItemUseContext;
class Player;
class BlockRaycastResult;
class BlockEntity;

namespace blocks {

/**
 * @brief 附魔台方块
 *
 * 提供附魔功能的方块。周围的书架可以增加附魔力量。
 *
 * 附魔力量计算：
 * - 有效书架：距离附魔台水平2格，垂直0-1格
 * - 书架与附魔台之间必须是可替换方块（空气、草等）
 * - 每个有效书架增加1点附魔力量（最大15）
 * - 书架判断使用ENCHANTMENT_POWER_PROVIDER标签，中间方块判断使用canBeReplaced()
 *
 * 当附魔台放置或周围方块变化时，会重新计算附魔力量。
 */
class EnchantingTableBlock : public Block {
public:
    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit EnchantingTableBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~EnchantingTableBlock() override = default;

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
     */
    [[nodiscard]] BlockEntityType getBlockEntityType() const { return BlockEntityType::EnchantingTable; }

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
     * @brief 获取遮挡形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] const CollisionShape& getOcclusionShape(const BlockState& state) const override;

    // ========== 放置和更新 ==========

    /**
     * @brief 方块被放置后的处理
     *
     * 附魔台放置时调度一个tick来重新计算附魔力量，
     * 因为方块实体在onBlockAdded调用时尚未创建。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居方块变化通知
     *
     * 当周围方块发生变化时，重新计算附魔力量。
     * 由于书架可以在2格距离内，邻居变化可能影响附魔力量。
     *
     * @param world 世界
     * @param pos 本方块位置
     * @param neighborBlock 变化的邻居方块
     * @param neighborPos 邻居位置
     * @param isMoving 是否因活塞推动
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    /**
     * @brief 方块tick
     *
     * 延迟1tick后重新计算附魔力量。
     * 在onBlockAdded中调度，因为方块实体在onBlockAdded调用时尚未创建。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @param random 随机数生成器
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 推动反应 ==========

    /**
     * @brief 获取推动反应
     * @param state 方块状态
     * @return 推动反应类型（附魔台不能被推动）
     */
    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Block;
    }

private:
    /// 方块形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
