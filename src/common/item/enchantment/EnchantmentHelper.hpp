#pragma once

#include "Enchantment.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include <vector>
#include <array>

namespace mc {

// 前向声明
class LivingEntity;
class Entity;
class Player;

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

    /**
     * @brief 获取击退等级
     *
     * @param stack 物品堆
     * @return 击退等级（0-2）
     */
    [[nodiscard]] static i32 getKnockbackLevel(const ItemStack& stack);

    /**
     * @brief 获取火焰附加等级
     *
     * @param stack 物品堆
     * @return 火焰附加等级（0-2）
     */
    [[nodiscard]] static i32 getFireAspectLevel(const ItemStack& stack);

    /**
     * @brief 获取抢夺等级
     *
     * @param stack 物品堆
     * @return 抢夺等级（0-3）
     */
    [[nodiscard]] static i32 getLootingLevel(const ItemStack& stack);

    /**
     * @brief 获取效率等级
     *
     * @param stack 物品堆
     * @return 效率等级（0-5）
     */
    [[nodiscard]] static i32 getEfficiencyLevel(const ItemStack& stack);

    /**
     * @brief 获取水下呼吸等级
     *
     * @param stack 物品堆
     * @return 水下呼吸等级（0-3）
     */
    [[nodiscard]] static i32 getRespirationLevel(const ItemStack& stack);

    /**
     * @brief 获取深海探索者等级
     *
     * @param stack 物品堆
     * @return 深海探索者等级（0-3）
     */
    [[nodiscard]] static i32 getDepthStriderLevel(const ItemStack& stack);

    /**
     * @brief 检查是否有水下速掘
     *
     * @param stack 物品堆
     * @return 如果有水下速掘返回true
     */
    [[nodiscard]] static bool hasAquaAffinity(const ItemStack& stack);

    /**
     * @brief 检查是否有冰霜行者
     *
     * @param stack 物品堆
     * @return 如果有冰霜行者返回true
     */
    [[nodiscard]] static bool hasFrostWalker(const ItemStack& stack);

    /**
     * @brief 检查是否有灵魂疾行
     *
     * @param stack 物品堆
     * @return 如果有灵魂疾行返回true
     */
    [[nodiscard]] static bool hasSoulSpeed(const ItemStack& stack);

    /**
     * @brief 检查是否有绑定诅咒
     *
     * @param stack 物品堆
     * @return 如果有绑定诅咒返回true
     */
    [[nodiscard]] static bool hasBindingCurse(const ItemStack& stack);

    /**
     * @brief 检查是否有消失诅咒
     *
     * @param stack 物品堆
     * @return 如果有消失诅咒返回true
     */
    [[nodiscard]] static bool hasVanishingCurse(const ItemStack& stack);

    /**
     * @brief 获取忠诚等级
     *
     * @param stack 物品堆
     * @return 忠诚等级（0-3）
     */
    [[nodiscard]] static i32 getLoyaltyLevel(const ItemStack& stack);

    /**
     * @brief 获取激流等级
     *
     * @param stack 物品堆
     * @return 激流等级（0-3）
     */
    [[nodiscard]] static i32 getRiptideLevel(const ItemStack& stack);

    /**
     * @brief 检查是否有引雷
     *
     * @param stack 物品堆
     * @return 如果有引雷返回true
     */
    [[nodiscard]] static bool hasChanneling(const ItemStack& stack);

    /**
     * @brief 获取横扫之刃伤害比例
     *
     * @param stack 物品堆
     * @return 横扫伤害比例（0.0-1.0）
     */
    [[nodiscard]] static f32 getSweepingDamageRatio(const ItemStack& stack);

    /**
     * @brief 获取钓鱼运气加成
     *
     * @param stack 物品堆
     * @return 海之眷顾等级（0-3）
     */
    [[nodiscard]] static i32 getFishingLuckBonus(const ItemStack& stack);

    /**
     * @brief 获取钓鱼速度加成
     *
     * @param stack 物品堆
     * @return 饵钓等级（0-3）
     */
    [[nodiscard]] static i32 getFishingSpeedBonus(const ItemStack& stack);

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

    // ========== 护甲附魔保护计算 ==========

    /**
     * @brief 计算护甲的附魔保护因子总和 (EPF)
     *
     * MC 1.16.5: 遍历所有护甲槽位，计算针对特定伤害类型的保护附魔总和
     * EPF 上限为 20，对应 80% 减伤
     *
     * @param armorSlots 护甲槽位数组（头盔、胸甲、护腿、靴子）
     * @param damageType 伤害类型
     * @return EPF 总和（已限制在 0-20 范围内）
     */
    [[nodiscard]] static i32 getTotalArmorProtection(
        const std::array<const ItemStack*, 4>& armorSlots,
        u32 damageType);

    /**
     * @brief 计算单个物品的附魔保护因子
     *
     * @param stack 物品堆
     * @param damageType 伤害类型
     * @return 保护因子值
     */
    [[nodiscard]] static i32 getProtectionFactor(const ItemStack& stack, u32 damageType);

    // ========== 耐久计算 ==========

    /**
     * @brief 检查耐久附魔是否阻止耐久损耗
     *
     * MC 1.16.5: 每级有 level/(level+1) 概率避免损耗
     * 护甲有 60% 概率不触发耐久效果
     *
     * @param level 耐久等级
     * @param isArmor 是否为护甲
     * @param random 随机数生成器
     * @return 如果应该忽略损耗返回true
     */
    [[nodiscard]] static bool shouldIgnoreDurabilityLoss(i32 level, bool isArmor, class math::Random& random);

private:
    EnchantmentHelper() = delete;  // 禁止实例化
};

} // namespace enchant
} // namespace item
} // namespace mc
