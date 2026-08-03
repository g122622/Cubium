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

#include "Potion.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/item/core/ItemStack.hpp"
#include <optional>
#include <string>
#include <vector>

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
     *
     * 返回药水的基础效果加上任何自定义效果。
     *
     * @param stack 物品堆
     * @return 效果列表
     */
    [[nodiscard]] static std::vector<entity::effect::EffectInstance> getEffects(const ItemStack& stack);

    /**
     * @brief 从物品堆获取自定义药水效果
     *
     * 只返回自定义添加的效果，不包括药水的基础效果。
     *
     * @param stack 物品堆
     * @return 自定义效果列表
     */
    [[nodiscard]] static std::vector<entity::effect::EffectInstance> getCustomEffects(const ItemStack& stack);

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
     * @brief 设置物品堆的自定义药水效果
     *
     * 替换物品堆上所有自定义效果。
     *
     * @param stack 物品堆
     * @param effects 自定义效果列表
     * @return 修改后的物品堆
     */
    static ItemStack& setCustomEffects(ItemStack& stack, const std::vector<entity::effect::EffectInstance>& effects);

    /**
     * @brief 添加单个自定义药水效果到物品堆
     *
     * 如果已存在相同类型的效果，会合并（取较强者）。
     *
     * @param stack 物品堆
     * @param effect 要添加的效果
     * @return 修改后的物品堆
     */
    static ItemStack& addCustomEffect(ItemStack& stack, const entity::effect::EffectInstance& effect);

    /**
     * @brief 移除物品堆上的所有自定义药水效果
     *
     * @param stack 物品堆
     * @return 修改后的物品堆
     */
    static ItemStack& removeCustomEffects(ItemStack& stack);

    /**
     * @brief 检查物品堆是否有自定义药水效果
     * @param stack 物品堆
     * @return 如果有自定义效果返回 true
     */
    [[nodiscard]] static bool hasCustomEffects(const ItemStack& stack);

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
     * @brief 获取物品堆的药水颜色
     *
     * 如果有自定义颜色则使用自定义颜色，否则计算效果颜色的平均值。
     *
     * @param stack 物品堆
     * @return ARGB颜色值
     */
    [[nodiscard]] static u32 getColor(const ItemStack& stack);

    /**
     * @brief 获取单个效果的颜色
     * @param type 效果类型
     * @return ARGB颜色值
     */
    [[nodiscard]] static u32 getEffectColor(entity::effect::EffectType type);

    /**
     * @brief 获取物品堆的自定义药水颜色
     *
     * 自定义颜色通过 CustomPotionColor 标签存储。
     *
     * @param stack 物品堆
     * @return 自定义颜色，如果没有则返回 std::nullopt
     */
    [[nodiscard]] static std::optional<u32> getCustomPotionColor(const ItemStack& stack);

    /**
     * @brief 设置物品堆的自定义药水颜色
     *
     * 设置后，药水将使用此颜色而不是根据效果计算颜色。
     * 设置为 std::nullopt 会移除自定义颜色。
     *
     * @param stack 物品堆
     * @param color 自定义颜色（ARGB格式），或 std::nullopt 移除自定义颜色
     * @return 修改后的物品堆
     */
    static ItemStack& setCustomPotionColor(ItemStack& stack, std::optional<u32> color);

    // ========== NBT 键 ==========

    static constexpr const char* NBT_POTION = "Potion";
    static constexpr const char* NBT_CUSTOM_POTION_EFFECTS = "CustomPotionEffects";
    static constexpr const char* NBT_CUSTOM_POTION_COLOR = "CustomPotionColor";

private:
    PotionUtils() = delete;
};

} // namespace potion
} // namespace mc
