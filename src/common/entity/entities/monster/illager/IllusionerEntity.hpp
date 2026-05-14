/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

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
