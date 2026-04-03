#pragma once

#include "Potion.hpp"
#include "../core/ItemStack.hpp"
#include <string>

namespace mc {
namespace potion {

/**
 * @brief 药水工具类
 *
 * 提供药水相关的工具方法，如从物品获取药水、设置药水等。
 *
 * 参考: net.minecraft.potion.PotionUtils
 */
class PotionUtils {
public:
    /**
     * @brief 从物品堆获取药水
     * @param stack 物品堆
     * @return 药水指针，无效则返回 WATER
     */
    [[nodiscard]] static const Potion* getPotion(const ItemStack& stack);

    /**
     * @brief 从物品堆获取效果列表
     * @param stack 物品堆
     * @return 效果列表
     */
    [[nodiscard]] static std::vector<entity::effect::EffectInstance> getEffects(const ItemStack& stack);

    /**
     * @brief 从药水获取效果列表
     * @param potion 药水
     * @return 效果列表
     */
    [[nodiscard]] static std::vector<entity::effect::EffectInstance> getEffects(const Potion* potion);

    /**
     * @brief 创建药水物品
     * @param potion 药水类型
     * @return 药水物品堆
     */
    [[nodiscard]] static ItemStack createPotionItem(const Potion* potion);

    /**
     * @brief 创建喷溅药水物品
     * @param potion 药水类型
     * @return 喷溅药水物品堆
     */
    [[nodiscard]] static ItemStack createSplashPotionItem(const Potion* potion);

    /**
     * @brief 创建滞留药水物品
     * @param potion 药水类型
     * @return 滞留药水物品堆
     */
    [[nodiscard]] static ItemStack createLingeringPotionItem(const Potion* potion);

    /**
     * @brief 设置物品堆的药水
     * @param stack 物品堆
     * @param potion 药水类型
     * @return 修改后的物品堆
     */
    static ItemStack& setPotion(ItemStack& stack, const Potion* potion);

    /**
     * @brief 检查物品堆是否为药水
     * @param stack 物品堆
     * @return 如果是药水返回true
     */
    [[nodiscard]] static bool isPotion(const ItemStack& stack);

    /**
     * @brief 检查物品堆是否为水瓶
     * @param stack 物品堆
     * @return 如果是水瓶返回true
     */
    [[nodiscard]] static bool isWaterBottle(const ItemStack& stack);

    /**
     * @brief 获取药水颜色
     * @param potion 药水
     * @return ARGB颜色值
     */
    [[nodiscard]] static u32 getColor(const Potion* potion);

    /**
     * @brief 获取效果列表的颜色
     * @param effects 效果列表
     * @return ARGB颜色值
     */
    [[nodiscard]] static u32 getColor(const std::vector<entity::effect::EffectInstance>& effects);

    /**
     * @brief 获取单个效果的颜色
     * @param type 效果类型
     * @return ARGB颜色值
     */
    [[nodiscard]] static u32 getEffectColor(entity::effect::EffectType type);

    // ========== NBT 键 ==========

    static constexpr const char* NBT_POTION = "Potion";
    static constexpr const char* NBT_CUSTOM_POTION_EFFECTS = "CustomPotionEffects";
    static constexpr const char* NBT_CUSTOM_POTION_COLOR = "CustomPotionColor";

private:
    PotionUtils() = delete;
};

} // namespace potion
} // namespace mc
