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

#pragma once

#include "EntityTypeTag.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

namespace mc {

/**
 * @brief 内置实体类型标签集合
 *
 * 提供所有 MC Java 版定义的实体类型标签的静态访问方法。
 * 参考: net.minecraft.tags.EntityTypeTags
 *
 * 用法示例:
 * @code
 * if (EntityTypeTags::IMPACT_PROJECTILES().contains(entity.getTypeId())) {
 *     // 该实体属于冲击投射物
 * }
 * @endcode
 */
class EntityTypeTags {
public:
    // ========== 投射物标签 ==========

    /// 箭矢标签（普通箭矢、光灵箭）
    /// 参考: minecraft:arrows
    static EntityTypeTag& ARROWS();

    /// 冲击投射物标签（箭矢、三叉戟、火球等，可破坏陶罐等方块）
    /// 参考: minecraft:impact_projectiles
    /// 包含 #minecraft:arrows 子标签
    static EntityTypeTag& IMPACT_PROJECTILES();

    /// 可偏转投射物标签（火球、风弹等，可被玩家攻击偏转）
    /// 参考: minecraft:redirectable_projectile
    static EntityTypeTag& REDIRECTABLE_PROJECTILE();

    // ========== 亡灵/节肢/水生标签 ==========

    /// 骷髅类标签（骷髅、流浪者、凋灵骷髅、骷髅马、沼骸）
    /// 参考: minecraft:skeletons
    static EntityTypeTag& SKELETONS();

    /// 僵尸类标签（僵尸、僵尸村民、僵尸化猪灵、尸壳、溺尸等）
    /// 参考: minecraft:zombies
    static EntityTypeTag& ZOMBIES();

    /// 亡灵标签（骷髅类 + 僵尸类 + 凋灵 + 幻翼等）
    /// 参考: minecraft:undead
    /// 包含 #minecraft:skeletons 和 #minecraft:zombies 子标签
    static EntityTypeTag& UNDEAD();

    /// 节肢动物标签（蜜蜂、蠹虫、蜘蛛、洞穴蜘蛛）
    /// 参考: minecraft:arthropod
    static EntityTypeTag& ARTHROPOD();

    /// 水生生物标签（海龟、美西螈、守卫者、鱼等）
    /// 参考: minecraft:aquatic
    static EntityTypeTag& AQUATIC();

    // ========== 附魔敏感标签 ==========

    /// 节肢杀手敏感标签（= #minecraft:arthropod）
    /// 参考: minecraft:sensitive_to_bane_of_arthropods
    static EntityTypeTag& SENSITIVE_TO_BANE_OF_ARTHROPODS();

    /// 亡灵杀手敏感标签（= #minecraft:undead）
    /// 参考: minecraft:sensitive_to_smite
    static EntityTypeTag& SENSITIVE_TO_SMITE();

    /// 穿刺附魔敏感标签（= #minecraft:aquatic）
    /// 参考: minecraft:sensitive_to_impaling
    static EntityTypeTag& SENSITIVE_TO_IMPALING();

    // ========== 灾厄村民标签 ==========

    /// 灾厄村民标签（唤魔者、幻术师、掠夺者、卫道士）
    /// 参考: minecraft:illager
    static EntityTypeTag& ILLAGER();

    /// 袭击者标签（唤魔者、掠夺者、劫掠兽、卫道士、幻术师、女巫）
    /// 参考: minecraft:raiders
    static EntityTypeTag& RAIDERS();

    // ========== 环境标签 ==========

    /// 白天燃烧标签（骷髅、流浪者、僵尸等）
    /// 参考: minecraft:burn_in_daylight
    static EntityTypeTag& BURN_IN_DAYLIGHT();

    /// 可水下呼吸标签（亡灵 + 美西螈 + 青蛙 + 守卫者等）
    /// 参考: minecraft:can_breathe_under_water
    static EntityTypeTag& CAN_BREATHE_UNDER_WATER();

    /// 摔落伤害免疫标签（铁傀儡、雪傀儡、潜影贝等）
    /// 参考: minecraft:fall_damage_immune
    static EntityTypeTag& FALL_DAMAGE_IMMUNE();

    /// 冻结免疫标签（流浪者、北极熊、雪傀儡、凋灵）
    /// 参考: minecraft:freeze_immune_entity_types
    static EntityTypeTag& FREEZE_IMMUNE_ENTITY_TYPES();

    /// 冻结额外伤害标签
    /// 参考: minecraft:freeze_hurts_extra_types
    static EntityTypeTag& FREEZE_HURTS_EXTRA_TYPES();

    // ========== 其他标签 ==========

    /// 蜂巢居民标签（蜜蜂）
    /// 参考: minecraft:beehive_inhabitors
    static EntityTypeTag& BEEHIVE_INHABITORS();

    /// 可偏转投射物的实体标签（潜影贝、守卫者等）
    /// 参考: minecraft:deflects_projectiles
    static EntityTypeTag& DEFLECTS_PROJECTILES();

    /// 忽略中毒和再生效果标签（亡灵等）
    /// 参考: minecraft:ignores_poison_and_regen
    static EntityTypeTag& IGNORES_POISON_AND_REGEN();

    /// 治疗与伤害反转标签（亡灵等）
    /// 参考: minecraft:inverted_healing_and_harm
    static EntityTypeTag& INVERTED_HEALING_AND_HARM();

    /// 免疫蠹虫效果标签
    /// 参考: minecraft:immune_to_infested
    static EntityTypeTag& IMMUNE_TO_INFESTED();

    /// 免疫渗出效果标签
    /// 参考: minecraft:immune_to_oozing
    static EntityTypeTag& IMMUNE_TO_OOZING();

    /// 风弹不激怒标签
    /// 参考: minecraft:no_anger_from_wind_charge
    static EntityTypeTag& NO_ANGER_FROM_WIND_CHARGE();

    /// 水下强制下坐骑标签（骆驼、鸡、驴、马、羊驼、骡、猪、劫掠兽、蜘蛛、炽足兽、行商羊驼、僵尸马等）
    /// 参考: minecraft:dismounts_underwater
    /// 当载具实体类型属于此标签时，乘客在水中会被强制下坐骑
    static EntityTypeTag& DISMOUNTS_UNDERWATER();

    /// 细雪可行走标签
    /// 参考: minecraft:powder_snow_walkable_mobs
    static EntityTypeTag& POWDER_SNOW_WALKABLE_MOBS();

    /**
     * @brief 初始化所有内置标签
     *
     * 在 EntityRegistry::initializeAll() 之后调用。
     * 数据包加载（EntityTypeTagLoader）应在 initialize() 之后调用。
     */
    static void initialize();

    /**
     * @brief 根据ID获取标签
     * @param id 标签资源位置
     * @return 标签指针，如果不存在返回 nullptr
     */
    [[nodiscard]] static EntityTypeTag* getTag(const ResourceLocation& id);

    /**
     * @brief 注册一个空标签（数据包加载时使用）
     * @param id 标签资源位置
     * @return 注册的标签引用
     */
    static EntityTypeTag& registerTag(const ResourceLocation& id);

    /**
     * @brief 遍历所有标签
     */
    static void forEachTag(std::function<void(EntityTypeTag&)> callback);

private:
    EntityTypeTags() = delete;

    static std::unordered_map<ResourceLocation, std::unique_ptr<EntityTypeTag>>& _getTags();
    static bool s_initialized;
};

} // namespace mc
