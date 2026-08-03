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

#include "item/potion/PotionBrewing.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/potion/Potion.hpp"
#include "item/Items.hpp"
#include "item/crafting/Ingredient.hpp"
#include "item/potion/PotionUtils.hpp"
#include "item/potion/Potions.hpp"
#include <vector>

namespace mc {
namespace potion {

// ========== 静态成员初始化 ==========

std::vector<PotionBrewing::PotionMix> PotionBrewing::s_potionMixes;
std::vector<PotionBrewing::ItemMix> PotionBrewing::s_itemMixes;
std::vector<crafting::Ingredient> PotionBrewing::s_potionItems;
bool PotionBrewing::s_initialized = false;

// ========== 公共方法 ==========

void PotionBrewing::initialize()
{
    if (s_initialized) {
        return;
    }
    s_initialized = true;

    // 注册药水容器
    _addContainer(Items::POTION);
    _addContainer(Items::SPLASH_POTION);
    _addContainer(Items::LINGERING_POTION);

    // 注册容器转换配方
    // 药水 + 火药 → 喷溅药水
    _addContainerRecipe(Items::POTION, Items::GUNPOWDER, Items::SPLASH_POTION);
    // 喷溅药水 + 龙息 → 滞留药水
    _addContainerRecipe(Items::SPLASH_POTION, Items::DRAGON_BREATH, Items::LINGERING_POTION);

    // ========== 基础药水转换 ==========
    // 水瓶 + 下界疣 → 尴尬的药水
    _addMix(Potions::WATER, Items::NETHER_WART, Potions::AWKWARD);

    // 水瓶 + 闪烁的西瓜片 → 平凡的药水
    _addMix(Potions::WATER, Items::GLISTERING_MELON_SLICE, Potions::MUNDANE);

    // 水瓶 + 恶魂之泪 → 平凡的药水
    _addMix(Potions::WATER, Items::GHAST_TEAR, Potions::MUNDANE);

    // 水瓶 + 兔子脚 → 平凡的药水
    _addMix(Potions::WATER, Items::RABBIT_FOOT, Potions::MUNDANE);

    // 水瓶 + 烈焰粉 → 平凡的药水
    _addMix(Potions::WATER, Items::BLAZE_POWDER, Potions::MUNDANE);

    // 水瓶 + 蜘蛛眼 → 平凡的药水
    _addMix(Potions::WATER, Items::SPIDER_EYE, Potions::MUNDANE);

    // 水瓶 + 糖 → 平凡的药水
    _addMix(Potions::WATER, Items::SUGAR, Potions::MUNDANE);

    // 水瓶 + 岩浆膏 → 平凡的药水
    _addMix(Potions::WATER, Items::MAGMA_CREAM, Potions::MUNDANE);

    // 水瓶 + 荧石粉 → 浓稠的药水
    _addMix(Potions::WATER, Items::GLOWSTONE_DUST, Potions::THICK);

    // 水瓶 + 红石 → 平凡的药水
    _addMix(Potions::WATER, Items::REDSTONE, Potions::MUNDANE);

    // ========== 从尴尬药水酿造 ==========
    // 尴尬的药水 + 金胡萝卜 → 夜视药水
    _addMix(Potions::AWKWARD, Items::GOLDEN_CARROT, Potions::NIGHT_VISION);

    // 尴尬的药水 + 恶魂之泪 → 生命恢复药水
    _addMix(Potions::AWKWARD, Items::GHAST_TEAR, Potions::REGENERATION);

    // 尴尬的药水 + 兔子脚 → 跳跃提升药水
    _addMix(Potions::AWKWARD, Items::RABBIT_FOOT, Potions::LEAPING);

    // 尴尬的药水 + 岩浆膏 → 防火药水
    _addMix(Potions::AWKWARD, Items::MAGMA_CREAM, Potions::FIRE_RESISTANCE);

    // 尴尬的药水 + 糖 → 速度药水
    _addMix(Potions::AWKWARD, Items::SUGAR, Potions::SWIFTNESS);

    // 尴尬的药水 + 河豚 → 水下呼吸药水
    _addMix(Potions::AWKWARD, Items::PUFFERFISH, Potions::WATER_BREATHING);

    // 尴尬的药水 + 闪烁的西瓜片 → 瞬间治疗药水
    _addMix(Potions::AWKWARD, Items::GLISTERING_MELON_SLICE, Potions::HEALING);

    // 尴尬的药水 + 蜘蛛眼 → 中毒药水
    _addMix(Potions::AWKWARD, Items::SPIDER_EYE, Potions::POISON);

    // 尴尬的药水 + 烈焰粉 → 力量药水
    _addMix(Potions::AWKWARD, Items::BLAZE_POWDER, Potions::STRENGTH);

    // 尴尬的药水 + 发酵蛛眼 → 虚弱药水
    _addMix(Potions::AWKWARD, Items::FERMENTED_SPIDER_EYE, Potions::WEAKNESS);

    // 水瓶 + 发酵蛛眼 → 虚弱药水
    _addMix(Potions::WATER, Items::FERMENTED_SPIDER_EYE, Potions::WEAKNESS);

    // 尴尬的药水 + 幻翼膜 → 缓降药水
    _addMix(Potions::AWKWARD, Items::PHANTOM_MEMBRANE, Potions::SLOW_FALLING);

    // 尴尬的药水 + 海龟壳 → 海龟大师药水
    _addMix(Potions::AWKWARD, Items::TURTLE_HELMET, Potions::TURTLE_MASTER);

    // ========== 夜视药水升级 ==========
    // 夜视药水 + 红石 → 长效夜视药水
    _addMix(Potions::NIGHT_VISION, Items::REDSTONE, Potions::LONG_NIGHT_VISION);

    // 夜视药水 + 发酵蛛眼 → 隐身药水
    _addMix(Potions::NIGHT_VISION, Items::FERMENTED_SPIDER_EYE, Potions::INVISIBILITY);

    // 长效夜视药水 + 发酵蛛眼 → 长效隐身药水
    _addMix(Potions::LONG_NIGHT_VISION, Items::FERMENTED_SPIDER_EYE, Potions::LONG_INVISIBILITY);

    // ========== 隐身药水升级 ==========
    // 隐身药水 + 红石 → 长效隐身药水
    _addMix(Potions::INVISIBILITY, Items::REDSTONE, Potions::LONG_INVISIBILITY);

    // ========== 跳跃提升药水升级 ==========
    // 跳跃药水 + 红石 → 长效跳跃药水
    _addMix(Potions::LEAPING, Items::REDSTONE, Potions::LONG_LEAPING);

    // 跳跃药水 + 荧石粉 → 跳跃药水 II
    _addMix(Potions::LEAPING, Items::GLOWSTONE_DUST, Potions::STRONG_LEAPING);

    // 跳跃药水 + 发酵蛛眼 → 缓慢药水
    _addMix(Potions::LEAPING, Items::FERMENTED_SPIDER_EYE, Potions::SLOWNESS);

    // 长效跳跃药水 + 发酵蛛眼 → 长效缓慢药水
    _addMix(Potions::LONG_LEAPING, Items::FERMENTED_SPIDER_EYE, Potions::LONG_SLOWNESS);

    // ========== 防火药水升级 ==========
    // 防火药水 + 红石 → 长效防火药水
    _addMix(Potions::FIRE_RESISTANCE, Items::REDSTONE, Potions::LONG_FIRE_RESISTANCE);

    // ========== 速度药水升级 ==========
    // 速度药水 + 红石 → 长效速度药水
    _addMix(Potions::SWIFTNESS, Items::REDSTONE, Potions::LONG_SWIFTNESS);

    // 速度药水 + 荧石粉 → 速度药水 II
    _addMix(Potions::SWIFTNESS, Items::GLOWSTONE_DUST, Potions::STRONG_SWIFTNESS);

    // 速度药水 + 发酵蛛眼 → 缓慢药水
    _addMix(Potions::SWIFTNESS, Items::FERMENTED_SPIDER_EYE, Potions::SLOWNESS);

    // 长效速度药水 + 发酵蛛眼 → 长效缓慢药水
    _addMix(Potions::LONG_SWIFTNESS, Items::FERMENTED_SPIDER_EYE, Potions::LONG_SLOWNESS);

    // ========== 水下呼吸药水升级 ==========
    // 水下呼吸药水 + 红石 → 长效水下呼吸药水
    _addMix(Potions::WATER_BREATHING, Items::REDSTONE, Potions::LONG_WATER_BREATHING);

    // ========== 瞬间治疗药水升级 ==========
    // 治疗药水 + 荧石粉 → 治疗药水 II
    _addMix(Potions::HEALING, Items::GLOWSTONE_DUST, Potions::STRONG_HEALING);

    // 治疗药水 + 发酵蛛眼 → 瞬间伤害药水
    _addMix(Potions::HEALING, Items::FERMENTED_SPIDER_EYE, Potions::HARMING);

    // 治疗药水 II + 发酵蛛眼 → 瞬间伤害药水 II
    _addMix(Potions::STRONG_HEALING, Items::FERMENTED_SPIDER_EYE, Potions::STRONG_HARMING);

    // ========== 中毒药水升级 ==========
    // 中毒药水 + 红石 → 长效中毒药水
    _addMix(Potions::POISON, Items::REDSTONE, Potions::LONG_POISON);

    // 中毒药水 + 荧石粉 → 中毒药水 II
    _addMix(Potions::POISON, Items::GLOWSTONE_DUST, Potions::STRONG_POISON);

    // 中毒药水 + 发酵蛛眼 → 瞬间伤害药水
    _addMix(Potions::POISON, Items::FERMENTED_SPIDER_EYE, Potions::HARMING);

    // 长效中毒药水 + 发酵蛛眼 → 瞬间伤害药水
    _addMix(Potions::LONG_POISON, Items::FERMENTED_SPIDER_EYE, Potions::HARMING);

    // 强效中毒药水 + 发酵蛛眼 → 瞬间伤害药水 II
    _addMix(Potions::STRONG_POISON, Items::FERMENTED_SPIDER_EYE, Potions::STRONG_HARMING);

    // ========== 生命恢复药水升级 ==========
    // 生命恢复药水 + 红石 → 长效生命恢复药水
    _addMix(Potions::REGENERATION, Items::REDSTONE, Potions::LONG_REGENERATION);

    // 生命恢复药水 + 荧石粉 → 生命恢复药水 II
    _addMix(Potions::REGENERATION, Items::GLOWSTONE_DUST, Potions::STRONG_REGENERATION);

    // ========== 力量药水升级 ==========
    // 力量药水 + 红石 → 长效力量药水
    _addMix(Potions::STRENGTH, Items::REDSTONE, Potions::LONG_STRENGTH);

    // 力量药水 + 荧石粉 → 力量药水 II
    _addMix(Potions::STRENGTH, Items::GLOWSTONE_DUST, Potions::STRONG_STRENGTH);

    // ========== 虚弱药水升级 ==========
    // 虚弱药水 + 红石 → 长效虚弱药水
    _addMix(Potions::WEAKNESS, Items::REDSTONE, Potions::LONG_WEAKNESS);

    // ========== 海龟大师药水升级 ==========
    // 海龟大师药水 + 红石 → 长效海龟大师药水
    _addMix(Potions::TURTLE_MASTER, Items::REDSTONE, Potions::LONG_TURTLE_MASTER);

    // 海龟大师药水 + 荧石粉 → 海龟大师药水 II
    _addMix(Potions::TURTLE_MASTER, Items::GLOWSTONE_DUST, Potions::STRONG_TURTLE_MASTER);

    // ========== 缓降药水升级 ==========
    // 缓降药水 + 红石 → 长效缓降药水
    _addMix(Potions::SLOW_FALLING, Items::REDSTONE, Potions::LONG_SLOW_FALLING);

    // ========== 缓慢药水升级 ==========
    // 缓慢药水 + 红石 → 长效缓慢药水
    _addMix(Potions::SLOWNESS, Items::REDSTONE, Potions::LONG_SLOWNESS);

    // 缓慢药水 + 荧石粉 → 缓慢药水 IV
    _addMix(Potions::SLOWNESS, Items::GLOWSTONE_DUST, Potions::STRONG_SLOWNESS);

    // ========== 瞬间伤害药水升级 ==========
    // 伤害药水 + 荧石粉 → 伤害药水 II
    _addMix(Potions::HARMING, Items::GLOWSTONE_DUST, Potions::STRONG_HARMING);
}

bool PotionBrewing::isPotionItem(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    for (const auto& ingredient : s_potionItems) {
        if (ingredient.test(stack)) {
            return true;
        }
    }
    return false;
}

bool PotionBrewing::isReagent(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    // 检查是否为药水类型转换材料
    for (const auto& mix : s_potionMixes) {
        if (mix.reagent.test(stack)) {
            return true;
        }
    }

    // 检查是否为容器转换材料
    for (const auto& mix : s_itemMixes) {
        if (mix.reagent.test(stack)) {
            return true;
        }
    }

    return false;
}

bool PotionBrewing::canBrew(const ItemStack& potionStack, const ItemStack& reagentStack)
{
    if (!isPotionItem(potionStack) || reagentStack.isEmpty()) {
        return false;
    }

    return _hasPotionConversion(PotionUtils::getPotion(potionStack), reagentStack) ||
        _hasItemConversion(potionStack.getItem(), reagentStack);
}

ItemStack PotionBrewing::brew(const ItemStack& potionStack, const ItemStack& reagentStack)
{
    if (potionStack.isEmpty()) {
        return potionStack;
    }

    // 尝试容器转换
    const Item* outputItem = _doItemConversion(potionStack.getItem(), reagentStack);
    if (outputItem != nullptr) {
        const Potion* potion = PotionUtils::getPotion(potionStack);
        ItemStack result(outputItem);
        PotionUtils::setPotion(result, potion);
        return result;
    }

    // 尝试药水类型转换
    const Potion* outputPotion = _doPotionConversion(PotionUtils::getPotion(potionStack), reagentStack);
    if (outputPotion != nullptr) {
        ItemStack result = potionStack.copy();
        PotionUtils::setPotion(result, outputPotion);
        return result;
    }

    return potionStack;
}

bool PotionBrewing::isBrewablePotion(const Potion* potion)
{
    if (potion == nullptr) {
        return false;
    }
    for (const auto& mix : s_potionMixes) {
        if (mix.output() == potion) {
            return true;
        }
    }
    return false;
}

// ========== 私有方法实现 ==========

void PotionBrewing::_addContainer(const Item* item)
{
    if (item != nullptr) {
        s_potionItems.push_back(crafting::Ingredient::fromItem(item));
    }
}

void PotionBrewing::_addContainerRecipe(const Item* input, const Item* reagent, const Item* output)
{
    if (input != nullptr && reagent != nullptr && output != nullptr) {
        s_itemMixes.emplace_back(input, crafting::Ingredient::fromItem(reagent), output);
    }
}

void PotionBrewing::_addMix(const Potion* input, const Item* reagent, const Potion* output)
{
    if (input != nullptr && reagent != nullptr && output != nullptr) {
        s_potionMixes.emplace_back(input, crafting::Ingredient::fromItem(reagent), output);
    }
}

bool PotionBrewing::_hasPotionConversion(const Potion* potion, const ItemStack& reagent)
{
    if (potion == nullptr || reagent.isEmpty()) {
        return false;
    }

    for (const auto& mix : s_potionMixes) {
        if (mix.input() == potion && mix.reagent.test(reagent)) {
            return true;
        }
    }
    return false;
}

bool PotionBrewing::_hasItemConversion(const Item* item, const ItemStack& reagent)
{
    if (item == nullptr || reagent.isEmpty()) {
        return false;
    }

    for (const auto& mix : s_itemMixes) {
        if (mix.input() == item && mix.reagent.test(reagent)) {
            return true;
        }
    }
    return false;
}

const Potion* PotionBrewing::_doPotionConversion(const Potion* potion, const ItemStack& reagent)
{
    if (potion == nullptr || reagent.isEmpty()) {
        return nullptr;
    }

    for (const auto& mix : s_potionMixes) {
        if (mix.input() == potion && mix.reagent.test(reagent)) {
            return mix.output();
        }
    }
    return nullptr;
}

const Item* PotionBrewing::_doItemConversion(const Item* item, const ItemStack& reagent)
{
    if (item == nullptr || reagent.isEmpty()) {
        return nullptr;
    }

    for (const auto& mix : s_itemMixes) {
        if (mix.input() == item && mix.reagent.test(reagent)) {
            return mix.output();
        }
    }
    return nullptr;
}

} // namespace potion
} // namespace mc
