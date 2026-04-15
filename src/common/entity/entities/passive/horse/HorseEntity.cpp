#include "HorseEntity.hpp"

#include "../../../attribute/Attributes.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {

namespace {

[[nodiscard]] constexpr i32 packHorseVariant(CoatColors color, CoatTypes type)
{
    return static_cast<i32>(getCoatColorId(color)) |
           (static_cast<i32>(getCoatTypeId(type)) << 8);
}

} // namespace

HorseEntity::HorseEntity(LegacyEntityType type, EntityId id)
    : AbstractHorseEntity(type, id)
{
    randomizeAppearance();
}

std::unique_ptr<Entity> HorseEntity::create(IWorld* /*world*/)
{
    return std::make_unique<HorseEntity>(LegacyEntityType::Unknown, 0);
}

i32 HorseEntity::getVariant() const
{
    return packHorseVariant(m_color, m_marking);
}

void HorseEntity::setVariant(i32 variant)
{
    m_color = getCoatColorById(variant & 0xFF);
    m_marking = getCoatTypeById((variant >> 8) & 0xFF);
}

void HorseEntity::randomizeAppearance()
{
    math::Random random(ticksExisted());
    m_color = getCoatColorById(random.nextInt(COAT_COLORS_COUNT));
    m_marking = getCoatTypeById(random.nextInt(COAT_TYPES_COUNT));
}

bool HorseEntity::isTameItem(const ItemStack& /*itemStack*/) const
{
    return false;
}

bool HorseEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // TODO: 对齐 1.16.5 的金苹果 / 金胡萝卜繁殖逻辑。
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> HorseEntity::spawnBaby(AnimalEntity& partner)
{
    // TODO: 对齐 1.16.5 的马 x 马 / 马 x 驴 后代外观与属性遗传逻辑。
    (void)partner;
    return nullptr;
}

void HorseEntity::tick()
{
    AbstractHorseEntity::tick();

    if (m_isRearing) {
        --m_rearingCounter;
        if (m_rearingCounter <= 0) {
            m_isRearing = false;
        }
    }

    if (!isTame() && isBeingRidden() && !m_isRearing) {
        math::Random random(ticksExisted());
        if (random.nextFloat() < 0.02f) {
            m_isRearing = true;
            m_rearingCounter = 20;
            // TODO: 对齐 1.16.5 的未驯服马上抬前腿和甩下骑手逻辑。
        }
    }
}

void HorseEntity::registerGoals()
{
    AbstractHorseEntity::registerGoals();
    // TODO: 补齐马的 RunAroundLikeCrazyGoal 等目标。
}

void HorseEntity::registerAttributes()
{
    AbstractHorseEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, m_horseHealth);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_speed);
}

} // namespace mc
