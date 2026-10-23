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

#include "common/mod/bedrock/addon/binding/ScriptDamageCause.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <array>
#include <utility>

namespace mc::mod::bedrock::addon {
namespace {
constexpr auto _CAUSES = std::to_array<std::pair<mc::DamageType, std::string_view>>({
    {mc::DamageType::InFire, "fire"},
    {mc::DamageType::Campfire, "campfire"},
    {mc::DamageType::LightningBolt, "lightning"},
    {mc::DamageType::OnFire, "fireTick"},
    {mc::DamageType::Lava, "lava"},
    {mc::DamageType::HotFloor, "magma"},
    {mc::DamageType::InWall, "suffocation"},
    {mc::DamageType::Cramming, "suffocation"},
    {mc::DamageType::Drown, "drowning"},
    {mc::DamageType::Starve, "starve"},
    {mc::DamageType::Cactus, "contact"},
    {mc::DamageType::Fall, "fall"},
    {mc::DamageType::EnderPearl, "fall"},
    {mc::DamageType::FlyIntoWall, "flyIntoWall"},
    {mc::DamageType::OutOfWorld, "void"},
    {mc::DamageType::Generic, "none"},
    {mc::DamageType::Magic, "magic"},
    {mc::DamageType::Wither, "wither"},
    {mc::DamageType::DragonBreath, "magic"},
    {mc::DamageType::Dryout, "drowning"},
    {mc::DamageType::SweetBerryBush, "contact"},
    {mc::DamageType::Freeze, "freezing"},
    {mc::DamageType::Stalagmite, "stalagmite"},
    {mc::DamageType::FallingBlock, "fallingBlock"},
    {mc::DamageType::FallingAnvil, "anvil"},
    {mc::DamageType::FallingStalactite, "stalactite"},
    {mc::DamageType::Sting, "entityAttack"},
    {mc::DamageType::MobAttack, "entityAttack"},
    {mc::DamageType::MobAttackNoAggro, "entityAttack"},
    {mc::DamageType::PlayerAttack, "entityAttack"},
    {mc::DamageType::Spear, "entityAttack"},
    {mc::DamageType::Arrow, "projectile"},
    {mc::DamageType::Trident, "projectile"},
    {mc::DamageType::MobProjectile, "projectile"},
    {mc::DamageType::Spit, "projectile"},
    {mc::DamageType::Fireworks, "fireworks"},
    {mc::DamageType::UnattributedFireball, "projectile"},
    {mc::DamageType::Fireball, "projectile"},
    {mc::DamageType::WitherSkull, "projectile"},
    {mc::DamageType::Thrown, "projectile"},
    {mc::DamageType::IndirectMagic, "magic"},
    {mc::DamageType::Thorns, "thorns"},
    {mc::DamageType::Explosion, "blockExplosion"},
    {mc::DamageType::ExplosionPlayer, "entityExplosion"},
    {mc::DamageType::SonicBoom, "sonicBoom"},
    {mc::DamageType::BadRespawnPoint, "blockExplosion"},
    {mc::DamageType::WindBurst, "projectile"},
    {mc::DamageType::MaceSmash, "maceSmash"},
    {mc::DamageType::OutsideBorder, "void"},
    {mc::DamageType::GenericKill, "selfDestruct"},
});
} // namespace

std::string_view damageCauseName(mc::DamageType type)
{
    for (const auto& [kind, name] : _CAUSES) {
        if (kind == type) {
            return name;
        }
    }

    MC_ASSERT_RELEASE_MSG(false, "Unmapped damage cause");

    return "none";
}

std::optional<mc::DamageType> parseDamageCause(std::string_view name)
{
    for (const auto& [kind, value] : _CAUSES) {
        if (name == value) {
            return kind;
        }
    }

    return std::nullopt;
}

} // namespace mc::mod::bedrock::addon
