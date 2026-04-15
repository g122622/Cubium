#pragma once

#include "AbstractSkeletonEntity.hpp"
#include "../../../../core/Types.hpp"

#include <memory>

namespace mc {

/**
 * @brief 骷髅实体
 *
 * 使用弓箭进行远程攻击的亡灵怪物。
 *
 * 参考 MC 1.16.5 SkeletonEntity。
 */
class SkeletonEntity : public AbstractSkeletonEntity {
public:
    SkeletonEntity(LegacyEntityType type, EntityId id);
    ~SkeletonEntity() override = default;

    SkeletonEntity(const SkeletonEntity&) = delete;
    SkeletonEntity& operator=(const SkeletonEntity&) = delete;
    SkeletonEntity(SkeletonEntity&&) = default;
    SkeletonEntity& operator=(SkeletonEntity&&) = default;

    static std::unique_ptr<Entity> create(IWorld* world);

    [[nodiscard]] f32 eyeHeight() const override { return 1.74f; }
    [[nodiscard]] f32 width() const override { return 0.6f; }
    [[nodiscard]] f32 height() const override { return 1.99f; }

protected:
    void registerGoals() override;
    void registerAttributes() override;
};

} // namespace mc
