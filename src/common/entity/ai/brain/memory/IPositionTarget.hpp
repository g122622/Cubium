#pragma once

#include "../../../../world/block/BlockPos.hpp"
#include "../../../../util/math/Vector3.hpp"

#include <memory>

namespace mc {

class LivingEntity;

namespace entity {
namespace ai {
namespace brain {
namespace memory {

/**
 * @brief Brain 位置目标抽象
 *
 * 对齐 MC 1.16.5 IPosWrapper，用于在记忆中统一保存“看向/走向”的目标。
 */
class IPositionTarget {
public:
    virtual ~IPositionTarget() = default;

    [[nodiscard]] virtual Vector3 getPosition() const = 0;
    [[nodiscard]] virtual BlockPos getBlockPos() const = 0;
    [[nodiscard]] virtual bool isVisibleTo(const LivingEntity& viewer) const = 0;
};

using PositionTargetPtr = std::shared_ptr<IPositionTarget>;

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
