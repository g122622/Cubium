/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "EntityTypeTags.hpp"
#include "common/entity/tag/EntityTypeTag.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// EntityTypeTags 实现
// ============================================================================

bool EntityTypeTags::s_initialized = false;

std::unordered_map<ResourceLocation, std::unique_ptr<EntityTypeTag>>& EntityTypeTags::_getTags()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<EntityTypeTag>> tags;
    return tags;
}

// ========== 投射物标签 ==========

EntityTypeTag& EntityTypeTags::ARROWS()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:arrows"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:arrows")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::IMPACT_PROJECTILES()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:impact_projectiles"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:impact_projectiles")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::REDIRECTABLE_PROJECTILE()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:redirectable_projectile"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:redirectable_projectile")] = std::move(t);
    }
    return *tag;
}

// ========== 亡灵/节肢/水生标签 ==========

EntityTypeTag& EntityTypeTags::SKELETONS()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:skeletons"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:skeletons")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::ZOMBIES()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:zombies"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:zombies")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::UNDEAD()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:undead"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:undead")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::ARTHROPOD()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:arthropod"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:arthropod")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::AQUATIC()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:aquatic"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:aquatic")] = std::move(t);
    }
    return *tag;
}

// ========== 附魔敏感标签 ==========

EntityTypeTag& EntityTypeTags::SENSITIVE_TO_BANE_OF_ARTHROPODS()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:sensitive_to_bane_of_arthropods"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:sensitive_to_bane_of_arthropods")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::SENSITIVE_TO_SMITE()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:sensitive_to_smite"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:sensitive_to_smite")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::SENSITIVE_TO_IMPALING()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:sensitive_to_impaling"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:sensitive_to_impaling")] = std::move(t);
    }
    return *tag;
}

// ========== 灾厄村民标签 ==========

EntityTypeTag& EntityTypeTags::ILLAGER()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:illager"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:illager")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::RAIDERS()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:raiders"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:raiders")] = std::move(t);
    }
    return *tag;
}

// ========== 环境标签 ==========

EntityTypeTag& EntityTypeTags::BURN_IN_DAYLIGHT()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:burn_in_daylight"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:burn_in_daylight")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::CAN_BREATHE_UNDER_WATER()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:can_breathe_under_water"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:can_breathe_under_water")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::FALL_DAMAGE_IMMUNE()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:fall_damage_immune"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:fall_damage_immune")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:freeze_immune_entity_types"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:freeze_immune_entity_types")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::FREEZE_HURTS_EXTRA_TYPES()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:freeze_hurts_extra_types"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:freeze_hurts_extra_types")] = std::move(t);
    }
    return *tag;
}

// ========== 其他标签 ==========

EntityTypeTag& EntityTypeTags::BEEHIVE_INHABITORS()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:beehive_inhabitors"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:beehive_inhabitors")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::DEFLECTS_PROJECTILES()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:deflects_projectiles"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:deflects_projectiles")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::IGNORES_POISON_AND_REGEN()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:ignores_poison_and_regen"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:ignores_poison_and_regen")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::INVERTED_HEALING_AND_HARM()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:inverted_healing_and_harm"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:inverted_healing_and_harm")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::IMMUNE_TO_INFESTED()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:immune_to_infested"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:immune_to_infested")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::IMMUNE_TO_OOZING()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:immune_to_oozing"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:immune_to_oozing")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::NO_ANGER_FROM_WIND_CHARGE()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:no_anger_from_wind_charge"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:no_anger_from_wind_charge")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::DISMOUNTS_UNDERWATER()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:dismounts_underwater"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:dismounts_underwater")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::POWDER_SNOW_WALKABLE_MOBS()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:powder_snow_walkable_mobs"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:powder_snow_walkable_mobs")] = std::move(t);
    }
    return *tag;
}

// ========== 铁傀儡赠花标签 ==========

EntityTypeTag& EntityTypeTags::ACCEPTS_IRON_GOLEM_GIFT()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:accepts_iron_golem_gift"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:accepts_iron_golem_gift")] = std::move(t);
    }
    return *tag;
}

