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

#include "DamageTypeTag.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <functional>
#include <memory>
#include <unordered_map>

namespace mc {

/**
 * @brief 内置伤害类型标签集合
 *
 * 提供所有 MC 1.21.11 已定义的伤害类型标签的静态访问方法。
 *
 * 用法示例:
 * @code
 * if (DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(source.type())) {
 *     // 该伤害绕过狼铠
 * }
 * // 或通过 DamageSource::is() 方法
 * if (source.is(DamageTypeTags::BYPASSES_WOLF_ARMOR())) {
 *     // 该伤害绕过狼铠
 * }
 * @endcode
 *
 * 参考: net.minecraft.tags.DamageTypeTags (MC 1.21.11)
 */
class DamageTypeTags {
public:
    // ========== 绕过防御类标签 ==========

    /// 绕过护甲标签
    /// 这些伤害类型无视护甲减免（不消耗护甲耐久，不触发护甲减免计算）
    /// 包含: on_fire, in_wall, cramming, drown, fly_into_wall, generic, wither,
    ///       dragon_breath, starve, fall, ender_pearl, freeze, stalagmite,
    ///       magic, indirect_magic, out_of_world, generic_kill, sonic_boom, outside_border
    static DamageTypeTag& BYPASSES_ARMOR();

    /// 绕过无敌标签
    /// 这些伤害类型无视无敌时间、创造模式无敌和无敌药水效果
    /// 包含: out_of_world, generic_kill
    static DamageTypeTag& BYPASSES_INVULNERABILITY();

    /// 绕过抗性提升标签
    /// 这些伤害类型无视抗性提升药水效果
    /// 包含: out_of_world, generic_kill
    static DamageTypeTag& BYPASSES_RESISTANCE();

    /// 绕过盾牌标签
    /// 这些伤害类型无法被盾牌格挡
    /// 包含: #bypasses_armor + cactus, campfire, dry_out, falling_anvil,
    ///       falling_stalactite, hot_floor, in_fire, lava, lightning_bolt, sweet_berry_bush
    static DamageTypeTag& BYPASSES_SHIELD();

    /// 绕过药水效果标签
    /// 这些伤害类型不受药水效果影响（如抗性提升）
    /// 包含: starve
    static DamageTypeTag& BYPASSES_EFFECTS();

    /// 绕过附魔标签
    /// 这些伤害类型不受保护附魔影响
    /// 包含: sonic_boom
    static DamageTypeTag& BYPASSES_ENCHANTMENTS();

    /// 绕过狼铠标签
    /// 这些伤害类型无视狼铠的吸收效果
    /// 包含: #bypasses_invulnerability + cramming, drown, dry_out, freeze, in_wall,
    ///       indirect_magic, magic, outside_border, starve, thorns, wither
    /// 运行时消费场景：
    /// 1. WolfEntity::_canArmorAbsorb — 判断狼铠是否能吸收该伤害
    static DamageTypeTag& BYPASSES_WOLF_ARMOR();

    // ========== 伤害分类标签 ==========

    /// 溺水伤害标签
    /// 包含: drown
    static DamageTypeTag& IS_DROWNING();

    /// 爆炸伤害标签
    /// 包含: fireworks, explosion, player_explosion, bad_respawn_point
    static DamageTypeTag& IS_EXPLOSION();

    /// 摔落伤害标签
    /// 包含: fall, ender_pearl, stalagmite
    static DamageTypeTag& IS_FALL();

    /// 火焰伤害标签
    /// 包含: in_fire, campfire, on_fire, lava, hot_floor, unattributed_fireball, fireball
    static DamageTypeTag& IS_FIRE();

    /// 冰冻伤害标签
    /// 包含: freeze
    static DamageTypeTag& IS_FREEZING();

    /// 闪电伤害标签
    /// 包含: lightning_bolt
    static DamageTypeTag& IS_LIGHTNING();

    /// 玩家攻击标签
    /// 包含: player_attack, spear, mace_smash
    static DamageTypeTag& IS_PLAYER_ATTACK();

    /// 投射物伤害标签
    /// 包含: arrow, trident, mob_projectile, unattributed_fireball, fireball,
    ///       wither_skull, thrown, wind_charge
    static DamageTypeTag& IS_PROJECTILE();

    // ========== 重锤相关标签 ==========

    /// 重锤砸地标签
    /// 包含: mace_smash
    static DamageTypeTag& MACE_SMASH();

    // ========== AI 行为相关标签 ==========

    /// 不激怒目标标签
    /// 这些伤害类型不会激怒被攻击的生物
    /// 包含: mob_attack_no_aggro
    static DamageTypeTag& NO_ANGER();

    /// 无冲击标签
    /// 这些伤害类型不产生击退效果
    /// 包含: drown
    static DamageTypeTag& NO_IMPACT();

    /// 无击退标签
    /// 这些伤害类型不产生击退（与 NO_IMPACT 不同，仅影响击退不影响其他效果）
    /// 包含: explosion, player_explosion, bad_respawn_point, in_fire, lightning_bolt,
    ///       on_fire, lava, hot_floor, in_wall, cramming, drown, starve, cactus, fall,
    ///       ender_pearl, fly_into_wall, out_of_world, generic, magic, wither, dragon_breath,
    ///       dry_out, sweet_berry_bush, freeze, stalagmite, outside_border, generic_kill,
    ///       campfire, spear
    static DamageTypeTag& NO_KNOCKBACK();

