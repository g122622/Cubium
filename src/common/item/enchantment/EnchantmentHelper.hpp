#pragma once

#include "Enchantment.hpp"
#include "common/item/core/ItemStack.hpp"
#include <vector>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 附魔查询工具类
 *
 * 提供查询物品附魔的静态方法。
 * 参考 MC 1.16.5 EnchantmentHelper
 *
 * 用法示例:
 * @code
 * // 检查是否有精准采集
 * bool hasSilkTouch = EnchantmentHelper::hasEnchantment(stack, "minecraft:silk_touch");
 *
 * // 获取时运等级
 * i32 fortuneLevel = EnchantmentHelper::getEnchantmentLevel(stack, "minecraft:fortune");
 *
 * // 检查是否有任意附魔
 * bool hasAnyEnchant = EnchantmentHelper::hasEnchantments(stack);
 * @endcode
 */
class EnchantmentHelper {
public:
    /**
     * @brief 获取物品上指定附魔的等级
     *
     * @param stack 物品堆
     * @param enchantmentId 附魔ID
     * @return 附魔等级（0表示无此附魔）
     */
    [[nodiscard]] static i32 getEnchantmentLevel(const ItemStack& stack, const String& enchantmentId);

    /**
     * @brief 获取物品上指定附魔的等级
     *
     * @param stack 物品堆
     * @param enchantment 附魔指针
     * @return 附魔等级（0表示无此附魔）
     */
    [[nodiscard]] static i32 getEnchantmentLevel(const ItemStack& stack, const Enchantment* enchantment);

    /**
     * @brief 检查物品是否有指定附魔
     *
     * @param stack 物品堆
     * @param enchantmentId 附魔ID
     * @return 如果有此附魔返回true
     */
    [[nodiscard]] static bool hasEnchantment(const ItemStack& stack, const String& enchantmentId);

    /**
     * @brief 检查物品是否有指定类型的附魔
     *
     * @param stack 物品堆
     * @param type 附魔类型
     * @return 如果有此类型的附魔返回true
     */
    [[nodiscard]] static bool hasEnchantmentType(const ItemStack& stack, EnchantmentType type);

    /**
     * @brief 检查物品是否有任意附魔
     *
     * @param stack 物品堆
     * @return 如果有任何附魔返回true
     */
    [[nodiscard]] static bool hasEnchantments(const ItemStack& stack);

    /**
     * @brief 获取物品上的所有附魔
     *
     * @param stack 物品堆
     * @return 附魔列表（附魔指针和等级）
     */
    [[nodiscard]] static std::vector<std::pair<const Enchantment*, i32>> getEnchantments(const ItemStack& stack);

    // ========== 特定附魔便捷方法 ==========

    /**
     * @brief 检查是否有精准采集
     *
     * @param stack 物品堆
     * @return 如果有精准采集返回true
     */
    [[nodiscard]] static bool hasSilkTouch(const ItemStack& stack);

    /**
     * @brief 获取时运等级
     *
     * @param stack 物品堆
     * @return 时运等级（0-3）
     */
    [[nodiscard]] static i32 getFortuneLevel(const ItemStack& stack);

    /**
     * @brief 获取锋利等级
     *
     * @param stack 物品堆
     * @return 锋利等级（0-5）
     */
    [[nodiscard]] static i32 getSharpnessLevel(const ItemStack& stack);

    /**
     * @brief 获取耐久等级
     *
     * @param stack 物品堆
     * @return 耐久等级（0-3）
     */
    [[nodiscard]] static i32 getUnbreakingLevel(const ItemStack& stack);

    // ========== 附魔计算 ==========

    /**
     * @brief 计算附魔后的保护值
     *
     * @param stack 物品堆
     * @param damageType 伤害类型
     * @return 总保护值
     */
    [[nodiscard]] static i32 getTotalProtection(const ItemStack& stack, u32 damageType);

    /**
     * @brief 计算附魔后的伤害加成
     *
     * @param stack 物品堆
     * @param entityType 目标实体类型
     * @return 额外伤害值
     */
    [[nodiscard]] static f32 getTotalDamageBonus(const ItemStack& stack, u32 entityType);

private:
    EnchantmentHelper() = delete;  // 禁止实例化
};

} // namespace enchant
} // namespace item
} // namespace mc