EntityTypeTag& EntityTypeTags::CANDIDATE_FOR_IRON_GOLEM_GIFT()
{
    static EntityTypeTag* tag = nullptr;
    if (tag == nullptr) {
        auto t = std::make_unique<EntityTypeTag>(ResourceLocation("minecraft:candidate_for_iron_golem_gift"));
        tag = t.get();
        _getTags()[ResourceLocation("minecraft:candidate_for_iron_golem_gift")] = std::move(t);
    }
    return *tag;
}

// ============================================================================
// 初始化
// ============================================================================

void EntityTypeTags::initialize()
{
    if (s_initialized) {
        return;
    }

    // 确保所有标签已创建（惰性初始化）
    ARROWS();
    IMPACT_PROJECTILES();
    REDIRECTABLE_PROJECTILE();
    SKELETONS();
    ZOMBIES();
    UNDEAD();
    ARTHROPOD();
    AQUATIC();
    SENSITIVE_TO_BANE_OF_ARTHROPODS();
    SENSITIVE_TO_SMITE();
    SENSITIVE_TO_IMPALING();
    ILLAGER();
    RAIDERS();
    BURN_IN_DAYLIGHT();
    CAN_BREATHE_UNDER_WATER();
    FALL_DAMAGE_IMMUNE();
    FREEZE_IMMUNE_ENTITY_TYPES();
    FREEZE_HURTS_EXTRA_TYPES();
    BEEHIVE_INHABITORS();
    DEFLECTS_PROJECTILES();
    IGNORES_POISON_AND_REGEN();
    INVERTED_HEALING_AND_HARM();
    IMMUNE_TO_INFESTED();
    IMMUNE_TO_OOZING();
    NO_ANGER_FROM_WIND_CHARGE();
    DISMOUNTS_UNDERWATER();
    POWDER_SNOW_WALKABLE_MOBS();
    ACCEPTS_IRON_GOLEM_GIFT();
    CANDIDATE_FOR_IRON_GOLEM_GIFT();

    // ========== 硬编码默认标签成员 ==========
    // 这些默认值将在数据包加载时被覆盖或合并。

    // 箭矢标签
    ARROWS().addAll({
        ResourceLocation("minecraft:arrow"),
        ResourceLocation("minecraft:spectral_arrow"),
    });

    // 冲击投射物标签（包含 #minecraft:arrows 子标签）
    IMPACT_PROJECTILES().addAll({
        // #minecraft:arrows 子标签的成员（展开引用）
        ResourceLocation("minecraft:arrow"),
        ResourceLocation("minecraft:spectral_arrow"),
        // 直接成员
        ResourceLocation("minecraft:firework_rocket"),
        ResourceLocation("minecraft:snowball"),
        ResourceLocation("minecraft:fireball"),
        ResourceLocation("minecraft:small_fireball"),
        ResourceLocation("minecraft:egg"),
        ResourceLocation("minecraft:trident"),
        ResourceLocation("minecraft:dragon_fireball"),
        ResourceLocation("minecraft:wither_skull"),
        ResourceLocation("minecraft:wind_charge"),
        ResourceLocation("minecraft:breeze_wind_charge"),
    });

    // 可偏转投射物
    REDIRECTABLE_PROJECTILE().addAll({
        ResourceLocation("minecraft:fireball"),
        ResourceLocation("minecraft:wind_charge"),
        ResourceLocation("minecraft:breeze_wind_charge"),
    });

    // 骷髅类
    SKELETONS().addAll({
        ResourceLocation("minecraft:skeleton"),
        ResourceLocation("minecraft:stray"),
        ResourceLocation("minecraft:wither_skeleton"),
        ResourceLocation("minecraft:skeleton_horse"),
        ResourceLocation("minecraft:bogged"),
    });

    // 僵尸类
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.ZOMBIES（含 zombie_horse、zombie_nautilus）。
    // camel_husk 实体尚未注册（CamelHuskEntity 未实现），暂不加入（TODO：实体实现后补此成员）。
    ZOMBIES().addAll({
        ResourceLocation("minecraft:zombie_horse"),
        ResourceLocation("minecraft:zombie"),
        ResourceLocation("minecraft:zombie_villager"),
        ResourceLocation("minecraft:zombified_piglin"),
        ResourceLocation("minecraft:zoglin"),
        ResourceLocation("minecraft:drowned"),
        ResourceLocation("minecraft:husk"),
        ResourceLocation("minecraft:zombie_nautilus"),
    });

    // 亡灵（包含 #minecraft:skeletons 和 #minecraft:zombies 子标签）
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.UNDEAD（= SKELETONS + ZOMBIES + wither + phantom）。
    // parched 实体尚未注册（ParchedEntity 未实现），vanilla SKELETONS 含 parched，暂不加入
    // （TODO：实体实现后补入 SKELETONS 与此处）。
    UNDEAD().addAll({
        // #minecraft:skeletons 子标签成员
        ResourceLocation("minecraft:skeleton"),
        ResourceLocation("minecraft:stray"),
        ResourceLocation("minecraft:wither_skeleton"),
        ResourceLocation("minecraft:skeleton_horse"),
        ResourceLocation("minecraft:bogged"),
        // #minecraft:zombies 子标签成员
        ResourceLocation("minecraft:zombie_horse"),
        ResourceLocation("minecraft:zombie"),
        ResourceLocation("minecraft:zombie_villager"),
        ResourceLocation("minecraft:zombified_piglin"),
        ResourceLocation("minecraft:zoglin"),
        ResourceLocation("minecraft:drowned"),
        ResourceLocation("minecraft:husk"),
        ResourceLocation("minecraft:zombie_nautilus"),
        // 直接成员
        ResourceLocation("minecraft:wither"),
        ResourceLocation("minecraft:phantom"),
    });

    // 节肢动物
    ARTHROPOD().addAll({
        ResourceLocation("minecraft:bee"),
        ResourceLocation("minecraft:endermite"),
        ResourceLocation("minecraft:silverfish"),
        ResourceLocation("minecraft:spider"),
        ResourceLocation("minecraft:cave_spider"),
    });

    // 水生生物
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.AQUATIC（line 121-135）：
    //   turtle, axolotl, guardian, elder_guardian, cod, pufferfish, salmon, tropical_fish, dolphin,
    //   squid, glow_squid, tadpole, nautilus, zombie_nautilus。
    // tadpole 实体虽尚未注册，但标签为 typeId 字符串集，含未注册 typeId 无运行时副作用，
    // 完整对齐 vanilla 数据，实体注册后标签自动正确。
    // 运行时 SENSITIVE_TO_IMPALING 派生本标签（穿刺附魔查）。
    AQUATIC().addAll({
        ResourceLocation("minecraft:turtle"),
        ResourceLocation("minecraft:axolotl"),
        ResourceLocation("minecraft:guardian"),
        ResourceLocation("minecraft:elder_guardian"),
        ResourceLocation("minecraft:cod"),
        ResourceLocation("minecraft:pufferfish"),
        ResourceLocation("minecraft:salmon"),
        ResourceLocation("minecraft:tropical_fish"),
        ResourceLocation("minecraft:dolphin"),
        ResourceLocation("minecraft:squid"),
        ResourceLocation("minecraft:glow_squid"),
        ResourceLocation("minecraft:tadpole"),
        ResourceLocation("minecraft:nautilus"),
        ResourceLocation("minecraft:zombie_nautilus"),
    });

    // 附魔敏感标签（引用其他标签）
    SENSITIVE_TO_BANE_OF_ARTHROPODS().addAll(
        std::vector<ResourceLocation>(ARTHROPOD().getEntityTypeIds().begin(), ARTHROPOD().getEntityTypeIds().end()));
    SENSITIVE_TO_SMITE().addAll(
        std::vector<ResourceLocation>(UNDEAD().getEntityTypeIds().begin(), UNDEAD().getEntityTypeIds().end()));
    SENSITIVE_TO_IMPALING().addAll(
        std::vector<ResourceLocation>(AQUATIC().getEntityTypeIds().begin(), AQUATIC().getEntityTypeIds().end()));

    // 灾厄村民
    ILLAGER().addAll({
        ResourceLocation("minecraft:evoker"),
        ResourceLocation("minecraft:illusioner"),
        ResourceLocation("minecraft:pillager"),
        ResourceLocation("minecraft:vindicator"),
    });

    // 袭击者
    RAIDERS().addAll({
        ResourceLocation("minecraft:evoker"),
        ResourceLocation("minecraft:pillager"),
        ResourceLocation("minecraft:ravager"),
        ResourceLocation("minecraft:vindicator"),
        ResourceLocation("minecraft:illusioner"),
        ResourceLocation("minecraft:witch"),
    });

    // 白天燃烧
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.BURN_IN_DAYLIGHT：
    //   skeleton, stray, wither_skeleton, bogged, zombie, zombie_horse, zombie_villager,
    //   drowned, zombie_nautilus, phantom。
    // 此前漏 zombie_horse 与 zombie_nautilus（两者已注册，属亡灵且 vanilla 在本标签中）。
    // 注意：本标签当前未在运行时查询（Cubium 用 shouldBurnInDaylight() 虚函数 + 各实体 tick 直接
    // 调 burnUndead 实现日光燃烧，非查标签），此处仅作数据对齐；未来若迁移到标签查询将直接正确。
    BURN_IN_DAYLIGHT().addAll({
        ResourceLocation("minecraft:skeleton"),
        ResourceLocation("minecraft:stray"),
        ResourceLocation("minecraft:wither_skeleton"),
        ResourceLocation("minecraft:bogged"),
        ResourceLocation("minecraft:zombie"),
        ResourceLocation("minecraft:zombie_horse"),
        ResourceLocation("minecraft:zombie_villager"),
        ResourceLocation("minecraft:drowned"),
        ResourceLocation("minecraft:zombie_nautilus"),
        ResourceLocation("minecraft:phantom"),
    });

    // 可水下呼吸
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.CAN_BREATHE_UNDER_WATER：
    //   #undead（亡灵不需要呼吸）+ axolotl, frog, guardian, elder_guardian, turtle, glow_squid,
    //   cod, pufferfish, salmon, squid, tropical_fish, tadpole, armor_stand, copper_golem, nautilus。
    // 此前偏差：误加 dolphin（vanilla 海豚需浮出水面呼吸，不在本标签）；漏 cod/pufferfish/salmon/
    //   tropical_fish/armor_stand/copper_golem/nautilus。
    // frog/tadpole 实体虽尚未注册，但标签为 typeId 字符串集，含未注册 typeId 无运行时副作用，
    // 完整对齐 vanilla 数据，实体注册后标签自动正确。
    // 注意：本标签当前未在运行时查询（Cubium 用 canBreatheUnderwater() 虚函数实现水下呼吸），
    // 此处仅作数据对齐。
    CAN_BREATHE_UNDER_WATER().addAll({
        // #minecraft:undead 子标签成员（亡灵不需要呼吸）
        ResourceLocation("minecraft:skeleton"),
        ResourceLocation("minecraft:stray"),
        ResourceLocation("minecraft:wither_skeleton"),
        ResourceLocation("minecraft:skeleton_horse"),
        ResourceLocation("minecraft:bogged"),
        ResourceLocation("minecraft:zombie_horse"),
        ResourceLocation("minecraft:zombie"),
        ResourceLocation("minecraft:zombie_villager"),
        ResourceLocation("minecraft:zombified_piglin"),
        ResourceLocation("minecraft:zoglin"),
        ResourceLocation("minecraft:drowned"),
        ResourceLocation("minecraft:husk"),
        ResourceLocation("minecraft:zombie_nautilus"),
        ResourceLocation("minecraft:wither"),
        ResourceLocation("minecraft:phantom"),
        // 水生及其他可水下呼吸生物
        ResourceLocation("minecraft:axolotl"),
        ResourceLocation("minecraft:frog"),
        ResourceLocation("minecraft:guardian"),
        ResourceLocation("minecraft:elder_guardian"),
        ResourceLocation("minecraft:turtle"),
        ResourceLocation("minecraft:glow_squid"),
        ResourceLocation("minecraft:cod"),
        ResourceLocation("minecraft:pufferfish"),
        ResourceLocation("minecraft:salmon"),
        ResourceLocation("minecraft:squid"),
        ResourceLocation("minecraft:tropical_fish"),
        ResourceLocation("minecraft:tadpole"),
        ResourceLocation("minecraft:armor_stand"),
        ResourceLocation("minecraft:copper_golem"),
        ResourceLocation("minecraft:nautilus"),
    });

    // 摔落伤害免疫
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.FALL_DAMAGE_IMMUNE：
    //   copper_golem, iron_golem, snow_golem, shulker, allay, bat, bee, blaze, cat, chicken, ghast,
    //   happy_ghast, phantom, magma_cube, ocelot, parrot, wither, breeze。
    // 此前偏差：漏 copper_golem/magma_cube/ocelot/breeze。
    // allay/happy_ghast 实体虽尚未注册，但标签为 typeId 字符串集，含未注册 typeId 无运行时副作用，
    // 完整对齐 vanilla 数据，实体注册后标签自动正确。
    // 注意：本标签当前未在运行时查询（Cubium 摔落伤害免疫由各实体类 causeFallDamage override
    // 或 isFallBlocking 等机制实现），此处仅作数据对齐。
    FALL_DAMAGE_IMMUNE().addAll({
        ResourceLocation("minecraft:copper_golem"),
        ResourceLocation("minecraft:iron_golem"),
        ResourceLocation("minecraft:snow_golem"),
        ResourceLocation("minecraft:shulker"),
        ResourceLocation("minecraft:allay"),
        ResourceLocation("minecraft:bat"),
        ResourceLocation("minecraft:bee"),
        ResourceLocation("minecraft:blaze"),
        ResourceLocation("minecraft:cat"),
        ResourceLocation("minecraft:chicken"),
        ResourceLocation("minecraft:ghast"),
        ResourceLocation("minecraft:happy_ghast"),
        ResourceLocation("minecraft:phantom"),
        ResourceLocation("minecraft:magma_cube"),
        ResourceLocation("minecraft:ocelot"),
        ResourceLocation("minecraft:parrot"),
        ResourceLocation("minecraft:wither"),
        ResourceLocation("minecraft:breeze"),
    });

    // 冻结免疫
    FREEZE_IMMUNE_ENTITY_TYPES().addAll({
        ResourceLocation("minecraft:stray"),
        ResourceLocation("minecraft:polar_bear"),
        ResourceLocation("minecraft:snow_golem"),
        ResourceLocation("minecraft:wither"),
    });

    // 冻结额外伤害
    FREEZE_HURTS_EXTRA_TYPES().addAll({
        ResourceLocation("minecraft:blaze"),
        ResourceLocation("minecraft:magma_cube"),
        ResourceLocation("minecraft:strider"),
    });

    // 蜂巢居民
    BEEHIVE_INHABITORS().addAll({
        ResourceLocation("minecraft:bee"),
    });

    // 偏转投射物
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.DEFLECTS_PROJECTILES（仅 breeze）。
    // 此前误将 shulker 加入本标签（shulker 在 vanilla 不偏转投射物，vanilla Shulker 无 deflection
    // override，基类 Entity.deflection 查本标签，shulker 不在标签故返 None）。运行时 Entity::deflection
    // （Entity.cpp:1830）查本标签，误加 shulker 致箭矢等投射物命中潜影贝时被偏转弹开而非造成伤害，
    // 偏离 vanilla（vanilla 潜影贝正常受投射物伤害）。已移除 shulker 修正。
    DEFLECTS_PROJECTILES().addAll({
        ResourceLocation("minecraft:breeze"),
    });

    // 忽略中毒和再生（= #minecraft:undead）
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.IGNORES_POISON_AND_REGEN（line 142 addTag(UNDEAD)）：
    //   仅亡灵免疫 Poison/Regen，无其他成员。
    // 此前偏差：手动列举 13 亡灵（漏 zombie_horse/zombie_nautilus）+ 误加 iron_golem。
    //   iron_golem 在 vanilla 不在本标签（vanilla 铁傀儡受中毒效果），误加致 Cubium 铁傀儡免疫中毒
    //   偏离 vanilla。改为 addAll(UNDEAD) 派生（自动含全部亡灵含 zombie_horse/zombie_nautilus）并移除
    //   iron_golem，与 INVERTED_HEALING_AND_HARM 同款 addTag(UNDEAD) 写法。
    // 运行时 LivingEntity::isPotionApplicable（LivingEntity.cpp:2618）查本标签免疫 Poison/Regen。
    IGNORES_POISON_AND_REGEN().addAll(
        std::vector<ResourceLocation>(UNDEAD().getEntityTypeIds().begin(), UNDEAD().getEntityTypeIds().end()));

    // 治疗与伤害反转（= #minecraft:undead）
    INVERTED_HEALING_AND_HARM().addAll(
        std::vector<ResourceLocation>(UNDEAD().getEntityTypeIds().begin(), UNDEAD().getEntityTypeIds().end()));

    // 免疫蠹虫效果（infested 附魔：被击杀生物生成蠹虫；蠹虫自身免疫防递归）
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.IMMUNE_TO_INFESTED（line 178，仅 silverfish）。
    // 此前误加 spider/cave_spider/endermite（vanilla 仅 silverfish 免疫，其他节肢不免疫）。
    // 本标签当前未在运行时查询（Infested 效果未实现，LivingEntity.cpp:2634 TODO），仅作数据对齐。
    IMMUNE_TO_INFESTED().addAll({
        ResourceLocation("minecraft:silverfish"),
    });

    // 免疫渗出效果（oozing 附魔：被击杀生物生成史莱姆；史莱姆自身免疫防递归）
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.IMMUNE_TO_OOZING（line 179，仅 slime）。
    // 此前误加 magma_cube（vanilla 仅 slime 免疫，岩浆怪不免疫）。
    // 本标签当前未在运行时查询（Oozing 效果未实现，LivingEntity.cpp:2634 TODO），仅作数据对齐。
    IMMUNE_TO_OOZING().addAll({
        ResourceLocation("minecraft:slime"),
    });

    // 风弹不激怒
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.NO_ANGER_FROM_WIND_CHARGE（line 166-177）：
    //   breeze, skeleton, bogged, stray, zombie, husk, spider, cave_spider, slime。
    // 此前仅 breeze，漏其余 8 成员。本标签当前未在运行时查询（风弹激怒逻辑未实现），仅作数据对齐。
    NO_ANGER_FROM_WIND_CHARGE().addAll({
        ResourceLocation("minecraft:breeze"),
        ResourceLocation("minecraft:skeleton"),
        ResourceLocation("minecraft:bogged"),
        ResourceLocation("minecraft:stray"),
        ResourceLocation("minecraft:zombie"),
        ResourceLocation("minecraft:husk"),
        ResourceLocation("minecraft:spider"),
        ResourceLocation("minecraft:cave_spider"),
        ResourceLocation("minecraft:slime"),
    });

    // 水下强制下坐骑
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.DISMOUNTS_UNDERWATER（line 103-118）：
    //   camel, chicken, donkey, happy_ghast, horse, llama, mule, pig, ravager, spider, strider,
    //   trader_llama, zombie_horse（船不在其中，船有自己的水下沉没逻辑）。
    // camel/happy_ghast 实体虽尚未注册，但标签为 typeId 字符串集，含未注册 typeId 无运行时副作用
    // （查询时无该实体命中），完整对齐 vanilla 数据，实体注册后标签自动正确。
    // 运行时 Entity::ridesCanMountDismountInWater（Entity.cpp:1822）查本标签判断载具水中强制下坐骑。
    DISMOUNTS_UNDERWATER().addAll({
        ResourceLocation("minecraft:camel"),
        ResourceLocation("minecraft:chicken"),
        ResourceLocation("minecraft:donkey"),
        ResourceLocation("minecraft:happy_ghast"),
        ResourceLocation("minecraft:horse"),
        ResourceLocation("minecraft:llama"),
        ResourceLocation("minecraft:mule"),
        ResourceLocation("minecraft:pig"),
        ResourceLocation("minecraft:ravager"),
        ResourceLocation("minecraft:spider"),
        ResourceLocation("minecraft:strider"),
        ResourceLocation("minecraft:trader_llama"),
        ResourceLocation("minecraft:zombie_horse"),
    });

    // 细雪可行走
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.POWDER_SNOW_WALKABLE_MOBS（line 54）：
    //   rabbit, endermite, silverfish, fox。
    // 此前误为 rabbit/fox/ocelot/cat（多 ocelot/cat，漏 endermite/silverfish）。
    // 本标签当前未在运行时查询，仅作数据对齐。
    POWDER_SNOW_WALKABLE_MOBS().addAll({
        ResourceLocation("minecraft:rabbit"),
        ResourceLocation("minecraft:endermite"),
        ResourceLocation("minecraft:silverfish"),
        ResourceLocation("minecraft:fox"),
    });

    // 铁傀儡赠花标签（运行时使用，此前漏填致赠花链路失效）
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider：
    //   ACCEPTS_IRON_GOLEM_GIFT（line 252）= copper_golem（铜傀儡可接受铁傀儡赠予的罂粟花）。
    //   CANDIDATE_FOR_IRON_GOLEM_GIFT（line 253）= villager + #accepts_iron_golem_gift
    //     （铁傀儡 OfferFlowerGoal 候选目标：村民与铜傀儡）。
    // 此前 initialize() 未给两标签填充成员（空标签），运行时 IronGolemGoals::_findNearestCandidate
    //   （IronGolemGoals.cpp:332）用空 CANDIDATE 标签过滤找不到候选、_tryGiftFlowerToCopperGolem
    //   （IronGolemGoals.cpp:394）用空 ACCEPTS 标签检查铜傀儡不在标签→铁傀儡永不赠花给铜傀儡。
    //   单元测试 OfferFlowerGiftTest::SetUpTestSuite 手动 addAll 填充掩盖了此生产缺陷。
    //   现于 initialize() 填充，生产路径赠花链路恢复正确（addAll 为 set 追加，单元测试重复 addAll 幂等）。
    ACCEPTS_IRON_GOLEM_GIFT().addAll({
        ResourceLocation("minecraft:copper_golem"),
    });
    CANDIDATE_FOR_IRON_GOLEM_GIFT().addAll({
        ResourceLocation("minecraft:villager"),
        // #minecraft:accepts_iron_golem_gift 子标签成员（= copper_golem）
        ResourceLocation("minecraft:copper_golem"),
    });

    s_initialized = true;
}

EntityTypeTag* EntityTypeTags::getTag(const ResourceLocation& id)
{
    auto& tags = _getTags();
    auto it = tags.find(id);
    return it != tags.end() ? it->second.get() : nullptr;
}

EntityTypeTag& EntityTypeTags::registerTag(const ResourceLocation& id)
{
    auto& tags = _getTags();
    auto it = tags.find(id);
    if (it != tags.end()) {
        return *it->second;
    }
    auto tag = std::make_unique<EntityTypeTag>(id);
    auto& ref = *tag;
    tags[id] = std::move(tag);
    return ref;
}

void EntityTypeTags::forEachTag(std::function<void(EntityTypeTag&)> callback)
{
    for (auto& [id, tag] : _getTags()) {
        callback(*tag);
    }
}

} // namespace mc
