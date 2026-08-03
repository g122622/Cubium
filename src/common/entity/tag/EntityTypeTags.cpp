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
    ZOMBIES().addAll({
        ResourceLocation("minecraft:zombie"),
        ResourceLocation("minecraft:zombie_villager"),
        ResourceLocation("minecraft:zombified_piglin"),
        ResourceLocation("minecraft:zoglin"),
        ResourceLocation("minecraft:drowned"),
        ResourceLocation("minecraft:husk"),
    });

    // 亡灵（包含 #minecraft:skeletons 和 #minecraft:zombies 子标签）
    UNDEAD().addAll({
        // #minecraft:skeletons 子标签成员
        ResourceLocation("minecraft:skeleton"),
        ResourceLocation("minecraft:stray"),
        ResourceLocation("minecraft:wither_skeleton"),
        ResourceLocation("minecraft:skeleton_horse"),
        ResourceLocation("minecraft:bogged"),
        // #minecraft:zombies 子标签成员
        ResourceLocation("minecraft:zombie"),
        ResourceLocation("minecraft:zombie_villager"),
        ResourceLocation("minecraft:zombified_piglin"),
        ResourceLocation("minecraft:zoglin"),
        ResourceLocation("minecraft:drowned"),
        ResourceLocation("minecraft:husk"),
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
    BURN_IN_DAYLIGHT().addAll({
        ResourceLocation("minecraft:skeleton"),
        ResourceLocation("minecraft:stray"),
        ResourceLocation("minecraft:wither_skeleton"),
        ResourceLocation("minecraft:bogged"),
        ResourceLocation("minecraft:zombie"),
        ResourceLocation("minecraft:zombie_villager"),
        ResourceLocation("minecraft:husk"),
        ResourceLocation("minecraft:drowned"),
        ResourceLocation("minecraft:phantom"),
    });

    // 可水下呼吸
    CAN_BREATHE_UNDER_WATER().addAll({
        // #minecraft:undead 子标签成员（亡灵不需要呼吸）
        ResourceLocation("minecraft:skeleton"),
        ResourceLocation("minecraft:stray"),
        ResourceLocation("minecraft:wither_skeleton"),
        ResourceLocation("minecraft:skeleton_horse"),
        ResourceLocation("minecraft:bogged"),
        ResourceLocation("minecraft:zombie"),
        ResourceLocation("minecraft:zombie_villager"),
        ResourceLocation("minecraft:zombified_piglin"),
        ResourceLocation("minecraft:zoglin"),
        ResourceLocation("minecraft:drowned"),
        ResourceLocation("minecraft:husk"),
        ResourceLocation("minecraft:wither"),
        ResourceLocation("minecraft:phantom"),
        // 水生生物
        ResourceLocation("minecraft:axolotl"),
        ResourceLocation("minecraft:frog"),
        ResourceLocation("minecraft:guardian"),
        ResourceLocation("minecraft:elder_guardian"),
        ResourceLocation("minecraft:turtle"),
        ResourceLocation("minecraft:dolphin"),
        ResourceLocation("minecraft:glow_squid"),
        ResourceLocation("minecraft:squid"),
    });

    // 摔落伤害免疫
    FALL_DAMAGE_IMMUNE().addAll({
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
        ResourceLocation("minecraft:phantom"),
        ResourceLocation("minecraft:wither"),
        ResourceLocation("minecraft:parrot"),
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
    DEFLECTS_PROJECTILES().addAll({
        ResourceLocation("minecraft:shulker"),
        ResourceLocation("minecraft:breeze"),
    });

    // 忽略中毒和再生
    IGNORES_POISON_AND_REGEN().addAll({
        // 亡灵
        ResourceLocation("minecraft:skeleton"),
        ResourceLocation("minecraft:stray"),
        ResourceLocation("minecraft:wither_skeleton"),
        ResourceLocation("minecraft:bogged"),
        ResourceLocation("minecraft:zombie"),
        ResourceLocation("minecraft:zombie_villager"),
        ResourceLocation("minecraft:zombified_piglin"),
        ResourceLocation("minecraft:zoglin"),
        ResourceLocation("minecraft:drowned"),
        ResourceLocation("minecraft:husk"),
        ResourceLocation("minecraft:wither"),
        ResourceLocation("minecraft:phantom"),
        // 其他
        ResourceLocation("minecraft:iron_golem"),
    });

    // 治疗与伤害反转（= #minecraft:undead）
    INVERTED_HEALING_AND_HARM().addAll(
        std::vector<ResourceLocation>(UNDEAD().getEntityTypeIds().begin(), UNDEAD().getEntityTypeIds().end()));

    // 免疫蠹虫效果
    IMMUNE_TO_INFESTED().addAll({
        ResourceLocation("minecraft:spider"),
        ResourceLocation("minecraft:cave_spider"),
        ResourceLocation("minecraft:endermite"),
        ResourceLocation("minecraft:silverfish"),
    });

    // 免疫渗出效果
    IMMUNE_TO_OOZING().addAll({
        ResourceLocation("minecraft:slime"),
        ResourceLocation("minecraft:magma_cube"),
    });

    // 风弹不激怒
    NO_ANGER_FROM_WIND_CHARGE().addAll({
        ResourceLocation("minecraft:breeze"),
    });

    // 水下强制下坐骑
    // 这些实体在水中会强制乘客下坐骑（船不在其中，船有自己的水下沉没逻辑）
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
    POWDER_SNOW_WALKABLE_MOBS().addAll({
        ResourceLocation("minecraft:rabbit"),
        ResourceLocation("minecraft:fox"),
        ResourceLocation("minecraft:ocelot"),
        ResourceLocation("minecraft:cat"),
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
