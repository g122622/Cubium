#pragma once

#include "../../../interfaces/IRangedAttackMob.hpp"
#include "SpellcastingIllagerEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 幻术师实体
 */
class IllusionerEntity : public SpellcastingIllagerEntity, public entity::IRangedAttackMob {
public:
    IllusionerEntity(LegacyEntityType type, EntityId id);
    ~IllusionerEntity() override = default;

    IllusionerEntity(const IllusionerEntity&) = delete;
    IllusionerEntity& operator=(const IllusionerEntity&) = delete;
    IllusionerEntity(IllusionerEntity&&) = default;
    IllusionerEntity& operator=(IllusionerEntity&&) = default;

    static std::unique_ptr<Entity> create(IWorld* world);

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;
    i32 getAttackInterval() const override { return 20; }
    bool canRangedAttack() const override { return true; }

    [[nodiscard]] bool isCasting() const { return isSpellcasting(); }

    void castBlindnessSpell();
    void castMirrorSpell();

    [[nodiscard]] bool hasMirrors() const { return !m_mirrorEntities.empty(); }
    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;
    [[nodiscard]] const char* getSpellSoundId() const override { return "entity.illusioner.cast_spell"; }

private:
    i32 m_blindnessCooldown = 0;
    i32 m_mirrorCooldown = 0;
    std::vector<EntityId> m_mirrorEntities;

    static constexpr i32 BLINDNESS_COOLDOWN = 100;
    static constexpr i32 MIRROR_COOLDOWN = 600;
    static constexpr i32 SPELLCASTING_DURATION = 20;
};

} // namespace mc
