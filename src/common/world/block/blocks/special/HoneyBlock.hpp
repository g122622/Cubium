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
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"

namespace mc {

class IWorld;

namespace blocks {

/**
 * @brief 蜂蜜块
 *
 * 粘性方块，实体在上面移动会减速。
 * 活塞推动时会粘住相邻方块。
 *
 * 物理：
 * - 滑度：0.5
 * - 跳跃因子：0.5
 * - 速度因子：0.4（在内部移动时）
 */
class HoneyBlock : public Block {
public:
    explicit HoneyBlock(const BlockProperties& properties);
    ~HoneyBlock() override = default;

    // ========== 实体交互 ==========

    /**
     * @brief 实体着地时调用
     *
     * 蜂蜜块不弹跳：Y 速度归零（对齐 Java updateEntityMovementAfterFallOn 走基类不反弹）。
     * 不在此处重置 fallDistance——摔落减伤由 onFallenUpon 以 multiplier=0.2 处理。
     */
    void onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    /**
     * @brief 实体摔落在蜂蜜块上时调用
     *
     * 蜂蜜块减伤 80%（保留 20%）：以 damageMultiplier=0.2 调 causeFallDamage。
     * 对齐 Java HoneyBlock#fallOn（causeFallDamage(distance, 0.2F, fall)）与 wiki
     * "摔在蜂蜜块上的生物受到的跌落伤害会减少80%"。大落差仍会受少量伤害（非完全免疫）。
     */
    void onFallenUpon(
        IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance) override;

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    // ========== 推动反应 ==========

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override;

    // ========== 蜂蜜块粘连 ==========

    [[nodiscard]] bool isStickyBlock(const BlockState& state) const noexcept override;

    [[nodiscard]] bool canStickTo(const BlockState& state, const BlockState& other) const noexcept override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

private:
    CollisionShape m_collisionShape;
};

} // namespace blocks
} // namespace mc
