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

#include "Enchantment.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace mc {

// 前向声明
class LivingEntity;
class Entity;
class Player;

// 前向声明（定义在 LivingEntity.hpp 中）
enum class EquipmentSlot : u8;

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
    [[nodiscard]] static i32 getEnchantmentLevel(const ItemStack& stack, const std::string& enchantmentId);

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
    [[nodiscard]] static bool hasEnchantment(const ItemStack& stack, const std::string& enchantmentId);

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

    /**
     * @brief 设置物品上的所有附魔
     *
     * 清除物品上现有的所有附魔，然后添加新的附魔列表。
     * 参考: net.minecraft.enchantment.EnchantmentHelper.setEnchantments
     *
     * @param enchantments 附魔列表（附魔指针和等级）
     * @param stack 物品堆（会被修改）
     */
    static void setEnchantments(const std::vector<std::pair<const Enchantment*, i32>>& enchantments, ItemStack& stack);

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

    // ========== 重锤附魔便捷方法 ==========

    /**
     * @brief 获取致密附魔等级
     *
     * @param stack 物品堆
     * @return 致密等级（0-5）
     */
    [[nodiscard]] static i32 getDensityLevel(const ItemStack& stack);

    /**
     * @brief 获取破甲附魔等级
     *
     * @param stack 物品堆
     * @return 破甲等级（0-4）
     */
    [[nodiscard]] static i32 getBreachLevel(const ItemStack& stack);

    /**
     * @brief 获取风爆附魔等级
     *
     * @param stack 物品堆
     * @return 风爆等级（0-3）
     */
    [[nodiscard]] static i32 getWindBurstLevel(const ItemStack& stack);

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
     * @param target 受击目标实体（ nullptr 时附魔按"无目标"返回 0）
     * @return 额外伤害值
     */
    [[nodiscard]] static f32 getTotalDamageBonus(const ItemStack& stack, const LivingEntity* target);

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
    [[nodiscard]] static i32 getTotalArmorProtection(const std::array<const ItemStack*, 4>& armorSlots, u32 damageType);

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
    [[nodiscard]] static bool shouldIgnoreDurabilityLoss(i32 level, bool isArmor, math::Random& random);

    // ========== 附魔回调分发 ==========

    /**
     * @brief 当攻击目标时调用附魔的 onEntityDamaged 回调（仅主手武器）
     *
     * 遍历主手物品的所有附魔，调用 onEntityDamaged 方法。
     * 参考 MC 1.16.5 EnchantmentHelper.applyArthropodEnchantmentDamage()
     *
     * @param user 攻击者
     * @param target 目标实体
     * @param weapon 武器物品堆
     */
    static void applyArthropodEnchantmentDamage(LivingEntity& user, Entity& target, const ItemStack& weapon);

    /**
     * @brief 当攻击目标时调用附魔的 onEntityDamaged 回调（所有装备）
     *
     * 遍历攻击者所有装备和护甲的附魔，调用 onEntityDamaged 方法。
     * 参考 MC 1.16.5 EnchantmentHelper.applyArthropodEnchantments()
     *
     * 用于投射物（如潜影贝子弹）命中目标后触发攻击型附魔效果。
     *
     * @param user 攻击者（发射投射物的实体）
     * @param target 目标实体
     */
    static void applyArthropodEnchantments(LivingEntity& user, Entity& target);

    /**
     * @brief 当受伤时调用荆棘附魔的 onUserHurt 回调
     *
     * 遍历所有护甲槽位的荆棘附魔，调用 onUserHurt 方法。
     * 参考 MC 1.16.5 EnchantmentHelper.applyThornsEnchantments()
     *
     * @param user 受伤者
     * @param attacker 攻击者
     * @param armorSlots 护甲槽位数组（头盔、胸甲、护腿、靴子）
     */
    static void applyThornsEnchantments(
        LivingEntity& user, Entity& attacker, const std::array<const ItemStack*, 4>& armorSlots);

    /**
     * @brief 当受伤时调用荆棘附魔的 onUserHurt 回调（从 LivingEntity 获取护甲）
     *
     * 遍历受伤者所有护甲的荆棘附魔，调用 onUserHurt 方法。
     * 参考 MC 1.16.5 EnchantmentHelper.applyThornEnchantments()
     *
     * @param user 受伤者
     * @param attacker 攻击者
     */
    static void applyThornsEnchantments(LivingEntity& user, Entity& attacker);

    // ========== 位置依赖附魔效果 ==========

    /**
     * @brief 在实体移动到新方块位置时运行位置依赖的附魔效果
     *
     * 遍历实体所有装备的附魔，对每个附魔调用 onLocationChanged()。
     * 当附魔从非活跃变为活跃时执行效果（如冰霜行者放置霜冰），
     * 当附魔从活跃变为非活跃时调用 onLocationEffectDeactivated()。
     *
     * @param entity 实体
     */
    static void runLocationChangedEffects(LivingEntity& entity);

    /**
     * @brief 在实体移动到新方块位置时运行指定物品的位置依赖附魔效果
     *
     * 用于装备变更时对新装备运行位置检测。
     *
     * @param entity 实体
     * @param stack 物品堆
     * @param slot 装备槽位
     */
    static void runLocationChangedEffects(LivingEntity& entity, const ItemStack& stack, EquipmentSlot slot);

    /**
     * @brief 停止指定物品上所有位置依赖的附魔效果
     *
     * 当装备被移除或物品损坏时调用，停用所有活跃的位置依赖效果。
     *
     * 当装备被移除或物品损坏时调用，停用所有活跃的位置依赖效果。
     *
     * @param entity 实体
     * @param stack 物品堆
     * @param slot 装备槽位
     */
    static void stopLocationBasedEffects(LivingEntity& entity, const ItemStack& stack, EquipmentSlot slot);

    /**
     * @brief 停止实体所有装备上所有位置依赖的附魔效果
     *
     * 停用实体所有装备上所有位置依赖的附魔效果。
     *
     * @param entity 实体
     */
    static void stopAllLocationBasedEffects(LivingEntity& entity);

    // ========== 附魔属性修饰符（常驻，装备时应用/卸下时移除） ==========

    /**
     * @brief 应用物品堆上所有附魔的属性修饰符到实体
     *
     * 对齐 vanilla 1.21.11 的 EnchantmentEffectComponents.ATTRIBUTES：装备附魔物品时，
     * 将该物品上每个附魔经 getAttributeModifiers(level) 返回的属性修饰符加到实体属性
     *（addTransientModifier）。由 LivingEntity 装备同步管线在物品固有修饰符之后调用。
     *
     * 仅应用槽位匹配的修饰符（Entry.equipmentSlot == slot），与物品固有修饰符过滤范式一致。
     * 同 id 修饰符先移除后添加，保证等级变化时更新而非叠加。
     *
     * @param entity 装备实体的实体
     * @param stack 装备的物品堆
     * @param slot 装备槽位
     */
    static void applyEnchantmentAttributeModifiers(LivingEntity& entity, const ItemStack& stack, EquipmentSlot slot);

    /**
     * @brief 移除物品堆上所有附魔的属性修饰符
     *
     * 装备被卸下或物品损坏时调用，移除该物品附魔经 getAttributeModifiers 提供的属性修饰符。
     * 由 LivingEntity::stopLocationBasedEffects 在物品固有修饰符移除之后调用。
     *
     * 注意：移除时按当前物品堆的附魔等级计算修饰符 id（与应用时一致），故物品堆须未被销毁前调用。
     *
     * @param entity 装备实体的实体
     * @param stack 被卸下的物品堆
     * @param slot 装备槽位
     */
    static void removeEnchantmentAttributeModifiers(LivingEntity& entity, const ItemStack& stack, EquipmentSlot slot);

    // ========== 附魔生成（附魔台用） ==========

    /**
     * @brief 附魔数据结构
     *
     * 包含附魔和等级，用于附魔台生成附魔列表。
     */
    struct EnchantmentData {
        const Enchantment* enchantment;
        i32 level;
        i32 weight; // 权重（用于随机选择）

        EnchantmentData(const Enchantment* ench, i32 lvl)
            : enchantment(ench)
            , level(lvl)
            , weight(ench ? ench->rarityWeight() : 0)
        {}
    };

    /**
     * @brief 计算物品附魔等级
     *
     * MC 1.16.5: EnchantmentHelper.calcItemStackEnchantability
     * 根据书架力量和槽位计算附魔等级。
     *
     * @param random 随机数生成器
     * @param slotIndex 槽位索引（0-2）
     * @param power 书架力量（0-15）
     * @param stack 物品堆
     * @return 附魔等级，如果物品不可附魔返回0
     */
    [[nodiscard]] static i32 calcItemStackEnchantability(
        math::Random& random, i32 slotIndex, i32 power, const ItemStack& stack);

    /**
     * @brief 获取物品可用的附魔列表
     *
     * MC 1.16.5: EnchantmentHelper.getEnchantmentDatas
     * 返回指定等级范围内可用于该物品的所有附魔。
     *
     * @param level 附魔等级
     * @param stack 物品堆
     * @param allowTreasure 是否允许宝藏附魔
     * @return 可用附魔列表
     */
    [[nodiscard]] static std::vector<EnchantmentData> getEnchantmentDatas(
        i32 level, const ItemStack& stack, bool allowTreasure);

    /**
     * @brief 构建附魔列表
     *
     * MC 1.16.5: EnchantmentHelper.buildEnchantmentList
     * 根据物品和等级生成附魔列表，可能包含多个附魔。
     *
     * @param random 随机数生成器
     * @param stack 物品堆
     * @param level 附魔等级
     * @param allowTreasure 是否允许宝藏附魔
     * @return 附魔列表
     */
    [[nodiscard]] static std::vector<EnchantmentData> buildEnchantmentList(
        math::Random& random, const ItemStack& stack, i32 level, bool allowTreasure);

    /**
     * @brief 移除与指定附魔不兼容的附魔
     *
     * MC 1.16.5: EnchantmentHelper.removeIncompatible
     * 从列表中移除与指定附魔不兼容的所有附魔。
     *
     * @param list 附魔列表（会被修改）
     * @param enchantment 参考附魔
     */
    static void removeIncompatible(std::vector<EnchantmentData>& list, const Enchantment* enchantment);

    /**
     * @brief 加权随机选择附魔
     *
     * MC 1.16.5: WeightedRandom.getRandomItem
     * 根据附魔稀有度权重随机选择一个附魔。
     *
     * @param random 随机数生成器
     * @param list 附魔列表
     * @return 选中的附魔数据，如果列表为空返回空
     */
    [[nodiscard]] static EnchantmentData getRandomEnchantment(math::Random& random, std::vector<EnchantmentData>& list);

    /**
     * @brief 添加随机附魔到物品
     *
     * MC 1.16.5: EnchantmentHelper.addRandomEnchantment
     * 根据等级生成随机附魔并应用到物品上。
     * 如果物品是书，会转换为附魔书。
     *
     * @param random 随机数生成器
     * @param stack 物品堆（会被修改）
     * @param level 附魔等级
     * @param allowTreasure 是否允许宝藏附魔
     * @return 添加了附魔的物品（可能是新的附魔书物品）
     */
    static ItemStack addRandomEnchantment(math::Random& random, ItemStack stack, i32 level, bool allowTreasure);

private:
    EnchantmentHelper() = delete; // 禁止实例化
};

} // namespace enchant
} // namespace item
} // namespace mc
