#include "AbstractIllagerEntity.hpp"

namespace mc {

AbstractIllagerEntity::AbstractIllagerEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
}

} // namespace mc
