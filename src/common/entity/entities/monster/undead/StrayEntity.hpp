#pragma once

#include "AbstractSkeletonEntity.hpp"
#include "../../../../core/Types.hpp"

#include <memory>

namespace mc {

/**
 * @brief 流浪者实体
 *
 * 对齐 MC 1.16.5 `StrayEntity` 的骷髅变种层次。
 */
class StrayEntity : public AbstractSkeletonEntity {
public:
    StrayEntity(LegacyEntityType type, EntityId id);

    ~StrayEntity() override = default;

    StrayEntity(const StrayEntity&) = delete;
    StrayEntity& operator=(const StrayEntity&) = delete;
    StrayEntity(StrayEntity&&) = default;
    StrayEntity& operator=(StrayEntity&&) = default;

    static std::unique_ptr<Entity> create(IWorld* world);

    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

protected:
    void registerAttributes() override;
};

} // namespace mc
