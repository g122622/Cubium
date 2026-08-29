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

#include "DamageTypeTags.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTag.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

namespace mc {

// ============================================================================
// DamageTypeTags 实现
// ============================================================================

bool DamageTypeTags::s_initialized = false;

std::unordered_map<ResourceLocation, std::unique_ptr<DamageTypeTag>>& DamageTypeTags::_getTags()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<DamageTypeTag>> tags;
    return tags;
}

// ============================================================================
// 标签访问器（惰性初始化）
// ============================================================================

// ========== 绕过防御类标签 ==========

DamageTypeTag& DamageTypeTags::BYPASSES_ARMOR()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:bypasses_armor"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:bypasses_armor")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::BYPASSES_INVULNERABILITY()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:bypasses_invulnerability"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:bypasses_invulnerability")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::BYPASSES_COOLDOWN()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:bypasses_cooldown"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:bypasses_cooldown")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::BYPASSES_RESISTANCE()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:bypasses_resistance"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:bypasses_resistance")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::BYPASSES_SHIELD()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:bypasses_shield"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:bypasses_shield")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::BYPASSES_EFFECTS()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:bypasses_effects"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:bypasses_effects")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::BYPASSES_ENCHANTMENTS()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:bypasses_enchantments"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:bypasses_enchantments")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::BYPASSES_WOLF_ARMOR()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:bypasses_wolf_armor"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:bypasses_wolf_armor")] = std::move(t);
    }
    return *tag;
}

// ========== 伤害分类标签 ==========

DamageTypeTag& DamageTypeTags::IS_DROWNING()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:is_drowning"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:is_drowning")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::IS_EXPLOSION()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:is_explosion"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:is_explosion")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::IS_FALL()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:is_fall"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:is_fall")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::IS_FIRE()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:is_fire"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:is_fire")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::IS_FREEZING()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:is_freezing"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:is_freezing")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::IS_LIGHTNING()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:is_lightning"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:is_lightning")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::IS_PLAYER_ATTACK()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:is_player_attack"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:is_player_attack")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::IS_PROJECTILE()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:is_projectile"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:is_projectile")] = std::move(t);
    }
    return *tag;
}

// ========== 重锤相关标签 ==========

DamageTypeTag& DamageTypeTags::MACE_SMASH()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:mace_smash"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:mace_smash")] = std::move(t);
    }
    return *tag;
}

// ========== AI 行为相关标签 ==========

DamageTypeTag& DamageTypeTags::NO_ANGER()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:no_anger"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:no_anger")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::NO_IMPACT()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:no_impact"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:no_impact")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::NO_KNOCKBACK()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:no_knockback"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:no_knockback")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::PANIC_CAUSES()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:panic_causes"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:panic_causes")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::PANIC_ENVIRONMENTAL_CAUSES()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:panic_environmental_causes"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:panic_environmental_causes")] = std::move(t);
    }
    return *tag;
}

// ========== 特殊生物标签 ==========

DamageTypeTag& DamageTypeTags::WITCH_RESISTANT_TO()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:witch_resistant_to"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:witch_resistant_to")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::WITHER_IMMUNE_TO()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:wither_immune_to"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:wither_immune_to")] = std::move(t);
    }
    return *tag;
}

// ========== 末影龙相关标签 ==========

DamageTypeTag& DamageTypeTags::ALWAYS_HURTS_ENDER_DRAGONS()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:always_hurts_ender_dragons"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:always_hurts_ender_dragons")] = std::move(t);
    }
    return *tag;
}

// ========== 盔甲架相关标签 ==========

DamageTypeTag& DamageTypeTags::ALWAYS_KILLS_ARMOR_STANDS()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:always_kills_armor_stands"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:always_kills_armor_stands")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::BURNS_ARMOR_STANDS()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:burns_armor_stands"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:burns_armor_stands")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::CAN_BREAK_ARMOR_STAND()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:can_break_armor_stand"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:can_break_armor_stand")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::IGNITES_ARMOR_STANDS()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:ignites_armor_stands"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:ignites_armor_stands")] = std::move(t);
    }
    return *tag;
}

// ========== 其他特殊标签 ==========

DamageTypeTag& DamageTypeTags::ALWAYS_MOST_SIGNIFICANT_FALL()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:always_most_significant_fall"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:always_most_significant_fall")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::ALWAYS_TRIGGERS_SILVERFISH()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:always_triggers_silverfish"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:always_triggers_silverfish")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::AVOIDS_GUARDIAN_THORNS()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:avoids_guardian_thorns"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:avoids_guardian_thorns")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::BURN_FROM_STEPPING()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:burn_from_stepping"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:burn_from_stepping")] = std::move(t);
    }
    return *tag;
}

