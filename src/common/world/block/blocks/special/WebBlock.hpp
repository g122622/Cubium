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
 * @brief 蜘蛛网方块
 *
 * 实体经过时会被减速的网状方块。
 *
 * 物理（对齐 vanilla 1.21.11 WebBlock.entityInside → Entity.makeStuckInBlock）：
 * - 通过 setMotionMultiplier 设置本帧位移乘数 (0.25, 0.05, 0.25)，
 *   水平位移 ×0.25、垂直位移 ×0.05（不区分上/下，全方向统一减速）。
 * - makeStuckInBlock 内部 resetFallDistance：实体穿过蜘蛛网下落不累积摔落距离
 *   （落到下方实方块时 fallDistance≈0，不摔伤）。该重置由 setMotionMultiplier 统一实现。
 * - 受 WEAVING（纺织）效果的 LivingEntity 减速更轻：(0.5, 0.25, 0.5)。
 * - 不影响跳跃。
 */
class WebBlock : public Block {
public:
    explicit WebBlock(const BlockProperties& properties);
    ~WebBlock() override = default;

    // ========== 实体交互 ==========

    /**
     * @brief 实体碰撞时调用
     *
     * 大幅减缓实体速度。
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override
    {
        // 蜘蛛网无碰撞
        static CollisionShape emptyShape = CollisionShape::empty();
        return emptyShape;
    }

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
