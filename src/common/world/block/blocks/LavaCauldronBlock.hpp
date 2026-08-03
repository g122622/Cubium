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
 * LIABILITY OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include <memory>

namespace mc {

namespace fluid {
class Fluid;
} // namespace fluid

class Player;
class BlockRaycastResult;
class ItemStack;

namespace blocks {

/**
 * @brief 岩浆炼药锅方块
 *
 * 岩浆炼药锅是一个始终满的炼药锅变体，没有水位属性。
 * 当空炼药锅接收到岩浆滴石滴水时生成，或在玩家使用岩浆桶右键空炼药锅时生成。
 *
 * 特性：
 * - 始终满（无 LEVEL 属性）
 * - 发光等级 15
 * - 实体进入时受到岩浆伤害
 * - 比较器输出始终为 3
 * - 不可接收滴石滴水
 * - 空桶可以提取岩浆，将方块替换为空炼药锅
 * - 不响应降水
 */
class LavaCauldronBlock : public Block {
public:
    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit LavaCauldronBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~LavaCauldronBlock() override = default;

    // ========== 交互 ==========

    /**
     * @brief 玩家右键点击
     *
     * 仅处理空桶提取岩浆的交互。
     * 使用岩浆桶对岩浆炼药锅右键不做任何操作（岩浆炼药锅始终满，不需要再添加岩浆）。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 形状 ==========

    /**
     * @brief 获取渲染形状（与普通炼药锅相同的外部形状）
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（外部形状 + 岩浆内容）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 获取实体内部碰撞形状
     *
     * 返回炼药锅外部形状与岩浆内容区域的并集。实体只有进入岩浆内容区域
     * 才会触发 onEntityCollision（受到岩浆伤害）。
     *
     * 参考: net.minecraft.block.LavaCauldronBlock#getEntityInsideCollisionShape
     * 原版: FILLED_SHAPE = Shapes.or(SHAPE, Block.column(12, 4, 15))
     * 其中 SHAPE_INSIDE = Block.column(12, 4, 15) = box(2, 4, 2, 14, 15, 14)
     */
    [[nodiscard]] const CollisionShape& getEntityInsideCollisionShape(const BlockState& state) const override;

    // ========== 实体碰撞 ==========

    /**
     * @brief 实体进入方块时受到岩浆伤害
     *
     * 对 LivingEntity 造成岩浆伤害并点燃实体。
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    // ========== 滴石接收 ==========

    /**
     * @brief 岩浆炼药锅不可接收滴石滴水（始终满）
     */
    [[nodiscard]] static bool canReceiveStalactiteDrip(const fluid::Fluid& fluid)
    {
        MC_UNUSED(fluid);
        return false;
    }

    // ========== 红石 ==========

    /**
     * @brief 比较器输出始终为 3
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    /**
     * @brief 始终有比较器输入覆盖
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 光照 ==========

    /**
     * @brief 岩浆炼药锅发光等级为 15
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return 15;
    }

    // ========== 静态工具方法 ==========

    /**
     * @brief 岩浆炼药锅始终满
     */
    [[nodiscard]] static bool isFull(const BlockState& state)
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 方块状态 ==========

    /**
     * @brief 岩浆炼药锅没有水位属性，不需要额外状态
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

private:
    /// 炼药锅外部形状（与普通炼药锅共享）
    CollisionShape m_outerShape;

    /// 填充形状 = 外部形状 ∪ 岩浆内容形状（用于实体内部碰撞检测）
    /// 参考 MC 原版: FILLED_SHAPE = Shapes.or(SHAPE, Block.column(12, 4, 15))
    /// SHAPE_INSIDE = Block.column(12, 4, 15) = box(2, 4, 2, 14, 15, 14)（像素坐标）
    CollisionShape m_filledShape;
};

} // namespace blocks
} // namespace mc
