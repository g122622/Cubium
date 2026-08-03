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

#include "../FallingBlock.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

class IWorld;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 龙蛋方块
 *
 * 末影龙死亡后掉落的方块，点击会传送。
 * 继承自 FallingBlock，下方无支撑时会下落。
 */
class DragonEggBlock : public FallingBlock {
public:
    explicit DragonEggBlock(const BlockProperties& properties);
    ~DragonEggBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 交互 ==========

    /**
     * @brief 右键点击方块
     *
     * 右键点击会触发龙蛋传送。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 左键攻击方块
     *
     * 左键点击也会触发龙蛋传送。
     */
    void attack(const BlockState& state, IWorld& world, const BlockPos& pos, Player& player) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== FallingBlock 重写 ==========

    /**
     * @brief 获取下落延迟
     *
     * 龙蛋的下落延迟为 5 tick，比普通下落方块（2 tick）更长。
     */
    [[nodiscard]] i32 getFallDelay() const override { return FALL_DELAY_TICKS; }

    /**
     * @brief 是否允许移动（路径查找）
     *
     * 龙蛋不允许实体路径查找通过。
     */
    [[nodiscard]] bool allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return false;
    }

private:
    /**
     * @brief 传送龙蛋到新位置
     *
     * 在指定范围内随机寻找空气方块位置，将龙蛋传送到该位置。
     * 最多尝试 1000 次。
     *
     * 传送范围：
     * - X: -15 ~ +15
     * - Y: -7 ~ +7
     * - Z: -15 ~ +15
     *
     * @param world 世界
     * @param pos 当前位置
     * @param state 当前方块状态
     * @return 如果传送成功返回 true
     */
    bool _teleport(IWorld& world, const BlockPos& pos, const BlockState& state);

    /// 龙蛋下落延迟（5 tick）
    static constexpr i32 FALL_DELAY_TICKS = 5;

    /// 最大传送尝试次数
    static constexpr i32 MAX_TELEPORT_ATTEMPTS = 1000;

    /// X/Z 传送范围（-15 ~ +15）
    static constexpr i32 HORIZONTAL_RANGE = 15;

    /// Y 传送范围（-7 ~ +7）
    static constexpr i32 VERTICAL_RANGE = 7;

    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