DamageTypeTag& DamageTypeTags::DAMAGES_HELMET()
{
    static DamageTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<DamageTypeTag>(ResourceLocation("minecraft:damages_helmet"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:damages_helmet")] = std::move(t);
    }
    return *tag;
}

// ============================================================================
// 初始化（硬编码默认值，与 MC 1.21.11 数据包保持一致）
// ============================================================================

void DamageTypeTags::initialize()
{
    if (s_initialized) {
        return;
    }

    // 确保所有标签已创建（惰性初始化）
    BYPASSES_ARMOR();
    BYPASSES_INVULNERABILITY();
    BYPASSES_COOLDOWN();
    BYPASSES_RESISTANCE();
    BYPASSES_SHIELD();
    BYPASSES_EFFECTS();
    BYPASSES_ENCHANTMENTS();
    BYPASSES_WOLF_ARMOR();
    IS_DROWNING();
    IS_EXPLOSION();
    IS_FALL();
    IS_FIRE();
    IS_FREEZING();
    IS_LIGHTNING();
    IS_PLAYER_ATTACK();
    IS_PROJECTILE();
    MACE_SMASH();
    NO_ANGER();
    NO_IMPACT();
    NO_KNOCKBACK();
    PANIC_CAUSES();
    PANIC_ENVIRONMENTAL_CAUSES();
    WITCH_RESISTANT_TO();
    WITHER_IMMUNE_TO();
    ALWAYS_HURTS_ENDER_DRAGONS();
    ALWAYS_KILLS_ARMOR_STANDS();
    BURNS_ARMOR_STANDS();
    CAN_BREAK_ARMOR_STAND();
    IGNITES_ARMOR_STANDS();
    ALWAYS_MOST_SIGNIFICANT_FALL();
    ALWAYS_TRIGGERS_SILVERFISH();
    AVOIDS_GUARDIAN_THORNS();
    BURN_FROM_STEPPING();
    DAMAGES_HELMET();

    // ========== 硬编码默认标签成员 ==========
    // 这些默认值与 MC 1.21.11 Vanilla 数据包中的 damage_type 标签 JSON 文件保持一致，
    // 数据包加载（DamageTypeTagLoader）会在 initialize() 之后追加或替换这些默认值。

    // bypasses_armor
    BYPASSES_ARMOR().addAll({
        DamageType::OnFire,
        DamageType::InWall,
        DamageType::Cramming,
        DamageType::Drown,
        DamageType::FlyIntoWall,
        DamageType::Generic,
        DamageType::Wither,
        DamageType::DragonBreath,
        DamageType::Starve,
        DamageType::Fall,
        DamageType::EnderPearl,
        DamageType::Freeze,
        DamageType::Stalagmite,
        DamageType::Magic,
        DamageType::IndirectMagic,
        DamageType::OutOfWorld,
        DamageType::GenericKill,
        DamageType::SonicBoom,
        DamageType::OutsideBorder,
    });

    // bypasses_invulnerability
    BYPASSES_INVULNERABILITY().addAll({
        DamageType::OutOfWorld,
        DamageType::GenericKill,
    });

    // bypasses_cooldown —— 1.21.11 vanilla 数据包中为空标签，不 addAll。
    // 语义保留：LivingEntity.hurtServer:1191 的无敌帧冷却守卫需查询此标签，
    // 数据包可扩展成员使某伤害类型绕过无敌帧冷却（始终走重置分支、全额承受）。
    // 对齐 Cubium LivingEntity::hurt 的 hurtResistantTime>10 差额逻辑守卫。

    // bypasses_resistance
    BYPASSES_RESISTANCE().addAll({
        DamageType::OutOfWorld,
        DamageType::GenericKill,
    });

    // bypasses_shield（包含 #bypasses_armor 子标签 + 直接成员）
    BYPASSES_SHIELD().addAll({
        // #minecraft:bypasses_armor 子标签成员（展开引用）
        DamageType::OnFire,
        DamageType::InWall,
        DamageType::Cramming,
        DamageType::Drown,
        DamageType::FlyIntoWall,
        DamageType::Generic,
        DamageType::Wither,
        DamageType::DragonBreath,
        DamageType::Starve,
        DamageType::Fall,
        DamageType::EnderPearl,
        DamageType::Freeze,
        DamageType::Stalagmite,
        DamageType::Magic,
        DamageType::IndirectMagic,
        DamageType::OutOfWorld,
        DamageType::GenericKill,
        DamageType::SonicBoom,
        DamageType::OutsideBorder,
        // 直接成员
        DamageType::Cactus,
        DamageType::Campfire,
        DamageType::Dryout,
        DamageType::FallingAnvil,
        DamageType::FallingStalactite,
        DamageType::HotFloor,
        DamageType::InFire,
        DamageType::Lava,
        DamageType::LightningBolt,
        DamageType::SweetBerryBush,
    });

    // bypasses_effects
    BYPASSES_EFFECTS().addAll({
        DamageType::Starve,
    });

    // bypasses_enchantments
    BYPASSES_ENCHANTMENTS().addAll({
        DamageType::SonicBoom,
    });

    // bypasses_wolf_armor（包含 #bypasses_invulnerability 子标签 + 直接成员）
    BYPASSES_WOLF_ARMOR().addAll({
        // #minecraft:bypasses_invulnerability 子标签成员（展开引用）
        DamageType::OutOfWorld,
        DamageType::GenericKill,
        // 直接成员
        DamageType::Cramming,
        DamageType::Drown,
        DamageType::Dryout,
        DamageType::Freeze,
        DamageType::InWall,
        DamageType::IndirectMagic,
        DamageType::Magic,
        DamageType::OutsideBorder,
        DamageType::Starve,
        DamageType::Thorns,
        DamageType::Wither,
    });

    // is_drowning
    IS_DROWNING().addAll({
        DamageType::Drown,
    });

    // is_explosion
    IS_EXPLOSION().addAll({
        DamageType::Fireworks,
        DamageType::Explosion,
        DamageType::ExplosionPlayer,
        DamageType::BadRespawnPoint,
    });

    // is_fall
    IS_FALL().addAll({
        DamageType::Fall,
        DamageType::EnderPearl,
        DamageType::Stalagmite,
    });

    // is_fire
    IS_FIRE().addAll({
        DamageType::InFire,
        DamageType::Campfire,
        DamageType::OnFire,
        DamageType::Lava,
        DamageType::HotFloor,
        DamageType::UnattributedFireball,
        DamageType::Fireball,
    });

    // is_freezing
    IS_FREEZING().addAll({
        DamageType::Freeze,
    });

    // is_lightning
    IS_LIGHTNING().addAll({
        DamageType::LightningBolt,
    });

    // is_player_attack
    IS_PLAYER_ATTACK().addAll({
        DamageType::PlayerAttack,
        DamageType::Spear,
        DamageType::MaceSmash,
    });

    // is_projectile
    IS_PROJECTILE().addAll({
        DamageType::Arrow,
        DamageType::Trident,
        DamageType::MobProjectile,
        DamageType::UnattributedFireball,
        DamageType::Fireball,
        DamageType::WitherSkull,
        DamageType::Thrown,
        DamageType::WindBurst,
    });

    // mace_smash
    MACE_SMASH().addAll({
        DamageType::MaceSmash,
    });

    // no_anger
    NO_ANGER().addAll({
        DamageType::MobAttackNoAggro,
    });

    // no_impact
    NO_IMPACT().addAll({
        DamageType::Drown,
    });

    // no_knockback
    NO_KNOCKBACK().addAll({
        DamageType::Explosion,
        DamageType::ExplosionPlayer,
        DamageType::BadRespawnPoint,
        DamageType::InFire,
        DamageType::LightningBolt,
        DamageType::OnFire,
        DamageType::Lava,
        DamageType::HotFloor,
        DamageType::InWall,
        DamageType::Cramming,
        DamageType::Drown,
        DamageType::Starve,
        DamageType::Cactus,
        DamageType::Fall,
        DamageType::EnderPearl,
        DamageType::FlyIntoWall,
        DamageType::OutOfWorld,
        DamageType::Generic,
        DamageType::Magic,
        DamageType::Wither,
        DamageType::DragonBreath,
        DamageType::Dryout,
        DamageType::SweetBerryBush,
        DamageType::Freeze,
        DamageType::Stalagmite,
        DamageType::OutsideBorder,
        DamageType::GenericKill,
        DamageType::Campfire,
        DamageType::Spear,
    });

    // panic_environmental_causes
    PANIC_ENVIRONMENTAL_CAUSES().addAll({
        DamageType::Cactus,
        DamageType::Freeze,
        DamageType::HotFloor,
        DamageType::InFire,
        DamageType::Lava,
        DamageType::LightningBolt,
        DamageType::OnFire,
    });

    // panic_causes（包含 #panic_environmental_causes + #is_player_attack 子标签 + 直接成员）
    PANIC_CAUSES().addAll({
        // #minecraft:panic_environmental_causes 子标签成员
        DamageType::Cactus,
        DamageType::Freeze,
        DamageType::HotFloor,
        DamageType::InFire,
        DamageType::Lava,
        DamageType::LightningBolt,
        DamageType::OnFire,
        // 直接成员
        DamageType::Arrow,
        DamageType::DragonBreath,
        DamageType::Explosion,
        DamageType::Fireball,
        DamageType::Fireworks,
        DamageType::IndirectMagic,
        DamageType::Magic,
        DamageType::MobAttack,
        DamageType::MobProjectile,
        DamageType::ExplosionPlayer,
        DamageType::SonicBoom,
        DamageType::Sting,
        DamageType::Thrown,
        DamageType::Trident,
        DamageType::UnattributedFireball,
        DamageType::WindBurst,
        DamageType::Wither,
        DamageType::WitherSkull,
        // #minecraft:is_player_attack 子标签成员
        DamageType::PlayerAttack,
        DamageType::Spear,
        DamageType::MaceSmash,
    });

    // witch_resistant_to
    WITCH_RESISTANT_TO().addAll({
        DamageType::Magic,
        DamageType::IndirectMagic,
        DamageType::SonicBoom,
        DamageType::Thorns,
    });

    // wither_immune_to
    WITHER_IMMUNE_TO().addAll({
        DamageType::Drown,
    });

    // always_hurts_ender_dragons（包含 #is_explosion 子标签）
    ALWAYS_HURTS_ENDER_DRAGONS().addAll({
        // #minecraft:is_explosion 子标签成员
        DamageType::Fireworks,
        DamageType::Explosion,
        DamageType::ExplosionPlayer,
        DamageType::BadRespawnPoint,
    });

    // always_kills_armor_stands
    ALWAYS_KILLS_ARMOR_STANDS().addAll({
        DamageType::Arrow,
        DamageType::Trident,
        DamageType::Fireball,
        DamageType::WitherSkull,
        DamageType::WindBurst,
    });

    // burns_armor_stands
    BURNS_ARMOR_STANDS().addAll({
        DamageType::OnFire,
    });

    // can_break_armor_stand（包含 #is_player_attack 子标签 + 直接成员）
    CAN_BREAK_ARMOR_STAND().addAll({
        DamageType::ExplosionPlayer,
        // #minecraft:is_player_attack 子标签成员
        DamageType::PlayerAttack,
        DamageType::Spear,
        DamageType::MaceSmash,
    });

    // ignites_armor_stands
    IGNITES_ARMOR_STANDS().addAll({
        DamageType::InFire,
        DamageType::Campfire,
    });

    // always_most_significant_fall
    ALWAYS_MOST_SIGNIFICANT_FALL().addAll({
        DamageType::OutOfWorld,
    });

    // always_triggers_silverfish
    ALWAYS_TRIGGERS_SILVERFISH().addAll({
        DamageType::Magic,
    });

    // avoids_guardian_thorns（包含 #is_explosion 子标签 + 直接成员）
    AVOIDS_GUARDIAN_THORNS().addAll({
        DamageType::Magic,
        DamageType::Thorns,
        // #minecraft:is_explosion 子标签成员
        DamageType::Fireworks,
        DamageType::Explosion,
        DamageType::ExplosionPlayer,
        DamageType::BadRespawnPoint,
    });

    // burn_from_stepping
    BURN_FROM_STEPPING().addAll({
        DamageType::Campfire,
        DamageType::HotFloor,
    });

    // damages_helmet
    DAMAGES_HELMET().addAll({
        DamageType::FallingAnvil,
        DamageType::FallingBlock,
        DamageType::FallingStalactite,
    });

    s_initialized = true;
}

DamageTypeTag* DamageTypeTags::getTag(const ResourceLocation& id)
{
    auto& tags = _getTags();
    auto it = tags.find(id);
    return it != tags.end() ? it->second.get() : nullptr;
}

DamageTypeTag& DamageTypeTags::registerTag(const ResourceLocation& id)
{
    auto& tags = _getTags();
    auto it = tags.find(id);
    if (it != tags.end()) {
        return *it->second;
    }
    auto tag = std::make_unique<DamageTypeTag>(id);
    auto& ref = *tag;
    tags[id] = std::move(tag);
    return ref;
}

void DamageTypeTags::forEachTag(std::function<void(DamageTypeTag&)> callback)
{
    for (auto& [id, tag] : _getTags()) {
        callback(*tag);
    }
}

} // namespace mc
