#pragma once

#include "IPositionTarget.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace memory {

/**
 * @brief 基于方块坐标的位置目标
 *
 * 对齐 MC 1.16.5 BlockPosWrapper，始终使用方块中心点作为导航/注视位置。
 */
class BlockPosTarget final : public IPositionTarget {
public:
    explicit BlockPosTarget(const BlockPos& blockPos)
        : m_blockPos(blockPos)
        , m_centerPos(blockPos.center())
    {
    }

    [[nodiscard]] Vector3 getPosition() const override
    {
        return m_centerPos;
    }

    [[nodiscard]] BlockPos getBlockPos() const override
    {
        return m_blockPos;
    }

    [[nodiscard]] bool isVisibleTo(const LivingEntity& /*viewer*/) const override
    {
        return true;
    }

private:
    BlockPos m_blockPos;
    Vector3 m_centerPos;
};

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
