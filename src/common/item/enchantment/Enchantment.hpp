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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {

// 前向声明
class LivingEntity;
class Entity;
class ItemStack;
class Player;

namespace item {
namespace enchant {

/**
 * @brief 附魔类型
 *
 * 定义附魔可以应用的物品类型。
 */
enum class EnchantmentType : u8 {
    Armor,      ///< 护甲（头盔、胸甲、护腿、靴子）
    ArmorFeet,  ///< 靴子
    ArmorLegs,  ///< 护腿
    ArmorHead,  ///< 头盔
    ArmorChest, ///< 胸甲
    Weapon,     ///< 武器（剑）
    Digger,     ///< 挖掘工具（镐、斧、铲、锄）
    FishingRod, ///< 钓鱼竿
    Breakable,  ///< 可破坏物品
    Bow,        ///< 弓
    Wearable,   ///< 可穿戴物品
    Crossbow,   ///< 弩
    Trident,    ///< 三叉戟
    Vanishable, ///< 可消失物品
    All         ///< 所有物品
};

/**
 * @brief 附魔稀有度
 *
 * 影响附魔在附魔台出现的概率和所需等级。
 */
enum class EnchantmentRarity : u8 {
    Common,   ///< 普通（10权重）- 保护、锋利等
    Uncommon, ///< 稀有（5权重）- 冲击、火焰附加等
    Rare,     ///< 罕见（2权重）- 掉落物倍增、精准采集等
    VeryRare  ///< 极罕见（1权重）- 时运、经验修补等
};

/**
 * @brief 附魔基类
 *
 * 定义所有附魔的通用接口和属性。
 *
 * 用法示例:
 * @code
 * const Enchantment* fortune = EnchantmentRegistry::get("minecraft:fortune");
 * i32 maxLevel = fortune->maxLevel(); // 3
 * bool canApply = fortune->canApplyTo(ItemType::Pickaxe);
 * @endcode
 */
class Enchantment {
public:
    virtual ~Enchantment() = default;

    // ========== 标识 ==========

    /**
     * @brief 获取附魔ID
     * @return 附魔ID（如 "minecraft:fortune"）
     */
    [[nodiscard]] virtual std::string id() const = 0;

    /**
     * @brief 获取附魔显示名称
     * @param level 附魔等级
     * @return 本地化名称键（如 "enchantment.minecraft.fortune"）
     */
    [[nodiscard]] virtual std::string getNameKey(i32 level = 1) const;

    // ========== 等级 ==========

    /**
     * @brief 获取最小等级
     * @return 最小等级（通常为1）
     */
    [[nodiscard]] virtual i32 minLevel() const noexcept { return 1; }

    /**
     * @brief 获取最大等级
     * @return 最大等级
     */
    [[nodiscard]] virtual i32 maxLevel() const noexcept { return 1; }

    // ========== 类型 ==========

    /**
     * @brief 获取附魔类型
     * @return 附魔类型
     */
    [[nodiscard]] virtual EnchantmentType type() const = 0;

    /**
     * @brief 获取附魔稀有度
     * @return 稀有度
     */
    [[nodiscard]] virtual EnchantmentRarity rarity() const noexcept { return EnchantmentRarity::Common; }

    /**
     * @brief 获取稀有度对应的权重
     *
     * 用于附魔台加权随机选择。
     * 普通=10, 稀有=5, 罕见=2, 极罕见=1
     *
     * @return 权重值
     */
    [[nodiscard]] i32 rarityWeight() const noexcept { return getRarityWeight(rarity()); }

    /**
     * @brief 是否为宝藏附魔
     * @return 如果只能从箱子或交易获得返回true
     */
    [[nodiscard]] virtual bool isTreasure() const noexcept { return false; }

    /**
     * @brief 是否为诅咒附魔
     * @return 如果是诅咒返回true
     */
    [[nodiscard]] virtual bool isCurse() const noexcept { return false; }

    // ========== 适用性 ==========

    /**
     * @brief 检查是否可以应用到指定物品类型
     * @param itemType 物品类型
     * @return 如果可以应用返回true
     */
    [[nodiscard]] virtual bool canApplyTo(u32 itemType) const;

    /**
     * @brief 检查附魔是否可以应用到物品堆
     *
     * 这是铁砧合并附魔时使用的方法。
     * 默认实现调用 canApplyAtEnchantingTable。
     *
     * @param stack 物品堆
     * @return 如果可以应用返回true
     */
    [[nodiscard]] virtual bool canApply(const ItemStack& stack) const;

    /**
     * @brief 检查是否可以在附魔台获得
     * @param stack 物品堆
     * @return 如果可以在附魔台获得返回true（宝藏附魔返回false）
     */
    [[nodiscard]] virtual bool canApplyAtEnchantingTable(const ItemStack& stack) const;

    /**
     * @brief 检查是否可以出现在村民交易中
     * @return 如果可以出现在村民交易返回true
     */
    [[nodiscard]] virtual bool canVillagerTrade() const noexcept { return !isTreasure(); }

    /**
     * @brief 检查是否可以生成在战利品箱中
     * @return 如果可以生成在战利品箱返回true
     */
    [[nodiscard]] virtual bool canGenerateInLoot() const noexcept { return true; }

    /**
     * @brief 检查是否可以附在书上
     * @return 如果可以附在书上返回true
     */
    [[nodiscard]] virtual bool isAllowedOnBooks() const noexcept { return true; }

    /**
     * @brief 检查与另一个附魔的兼容性
     * @param other 另一个附魔
     * @return 如果兼容返回true（可以同时存在）
     */
    [[nodiscard]] virtual bool isCompatibleWith(const Enchantment& other) const;

    // ========== 附魔台成本 ==========

    /**
     * @brief 获取指定等级的最小经验成本
     * @param level 附魔等级
     * @return 最小经验等级
     */
    [[nodiscard]] virtual i32 getMinCost(i32 level) const;

    /**
     * @brief 获取指定等级的最大经验成本
     * @param level 附魔等级
     * @return 最大经验等级
     */
    [[nodiscard]] virtual i32 getMaxCost(i32 level) const;

    /**
     * @brief 获取附魔台槽位中的最小等级要求
     * @param level 附魔等级
     * @return 最小玩家等级要求
     */
    [[nodiscard]] virtual i32 getMinEnchantability(i32 level) const noexcept;

    /**
     * @brief 获取附魔台槽位中的最大等级要求
     * @param level 附魔等级
     * @return 最大玩家等级要求
     */
    [[nodiscard]] virtual i32 getMaxEnchantability(i32 level) const noexcept;

    // ========== 修饰符 ==========

    /**
     * @brief 计算伤害加成
     * @param level 附魔等级
     * @param target 受击目标实体（可选， nullptr 时附魔按"无目标"返回 0）
     * @return 额外伤害值
     *
     * 对齐 MC Java 1.21.11：伤害类附魔（锋利/亡灵杀手/节肢杀手/穿刺）的目标判定改用
     * EntityTypeTags 标签（SENSITIVE_TO_SMITE/SENSITIVE_TO_BANE_OF_ARTHROPODS/
     * SENSITIVE_TO_IMPALING），而非旧的 getMobType 枚举。穿刺（ImpalingEnchantment）
     * 已按此用 SENSITIVE_TO_IMPALING 标签判定水生生物。target 为 nullptr 时返回 0
     * （无目标无法做标签判定）。
     */
    [[nodiscard]] virtual f32 getDamageBonus(i32 level, const LivingEntity* target = nullptr) const noexcept;

    /**
     * @brief 计算保护加成
     * @param level 附魔等级
     * @param damageType 伤害类型
     * @return 保护点数
     */
    [[nodiscard]] virtual i32 getDamageProtection(i32 level, u32 damageType) const noexcept;

    /**
     * @brief 获取附魔在指定等级提供的属性修饰符
     *
     * 对齐 vanilla 1.21.11 的 EnchantmentEffectComponents.ATTRIBUTES
     *（EnchantmentAttributeEffect：属性 + LevelBasedValue + Operation + 槽位组）。
     * 装备该附魔物品时，由 LivingEntity 装备同步管线经
     * EnchantmentHelper::applyEnchantmentAttributeModifiers 将这些修饰符加到实体属性；
     * 卸下时经 removeEnchantmentAttributeModifiers 移除。
     *
     * 默认返回空（多数附魔不提供属性修饰符）。提供属性修饰符的附魔（如水下呼吸→oxygen_bonus）
     * override 本方法，按等级返回对应修饰符条目（Entry 内含 equipmentSlot，调用方按槽位过滤）。
     *
     * @param level 附魔等级（>=1）
     * @return 该等级下的属性修饰符集合（默认空）
     */
    [[nodiscard]] virtual item::ItemAttributeModifiers getAttributeModifiers(i32 level) const;

    // ========== 回调方法 ==========

    /**
     * @brief 当持有者攻击目标实体时调用
     *
     * 用于实现节肢杀手的缓慢效果、火焰附加的点燃效果等。
     *
     * @param user 攻击者（持有附魔物品的实体）
     * @param target 被攻击的目标实体
     * @param level 附魔等级
     */
    virtual void onEntityDamaged(LivingEntity& user, Entity& target, i32 level) const;

    /**
     * @brief 当持有者受到伤害时调用
     *
     * 用于实现荆棘的反伤效果。
     *
     * @param user 受伤者（持有附魔物品的实体）
     * @param attacker 攻击者
     * @param level 附魔等级
     */
    virtual void onUserHurt(LivingEntity& user, Entity& attacker, i32 level) const;

    // ========== 位置依赖附魔回调 ==========

    /**
     * @brief 当实体移动到新的方块位置时调用
     *
     * 用于实现冰霜行者（在水面上放置霜冰）、灵魂疾行（灵魂沙/土上加速）等
     * 基于位置的附魔效果。
     *
     * 此方法在每个 tick 中，当实体的方块位置发生变化时被调用。
     * 附魔应该在此方法中检查当前环境条件（如脚下方块、是否在地面等），
     * 然后决定是否激活效果。
     *
     * @param entity 持有附魔物品的实体
     * @param stack 附魔物品堆
     * @param slot 装备槽位
     * @param level 附魔等级
     * @param isActive 当前该附魔是否已经处于活跃状态
     * @return 如果附魔在此位置应该激活返回 true（活跃），否则返回 false（非活跃）
     */
    [[nodiscard]] virtual bool onLocationChanged(
        LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level, bool isActive) const;

    /**
     * @brief 当位置依赖附魔效果被停用时调用
     *
     * 当附魔效果从活跃变为非活跃时调用（例如实体离开灵魂沙、装备被移除等）。
     * 应在此方法中移除 onLocationChanged 中添加的临时效果（如属性修饰符）。
     *
     * @param entity 持有附魔物品的实体
     * @param stack 附魔物品堆
     * @param slot 装备槽位
     * @param level 附魔等级
     */
    virtual void onLocationEffectDeactivated(LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level) const;

    // ========== 稀有度权重 ==========

    /**
     * @brief 获取稀有度对应的权重
     * @param rarity 稀有度
     * @return 权重值
     */
    [[nodiscard]] static i32 getRarityWeight(EnchantmentRarity rarity) noexcept;

protected:
    /**
     * @brief 检查类型兼容性（内部使用）
     * @param other 另一个附魔
     * @return 如果类型兼容返回true
     */
    [[nodiscard]] bool isTypeCompatibleWith(const Enchantment& other) const;
};

} // namespace enchant
} // namespace item
} // namespace mc
