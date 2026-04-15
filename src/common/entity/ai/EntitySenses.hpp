#pragma once

#include <vector>

namespace mc {

class Entity;
class MobEntity;

namespace entity::ai {

/**
 * @brief 实体视线缓存
 *
 * 对齐 1.16.5 `EntitySenses` 的最小公共层。
 */
class EntitySenses {
public:
    explicit EntitySenses(MobEntity* mob);

    void tick();

    [[nodiscard]] bool canSee(const Entity& entity);

private:
    MobEntity* m_mob;
    std::vector<const Entity*> m_seenEntities;
    std::vector<const Entity*> m_unseenEntities;
};

} // namespace entity::ai
} // namespace mc
