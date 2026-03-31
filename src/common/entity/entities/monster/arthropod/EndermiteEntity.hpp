#pragma once

#include "../MonsterEntity.hpp"
#include "../../../../core/Types.hpp"

namespace mc {

/**
 * @brief 末影螨实体
 *
 * 末影人瞬移时有概率生成的敌对小生物。
 *
 * 参考 MC 1.16.5 EndermiteEntity
 */
class EndermiteEntity : public MonsterEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    EndermiteEntity(LegacyEntityType type, EntityId id);
    ~EndermiteEntity() override = default;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    i32 m_lifetime = 0;
    bool m_persistent = false;
};

/**
 * @brief 蠹虫实体
 *
 * 生成于要塞的敌对小生物，可以唤起更多蠹虫。
 *
 * 参考 MC 1.16.5 SilverfishEntity
 */
class SilverfishEntity : public MonsterEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    SilverfishEntity(LegacyEntityType type, EntityId id);
    ~SilverfishEntity() override = default;

    // ========== 生命周期 ==========

protected:
    void registerGoals() override;
    void registerAttributes() override;
};

} // namespace mc
