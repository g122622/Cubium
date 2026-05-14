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

#include "../weapon/ThrowableItem.hpp"

namespace mc {

namespace potion {
class Potion;
}

namespace item {

/**
 * @brief 可投掷药水基类
 *
 * 喷溅药水和滞留药水的共同基类。
 * 提供药水效果检测、翻译键生成、投掷音效等共享功能。
 *
 * 参考: net.minecraft.item.ThrowablePotionItem (概念类)
 */
class ThrowablePotionItem : public ThrowableItem {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit ThrowablePotionItem(const ItemProperties& properties);

    ~ThrowablePotionItem() override = default;

    // ========== Item 接口重写 ==========

    /**
     * @brief 是否有附魔光效
     * @param stack 物品堆
     * @return 如果药水有效果则返回true
     */
    [[nodiscard]] bool hasEffect(const ItemStack& stack) const override;

    /**
     * @brief 获取翻译键
     * @param stack 物品堆
     * @return 带药水类型的翻译键
     */
    [[nodiscard]] std::string getTranslationKey(const ItemStack& stack) const override;

    // ========== ThrowableItem 接口重写 ==========

    /**
     * @brief 播放投掷音效
     * MC 1.16.5: 所有药水使用相同的投掷音效
     */
    void playThrowSound(Player& player) const override;

    /**
     * @brief 获取投掷速度
     * MC 1.16.5: 药水投掷速度为 0.5
     */
    [[nodiscard]] f32 getThrowVelocity() const override { return 0.5f; }

    /**
     * @brief 获取投掷偏移
     * MC 1.16.5: 药水投掷偏移为 0.0f
     */
    [[nodiscard]] f32 getThrowInaccuracy() const override { return 0.0f; }

protected:
    /**
     * @brief 获取基础翻译键（不含药水效果后缀）
     * @return 基础翻译键，如 "item.minecraft.splash_potion"
     */
    [[nodiscard]] virtual std::string getBaseTranslationKey() const = 0;

    /**
     * @brief 获取带效果后缀的翻译键前缀
     * @return 翻译键前缀，如 "item.minecraft.splash_potion.effect."
     */
    [[nodiscard]] virtual std::string getEffectTranslationKeyPrefix() const = 0;
};

} // namespace item
} // namespace mc
