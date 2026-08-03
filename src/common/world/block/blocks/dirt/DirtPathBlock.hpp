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
 * The above copyright notice and this permission shall be included in all
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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 草径方块（土径）
 *
 * 由泥土用锹右键踩出的小路方块，顶部比完整方块矮 1 像素（15/16 格高）。
 * 渲染形状与碰撞形状均为 15/16 高。
 *
 * 参考: net.minecraft.block.DirtPathBlock
 */
class DirtPathBlock : public Block {
public:
    explicit DirtPathBlock(const BlockProperties& properties);

    ~DirtPathBlock() override = default;

    /**
     * @brief 获取渲染形状（15/16 格高）
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（15/16 格高，与渲染形状一致）
     *
     * 注意：与 FarmlandBlock 不同，FarmlandBlock 的碰撞形状是完整方块。
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 草径不可被路径寻找通过
     *
     * 草径碰撞箱比完整方块矮（15/16格高），导致默认的 allowsMovement
     * 会返回 true（非完整碰撞箱方块默认允许路径寻找通过）。因此必须
     * 显式重写返回 false，防止实体将草径视为可通过的路径。
     */
    [[nodiscard]] bool allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // TODO: 实现 updatePostPlacement + tick 的转变泥土逻辑：
    //       上方落入实体或放置方块时，草径应 scheduleTick 并在 tick 中转变回泥土（VanillaBlocks::DIRT）。
    //       当前仅修复渲染高度，该功能逻辑待后续实现。

private:
    /// 15/16 格高的形状（渲染与碰撞共用）
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
