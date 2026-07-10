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
 * 提供所有已定义的实体类型标签的静态访问方法。
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
    static EntityTypeTag& ARROWS();

    /// 冲击投射物标签（箭矢、三叉戟、火球等，可破坏陶罐等方块）
    /// 包含 #minecraft:arrows 子标签
    static EntityTypeTag& IMPACT_PROJECTILES();

    /// 可偏转投射物标签（火球、风弹等，可被玩家攻击偏转）
    static EntityTypeTag& REDIRECTABLE_PROJECTILE();

    // ========== 亡灵/节肢/水生标签 ==========

    /// 骷髅类标签（骷髅、流浪者、凋灵骷髅、骷髅马、沼骸）
    static EntityTypeTag& SKELETONS();

    /// 僵尸类标签（僵尸、僵尸村民、僵尸化猪灵、尸壳、溺尸等）
    static EntityTypeTag& ZOMBIES();

    /// 亡灵标签（骷髅类 + 僵尸类 + 凋灵 + 幻翼等）
    /// 包含 #minecraft:skeletons 和 #minecraft:zombies 子标签
    static EntityTypeTag& UNDEAD();

    /// 节肢动物标签（蜜蜂、蠹虫、蜘蛛、洞穴蜘蛛）
    static EntityTypeTag& ARTHROPOD();

    /// 水生生物标签（海龟、美西螈、守卫者、鱼等）
    static EntityTypeTag& AQUATIC();

    // ========== 附魔敏感标签 ==========

    /// 节肢杀手敏感标签（= #minecraft:arthropod）
    static EntityTypeTag& SENSITIVE_TO_BANE_OF_ARTHROPODS();

    /// 亡灵杀手敏感标签（= #minecraft:undead）
    static EntityTypeTag& SENSITIVE_TO_SMITE();

    /// 穿刺附魔敏感标签（= #minecraft:aquatic）
    static EntityTypeTag& SENSITIVE_TO_IMPALING();

    // ========== 灾厄村民标签 ==========

    /// 灾厄村民标签（唤魔者、幻术师、掠夺者、卫道士）
    static EntityTypeTag& ILLAGER();

    /// 袭击者标签（唤魔者、掠夺者、劫掠兽、卫道士、幻术师、女巫）
    static EntityTypeTag& RAIDERS();

    // ========== 环境标签 ==========

    /// 白天燃烧标签（骷髅、流浪者、僵尸等）
    static EntityTypeTag& BURN_IN_DAYLIGHT();

    /// 可水下呼吸标签（亡灵 + 美西螈 + 青蛙 + 守卫者等）
    static EntityTypeTag& CAN_BREATHE_UNDER_WATER();

    /// 摔落伤害免疫标签（铁傀儡、雪傀儡、潜影贝等）
    static EntityTypeTag& FALL_DAMAGE_IMMUNE();

    /// 冻结免疫标签（流浪者、北极熊、雪傀儡、凋灵）
    static EntityTypeTag& FREEZE_IMMUNE_ENTITY_TYPES();

    /// 冻结额外伤害标签（烈焰人、岩浆怪、炽足兽）
    static EntityTypeTag& FREEZE_HURTS_EXTRA_TYPES();

    // ========== 其他标签 ==========

    /// 蜂巢居民标签（蜜蜂）
    static EntityTypeTag& BEEHIVE_INHABITORS();

    /// 可偏转投射物的实体标签（潜影贝、守卫者等）
    static EntityTypeTag& DEFLECTS_PROJECTILES();

    /// 忽略中毒和再生效果标签（亡灵等）
    static EntityTypeTag& IGNORES_POISON_AND_REGEN();

    /// 治疗与伤害反转标签（亡灵等）
    static EntityTypeTag& INVERTED_HEALING_AND_HARM();

    /// 免疫蠹虫效果标签
    static EntityTypeTag& IMMUNE_TO_INFESTED();

    /// 免疫渗出效果标签
    static EntityTypeTag& IMMUNE_TO_OOZING();

    /// 风弹不激怒标签
    static EntityTypeTag& NO_ANGER_FROM_WIND_CHARGE();

    /// 水下强制下坐骑标签（骆驼、鸡、驴、马、羊驼、骡、猪、劫掠兽、蜘蛛、炽足兽、行商羊驼、僵尸马等）
    /// 当载具实体类型属于此标签时，乘客在水中会被强制下坐骑
    static EntityTypeTag& DISMOUNTS_UNDERWATER();

    /// 细雪可行走标签
    static EntityTypeTag& POWDER_SNOW_WALKABLE_MOBS();

    // ========== 铁傀儡赠花标签 ==========

    /// 接受铁傀儡礼物的实体标签（铜傀儡）
    /// 当铁傀儡 OfferFlowerGoal 自然结束时，若目标实体属于此标签，
    /// 会将罂粟花装备到其天线槽（EquipmentSlot::Saddle）并标记为保整掉落。
    /// 对应 MC 1.21.11 标签 minecraft:accepts_iron_golem_gift。
    static EntityTypeTag& ACCEPTS_IRON_GOLEM_GIFT();

    /// 铁傀儡赠花候选实体标签（村民 + #minecraft:accepts_iron_golem_gift）
    /// 铁傀儡 OfferFlowerGoal 在白天以 1/8000 概率搜索此标签内的最近实体作为赠花目标。
    /// 对应 MC 1.21.11 标签 minecraft:candidate_for_iron_golem_gift。
    static EntityTypeTag& CANDIDATE_FOR_IRON_GOLEM_GIFT();

    /**
     * @brief 初始化所有内置标签
     *
     * 在 EntityRegistry::initializeAll() 之后调用。
     * 数据包加载（EntityTypeTagLoader）应在 initialize() 之后调用。
     */
    static void initialize();

    /**
     * @brief 检查标签系统是否已初始化
     * 在 canFreeze() 等方法中用于安全检查，避免在初始化前访问标签数据。
     */
    [[nodiscard]] static bool isInitialized() { return s_initialized; }

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
