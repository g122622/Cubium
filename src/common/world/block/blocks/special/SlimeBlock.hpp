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

#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"

namespace mc {

class IWorld;

namespace blocks {

/**
 * @brief 粘液块
 *
 * 弹性方块，实体落在上面会弹跳。
 * 活塞推动时会粘住相邻方块。
 *
 * 物理：
 * - 弹跳系数：0.9（每次弹跳损失 10% 速度）
 * - 滑度：0.8
 */
class SlimeBlock : public Block {
public:
    explicit SlimeBlock(const BlockProperties& properties);
    ~SlimeBlock() override = default;

    // ========== 实体交互 ==========

    /**
     * @brief 实体着地时调用
     *
     * 实现弹跳效果：如果实体向下落，Y速度取反并乘以 0.9。
     */
    void onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    // ========== 推动反应 ==========

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override;

    // ========== 黏液块粘连 ==========

    [[nodiscard]] bool isStickyBlock(const BlockState& state) const noexcept override;

    [[nodiscard]] bool canStickTo(const BlockState& state, const BlockState& other) const noexcept override;
};

} // namespace blocks
} // namespace mc
