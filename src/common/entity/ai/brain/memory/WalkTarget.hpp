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

#include <memory>
#include <utility>

#include "common/core/Types.hpp"
#include "common/entity/ai/brain/memory/BlockPosTarget.hpp"
#include "common/entity/ai/brain/memory/IPositionTarget.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace memory {

/**
 * @brief Brain 行走目标
 *
 * 封装位置目标、速度修正和可接受接近距离。
 */
class WalkTarget {
public:
    WalkTarget(PositionTargetPtr target, f32 speedModifier, i32 closeEnoughDist)
        : m_target(std::move(target))
        , m_speedModifier(speedModifier)
        , m_closeEnoughDist(closeEnoughDist)
    {
        MC_ASSERT_NOT_NULL(m_target);
    }

    WalkTarget(const BlockPos& blockPos, f32 speedModifier, i32 closeEnoughDist)
        : WalkTarget(std::make_shared<BlockPosTarget>(blockPos), speedModifier, closeEnoughDist)
    {}

    WalkTarget(const Vector3& targetPos, f32 speedModifier, i32 closeEnoughDist)
        : WalkTarget(BlockPos(targetPos), speedModifier, closeEnoughDist)
    {}

    [[nodiscard]] const PositionTargetPtr& getTarget() const noexcept { return m_target; }

    [[nodiscard]] f32 getSpeed() const noexcept { return m_speedModifier; }

    [[nodiscard]] i32 getDistance() const noexcept { return m_closeEnoughDist; }

private:
    PositionTargetPtr m_target;
    f32 m_speedModifier;
    i32 m_closeEnoughDist;
};

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
