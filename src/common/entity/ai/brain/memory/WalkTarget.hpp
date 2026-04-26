#pragma once

#include "BlockPosTarget.hpp"
#include "../../../../util/assert/AssertAll.hpp"

#include <utility>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace memory {

/**
 * @brief Brain 行走目标
 *
 * 对齐 MC 1.16.5 WalkTarget，封装位置目标、速度修正和可接受接近距离。
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
    {
    }

    WalkTarget(const Vector3& targetPos, f32 speedModifier, i32 closeEnoughDist)
        : WalkTarget(BlockPos(targetPos), speedModifier, closeEnoughDist)
    {
    }

    [[nodiscard]] const PositionTargetPtr& getTarget() const
    {
        return m_target;
    }

    [[nodiscard]] f32 getSpeed() const
    {
        return m_speedModifier;
    }

    [[nodiscard]] i32 getDistance() const
    {
        return m_closeEnoughDist;
    }

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
