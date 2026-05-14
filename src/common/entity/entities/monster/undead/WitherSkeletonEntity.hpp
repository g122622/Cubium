#pragma once

#include "../../../../core/Types.hpp"
#include "AbstractSkeletonEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 凋灵骷髅实体
 *
 * 对齐 MC 1.16.5 `WitherSkeletonEntity` 的骷髅变种层次。
 */
class WitherSkeletonEntity : public AbstractSkeletonEntity {
public:
    WitherSkeletonEntity(LegacyEntityType type, EntityId id);

    ~WitherSkeletonEntity() override = default;

    WitherSkeletonEntity(const WitherSkeletonEntity&) = delete;
    WitherSkeletonEntity& operator=(const WitherSkeletonEntity&) = delete;
    WitherSkeletonEntity(WitherSkeletonEntity&&) = default;
    WitherSkeletonEntity& operator=(WitherSkeletonEntity&&) = default;

    static std::unique_ptr<Entity> create(IWorld* world);

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override
    {
        (void)target;
        (void)charge;
    }

    [[nodiscard]] bool hasStoneSword() const { return m_hasStoneSword; }
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }
    [[nodiscard]] f32 eyeHeight() const override { return 2.1f; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_hasStoneSword = true;
};

} // namespace mc
