#include "SpellcastingIllagerEntity.hpp"

namespace mc {

SpellcastingIllagerEntity::SpellcastingIllagerEntity(LegacyEntityType type, EntityId id)
    : AbstractIllagerEntity(type, id)
{}

SpellcastingIllagerEntity::SpellType SpellcastingIllagerEntity::spellTypeFromId(i32 id)
{
    switch (id) {
        case 1:
            return SpellType::SummonVex;
        case 2:
            return SpellType::Fangs;
        case 3:
            return SpellType::Wololo;
        case 4:
            return SpellType::Disappear;
        case 5:
            return SpellType::Blindness;
        default:
            return SpellType::None;
    }
}

void SpellcastingIllagerEntity::tick()
{
    AbstractIllagerEntity::tick();

    if (m_spellTicks > 0) {
        --m_spellTicks;
    }

    // TODO: 接入客户端粒子与同步后，补齐施法粒子颜色反馈
}

} // namespace mc
