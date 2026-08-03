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

#include "../../Block.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 泥巴方块
 *
 * 一种由泥土加水制成的方块，实体走在上面会略微下沉。
 * 碰撞箱比完整方块略矮（14/16格高），但视觉和支持形状仍为完整方块。
 *
 * 参考: net.minecraft.block.MudBlock
 */
class MudBlock : public Block {
public:
    explicit MudBlock(const BlockProperties& properties);

    ~MudBlock() override = default;

    /**
     * @brief 返回略矮的碰撞形状（14/16格高）
     *
     * 实体会略微沉入泥巴中。支持形状和视觉形状仍为完整方块（使用默认实现）。
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 泥巴不可被路径寻找通过
     *
     * 泥巴碰撞箱比完整方块矮（14/16格高），导致默认的 allowsMovement
     * 会返回 true（非完整碰撞箱方块默认允许路径寻找通过）。因此必须
     * 显式重写返回 false，防止实体将泥巴视为可通过的路径。
     *
     * 参考: net.minecraft.block.MudBlock#isPathfindable
     */
    [[nodiscard]] bool allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 获取遮光亮度
     *
     * 泥巴虽然碰撞形状不完整（14/16高），但视觉上应产生AO阴影，
     * 因此返回 0.2F 而非默认的基于碰撞形状的判断值。
     *
     * 参考: net.minecraft.block.MudBlock#getShadeBrightness
     */
    [[nodiscard]] f32 getShadeBrightness(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return 0.2f;
    }

private:
    /// 14/16格高的碰撞形状
    CollisionShape m_collisionShape;
};

} // namespace blocks
} // namespace mc