    /// 恐慌原因标签
    /// 这些伤害类型会引发生物恐慌（逃跑）
    /// 包含: #panic_environmental_causes + arrow, dragon_breath, explosion, fireball, fireworks,
    ///       indirect_magic, magic, mob_attack, mob_projectile, player_explosion, sonic_boom,
    ///       sting, thrown, trident, unattributed_fireball, wind_charge, wither, wither_skull,
    ///       #is_player_attack
    static DamageTypeTag& PANIC_CAUSES();

    /// 环境恐慌原因标签
    /// 这些环境伤害类型会引发生物恐慌
    /// 包含: cactus, freeze, hot_floor, in_fire, lava, lightning_bolt, on_fire
    static DamageTypeTag& PANIC_ENVIRONMENTAL_CAUSES();

    // ========== 特殊生物标签 ==========

    /// 女巫抗性标签
    /// 女巫对这些伤害类型有抗性（受到的伤害降低）
    /// 包含: magic, indirect_magic, sonic_boom, thorns
    static DamageTypeTag& WITCH_RESISTANT_TO();

    /// 凋灵免疫标签
    /// 凋灵对这些伤害类型免疫
    /// 包含: drown
    static DamageTypeTag& WITHER_IMMUNE_TO();

    // ========== 末影龙相关标签 ==========

    /// 始终伤害末影龙标签
    /// 这些伤害类型始终对末影龙造成伤害（末影龙通常对爆炸免疫）
    /// 包含: #is_explosion
    static DamageTypeTag& ALWAYS_HURTS_ENDER_DRAGONS();

    // ========== 盔甲架相关标签 ==========

    /// 始终击杀盔甲架标签
    /// 这些伤害类型始终能破坏盔甲架
    /// 包含: arrow, trident, fireball, wither_skull, wind_charge
    static DamageTypeTag& ALWAYS_KILLS_ARMOR_STANDS();

    /// 燃烧盔甲架标签
    /// 这些伤害类型会点燃盔甲架
    /// 包含: on_fire
    static DamageTypeTag& BURNS_ARMOR_STANDS();

    /// 可破坏盔甲架标签
    /// 这些伤害类型可以破坏盔甲架
    /// 包含: player_explosion, #is_player_attack
    static DamageTypeTag& CAN_BREAK_ARMOR_STAND();

    /// 点燃盔甲架标签
    /// 这些伤害类型会点燃盔甲架（与 BURNS_ARMOR_STANDS 不同，此处指直接点燃）
    /// 包含: in_fire, campfire
    static DamageTypeTag& IGNITES_ARMOR_STANDS();

    // ========== 其他特殊标签 ==========

    /// 始终最显著摔落标签
    /// 这些伤害类型在战斗记录中始终被视为最显著的摔落
    /// 包含: out_of_world
    static DamageTypeTag& ALWAYS_MOST_SIGNIFICANT_FALL();

    /// 始终触发蠹虫标签
    /// 这些伤害类型会触发蠹虫生成（从石头中）
    /// 包含: magic
    static DamageTypeTag& ALWAYS_TRIGGERS_SILVERFISH();

    /// 守卫者荆棘回避标签
    /// 守卫者对这些伤害类型不触发荆棘反伤
    /// 包含: magic, thorns, #is_explosion
    static DamageTypeTag& AVOIDS_GUARDIAN_THORNS();

    /// 踩踏燃烧标签
    /// 这些伤害类型来自踩踏燃烧方块（用于触发脚步声等效果）
    /// 包含: campfire, hot_floor
    static DamageTypeTag& BURN_FROM_STEPPING();

    /// 损坏头盔标签
    /// 这些伤害类型会损坏玩家头盔（坠落方块、铁砧、钟乳石）
    /// 包含: falling_anvil, falling_block, falling_stalactite
    static DamageTypeTag& DAMAGES_HELMET();

    /**
     * @brief 初始化所有内置标签
     *
     * 在 DamageTypeTags::initialize() 之后，可通过数据包加载器（DamageTypeTagLoader）
     * 追加或替换标签内容。
     */
    static void initialize();

    /**
     * @brief 检查标签系统是否已初始化
     */
    [[nodiscard]] static bool isInitialized() { return s_initialized; }

    /**
     * @brief 根据ID获取标签
     * @param id 标签资源位置
     * @return 标签指针，如果不存在返回 nullptr
     */
    [[nodiscard]] static DamageTypeTag* getTag(const ResourceLocation& id);

    /**
     * @brief 注册一个空标签（数据包加载时使用）
     * @param id 标签资源位置
     * @return 注册的标签引用
     */
    static DamageTypeTag& registerTag(const ResourceLocation& id);

    /**
     * @brief 遍历所有标签
     */
    static void forEachTag(std::function<void(DamageTypeTag&)> callback);

private:
    DamageTypeTags() = delete;

    static std::unordered_map<ResourceLocation, std::unique_ptr<DamageTypeTag>>& _getTags();
    static bool s_initialized;
};

} // namespace mc
