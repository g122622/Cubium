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

#include "IPositionTarget.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace memory {

/**
 * @brief 基于方块坐标的位置目标
 *
 * 始终使用方块中心点作为导航/注视位置。
 */
class BlockPosTarget final : public IPositionTarget {
public:
    explicit BlockPosTarget(const BlockPos& blockPos)
        : m_blockPos(blockPos)
        , m_centerPos(blockPos.center())
    {}

    [[nodiscard]] Vector3 getPosition() const override { return m_centerPos; }

    [[nodiscard]] BlockPos getBlockPos() const override { return m_blockPos; }

    [[nodiscard]] bool isVisibleTo(const LivingEntity& /*viewer*/) const override { return true; }

private:
    BlockPos m_blockPos;
    Vector3 m_centerPos;
};

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
