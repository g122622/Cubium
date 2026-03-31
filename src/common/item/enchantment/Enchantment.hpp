#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include <memory>
#include <vector>
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 附魔类型
 *
 * 定义附魔可以应用的物品类型。
 * 参考 MC 1.16.5 EnchantmentType
 */
enum class EnchantmentType : u8 {
    Armor,          ///< 护甲（头盔、胸甲、护腿、靴子）
    ArmorFeet,      ///< 靴子
    ArmorHead,      ///< 头盔
    ArmorChest,     ///< 胸甲
    Weapon,         ///< 武器（剑）
    Digger,         ///< 挖掘工具（镐、斧、铲、锄）
    FishingRod,     ///< 钓鱼竿
    Breakable,      ///< 可破坏物品
    Bow,            ///< 弓
    Wearable,       ///< 可穿戴物品
    Crossbow,       ///< 弩
    Trident,        ///< 三叉戟
    Vanishable,     ///< 可消失物品
    All             ///< 所有物品
};

/**
 * @brief 附魔稀有度
 *
 * 影响附魔在附魔台出现的概率和所需等级。
 * 参考 MC 1.16.5 Rarity
 */
enum class EnchantmentRarity : u8 {
    Common,     ///< 普通（10权重）- 保护、锋利等
    Uncommon,   ///< 稀有（5权重）- 冲击、火焰附加等
    Rare,       ///< 罕见（2权重）- 掉落物倍增、精准采集等
    VeryRare    ///< 极罕见（1权重）- 时运、经验修补等
};

/**
 * @brief 附魔基类
 *
 * 定义所有附魔的通用接口和属性。
 * 参考 MC 1.16.5 Enchantment
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
    [[nodiscard]] virtual String id() const = 0;

    /**
     * @brief 获取附魔显示名称
     * @param level 附魔等级
     * @return 本地化名称键（如 "enchantment.minecraft.fortune"）
     */
    [[nodiscard]] virtual String getNameKey(i32 level = 1) const;

    // ========== 等级 ==========

    /**
     * @brief 获取最小等级
     * @return 最小等级（通常为1）
     */
    [[nodiscard]] virtual i32 minLevel() const { return 1; }

    /**
     * @brief 获取最大等级
     * @return 最大等级
     */
    [[nodiscard]] virtual i32 maxLevel() const { return 1; }

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
    [[nodiscard]] virtual EnchantmentRarity rarity() const { return EnchantmentRarity::Common; }

    // ========== 适用性 ==========

    /**
     * @brief 检查是否可以应用到指定物品类型
     * @param itemType 物品类型
     * @return 如果可以应用返回true
     */
    [[nodiscard]] virtual bool canApplyTo(u32 itemType) const;

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
    [[nodiscard]] virtual i32 getMinEnchantability(i32 level) const;

    /**
     * @brief 获取附魔台槽位中的最大等级要求
     * @param level 附魔等级
     * @return 最大玩家等级要求
     */
    [[nodiscard]] virtual i32 getMaxEnchantability(i32 level) const;

    // ========== 修饰符 ==========

    /**
     * @brief 计算伤害加成
     * @param level 附魔等级
     * @param entityType 目标实体类型（可选）
     * @return 额外伤害值
     */
    [[nodiscard]] virtual f32 getDamageBonus(i32 level, u32 entityType = 0) const;

    /**
     * @brief 计算保护加成
     * @param level 附魔等级
     * @param damageType 伤害类型
     * @return 保护点数
     */
    [[nodiscard]] virtual i32 getDamageProtection(i32 level, u32 damageType) const;

    // ========== 稀有度权重 ==========

    /**
     * @brief 获取稀有度对应的权重
     * @param rarity 稀有度
     * @return 权重值
     */
    [[nodiscard]] static i32 getRarityWeight(EnchantmentRarity rarity);

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
