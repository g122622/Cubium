#pragma once

#include "SpellcastingIllagerEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 唤魔者实体
 */
class EvokerEntity : public SpellcastingIllagerEntity {
public:
    EvokerEntity(LegacyEntityType type, EntityId id);
    ~EvokerEntity() override = default;

    EvokerEntity(const EvokerEntity&) = delete;
    EvokerEntity& operator=(const EvokerEntity&) = delete;
    EvokerEntity(EvokerEntity&&) = default;
    EvokerEntity& operator=(EvokerEntity&&) = default;

    static std::unique_ptr<Entity> create(IWorld* world);

    [[nodiscard]] bool isCasting() const { return isSpellcasting(); }
    [[nodiscard]] i32 getSpellType() const { return static_cast<i32>(spellType()); }

    void startCasting(i32 spellType);
    void finishCasting();

    void castFangsAttack();
    void summonVex();

    [[nodiscard]] i32 getFangsCooldown() const { return m_fangsCooldown; }
    [[nodiscard]] i32 getSummonCooldown() const { return m_summonCooldown; }

    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;
    [[nodiscard]] const char* getSpellSoundId() const override { return "entity.evoker.cast_spell"; }

private:
    i32 m_fangsCooldown = 0;
    i32 m_summonCooldown = 0;

    static constexpr i32 CASTING_DURATION = 40;
    static constexpr i32 FANGS_COOLDOWN = 100;
    static constexpr i32 SUMMON_COOLDOWN = 340;
};

} // namespace mc
