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

#include "ThrowablePotionItem.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/weapon/ThrowableItem.hpp"
#include <string>

namespace mc {

namespace potion {
class Potion;
}

namespace item {

/**
 * @brief 滞留药水物品
 *
 * 投掷后产生滞留区域，在区域内的实体会获得效果。
 * 持续约30秒，每秒应用一次效果。
 * 继承ThrowablePotionItem以获得药水物品的共享行为。
 *
 * 参考: net.minecraft.item.LingeringPotionItem
 */
class LingeringPotionItem : public ThrowablePotionItem {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit LingeringPotionItem(const ItemProperties& properties);

    ~LingeringPotionItem() override = default;

    // ========== ThrowableItem 接口重写 ==========

    /**
     * @brief 创建投掷实体
     * @return 药水实体（滞留型）
     */
    [[nodiscard]] entity::ProjectileItemEntity* createProjectile(
        IWorld& world, Player& player, const ItemStack& stack) const override;

    /**
     * @brief 滞留药水是滞留型
     */
    [[nodiscard]] bool isLingering() const override { return true; }

protected:
    /**
     * @brief 获取基础翻译键
     */
    [[nodiscard]] std::string getBaseTranslationKey() const override
    {
        return std::string("item.minecraft.lingering_potion");
    }

    /**
     * @brief 获取带效果后缀的翻译键前缀
     */
    [[nodiscard]] std::string getEffectTranslationKeyPrefix() const override
    {
        return std::string("item.minecraft.lingering_potion.effect.");
    }
};

} // namespace item
} // namespace mc
