#include "EntityType.hpp"
#include "Entity.hpp"

namespace mc {
namespace entity {

EntityType::~EntityType() = default;

std::unique_ptr<Entity> EntityType::create(IWorld* world) const
{
    if (!m_factory) {
        return nullptr;
    }

    auto entity = m_factory(world);
    if (entity) {
        entity->setTypeId(m_name);
    }
    return entity;
}

} // namespace entity
} // namespace mc
