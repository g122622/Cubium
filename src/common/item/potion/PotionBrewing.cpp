#include "PotionBrewing.hpp"
#include "../Items.hpp"
#include "../crafting/Ingredient.hpp"
#include "PotionUtils.hpp"
#include "Potions.hpp"

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
    addContainer(Items::POTION);
    addContainer(Items::SPLASH_POTION);
    addContainer(Items::LINGERING_POTION);

    // 注册容器转换配方
    // 药水 + 火药 → 喷溅药水
    addContainerRecipe(Items::POTION, Items::GUNPOWDER, Items::SPLASH_POTION);
    // 喷溅药水 + 龙息 → 滞留药水
    addContainerRecipe(Items::SPLASH_POTION, Items::DRAGON_BREATH, Items::LINGERING_POTION);

    // ========== 基础药水转换 ==========
    // 水瓶 + 下界疣 → 尴尬的药水
    addMix(Potions::WATER, Items::NETHER_WART, Potions::AWKWARD);

    // 水瓶 + 闪烁的西瓜片 → 平凡的药水
    addMix(Potions::WATER, Items::GLISTERING_MELON_SLICE, Potions::MUNDANE);

    // 水瓶 + 恶魂之泪 → 平凡的药水
    addMix(Potions::WATER, Items::GHAST_TEAR, Potions::MUNDANE);

    // 水瓶 + 兔子脚 → 平凡的药水
    addMix(Potions::WATER, Items::RABBIT_FOOT, Potions::MUNDANE);

    // 水瓶 + 烈焰粉 → 平凡的药水
    addMix(Potions::WATER, Items::BLAZE_POWDER, Potions::MUNDANE);

    // 水瓶 + 蜘蛛眼 → 平凡的药水
    addMix(Potions::WATER, Items::SPIDER_EYE, Potions::MUNDANE);

    // 水瓶 + 糖 → 平凡的药水
    addMix(Potions::WATER, Items::SUGAR, Potions::MUNDANE);

    // 水瓶 + 岩浆膏 → 平凡的药水
    addMix(Potions::WATER, Items::MAGMA_CREAM, Potions::MUNDANE);

    // 水瓶 + 荧石粉 → 浓稠的药水
    addMix(Potions::WATER, Items::GLOWSTONE_DUST, Potions::THICK);

    // 水瓶 + 红石 → 平凡的药水
    addMix(Potions::WATER, Items::REDSTONE, Potions::MUNDANE);

    // ========== 从尴尬药水酿造 ==========
    // 尴尬的药水 + 金胡萝卜 → 夜视药水
    addMix(Potions::AWKWARD, Items::GOLDEN_CARROT, Potions::NIGHT_VISION);

    // 尴尬的药水 + 恶魂之泪 → 生命恢复药水
    addMix(Potions::AWKWARD, Items::GHAST_TEAR, Potions::REGENERATION);

    // 尴尬的药水 + 兔子脚 → 跳跃提升药水
    addMix(Potions::AWKWARD, Items::RABBIT_FOOT, Potions::LEAPING);

    // 尴尬的药水 + 岩浆膏 → 防火药水
    addMix(Potions::AWKWARD, Items::MAGMA_CREAM, Potions::FIRE_RESISTANCE);

    // 尴尬的药水 + 糖 → 速度药水
    addMix(Potions::AWKWARD, Items::SUGAR, Potions::SWIFTNESS);

    // 尴尬的药水 + 河豚 → 水下呼吸药水
    addMix(Potions::AWKWARD, Items::PUFFERFISH, Potions::WATER_BREATHING);

    // 尴尬的药水 + 闪烁的西瓜片 → 瞬间治疗药水
    addMix(Potions::AWKWARD, Items::GLISTERING_MELON_SLICE, Potions::HEALING);

    // 尴尬的药水 + 蜘蛛眼 → 中毒药水
    addMix(Potions::AWKWARD, Items::SPIDER_EYE, Potions::POISON);

    // 尴尬的药水 + 烈焰粉 → 力量药水
    addMix(Potions::AWKWARD, Items::BLAZE_POWDER, Potions::STRENGTH);

    // 尴尬的药水 + 发酵蛛眼 → 虚弱药水
    addMix(Potions::AWKWARD, Items::FERMENTED_SPIDER_EYE, Potions::WEAKNESS);

    // 水瓶 + 发酵蛛眼 → 虚弱药水（MC 1.16.5 补充配方）
    addMix(Potions::WATER, Items::FERMENTED_SPIDER_EYE, Potions::WEAKNESS);

    // 尴尬的药水 + 幻翼膜 → 缓降药水
    addMix(Potions::AWKWARD, Items::PHANTOM_MEMBRANE, Potions::SLOW_FALLING);

    // 尴尬的药水 + 海龟壳 → 海龟大师药水
    addMix(Potions::AWKWARD, Items::TURTLE_HELMET, Potions::TURTLE_MASTER);

    // ========== 夜视药水升级 ==========
    // 夜视药水 + 红石 → 长效夜视药水
    addMix(Potions::NIGHT_VISION, Items::REDSTONE, Potions::LONG_NIGHT_VISION);

    // 夜视药水 + 发酵蛛眼 → 隐身药水
    addMix(Potions::NIGHT_VISION, Items::FERMENTED_SPIDER_EYE, Potions::INVISIBILITY);

    // 长效夜视药水 + 发酵蛛眼 → 长效隐身药水
    addMix(Potions::LONG_NIGHT_VISION, Items::FERMENTED_SPIDER_EYE, Potions::LONG_INVISIBILITY);

    // ========== 隐身药水升级 ==========
    // 隐身药水 + 红石 → 长效隐身药水
    addMix(Potions::INVISIBILITY, Items::REDSTONE, Potions::LONG_INVISIBILITY);

    // ========== 跳跃提升药水升级 ==========
    // 跳跃药水 + 红石 → 长效跳跃药水
    addMix(Potions::LEAPING, Items::REDSTONE, Potions::LONG_LEAPING);

    // 跳跃药水 + 荧石粉 → 跳跃药水 II
    addMix(Potions::LEAPING, Items::GLOWSTONE_DUST, Potions::STRONG_LEAPING);

    // 跳跃药水 + 发酵蛛眼 → 缓慢药水（MC 1.16.5）
    addMix(Potions::LEAPING, Items::FERMENTED_SPIDER_EYE, Potions::SLOWNESS);

    // 长效跳跃药水 + 发酵蛛眼 → 长效缓慢药水（MC 1.16.5）
    addMix(Potions::LONG_LEAPING, Items::FERMENTED_SPIDER_EYE, Potions::LONG_SLOWNESS);

    // ========== 防火药水升级 ==========
    // 防火药水 + 红石 → 长效防火药水
    addMix(Potions::FIRE_RESISTANCE, Items::REDSTONE, Potions::LONG_FIRE_RESISTANCE);

    // ========== 速度药水升级 ==========
    // 速度药水 + 红石 → 长效速度药水
    addMix(Potions::SWIFTNESS, Items::REDSTONE, Potions::LONG_SWIFTNESS);

    // 速度药水 + 荧石粉 → 速度药水 II
    addMix(Potions::SWIFTNESS, Items::GLOWSTONE_DUST, Potions::STRONG_SWIFTNESS);

    // 速度药水 + 发酵蛛眼 → 缓慢药水
    addMix(Potions::SWIFTNESS, Items::FERMENTED_SPIDER_EYE, Potions::SLOWNESS);

    // 长效速度药水 + 发酵蛛眼 → 长效缓慢药水
    addMix(Potions::LONG_SWIFTNESS, Items::FERMENTED_SPIDER_EYE, Potions::LONG_SLOWNESS);

    // ========== 水下呼吸药水升级 ==========
    // 水下呼吸药水 + 红石 → 长效水下呼吸药水
    addMix(Potions::WATER_BREATHING, Items::REDSTONE, Potions::LONG_WATER_BREATHING);

    // ========== 瞬间治疗药水升级 ==========
    // 治疗药水 + 荧石粉 → 治疗药水 II
    addMix(Potions::HEALING, Items::GLOWSTONE_DUST, Potions::STRONG_HEALING);

    // 治疗药水 + 发酵蛛眼 → 瞬间伤害药水
    addMix(Potions::HEALING, Items::FERMENTED_SPIDER_EYE, Potions::HARMING);

    // 治疗药水 II + 发酵蛛眼 → 瞬间伤害药水 II
    addMix(Potions::STRONG_HEALING, Items::FERMENTED_SPIDER_EYE, Potions::STRONG_HARMING);

    // ========== 中毒药水升级 ==========
    // 中毒药水 + 红石 → 长效中毒药水
    addMix(Potions::POISON, Items::REDSTONE, Potions::LONG_POISON);

    // 中毒药水 + 荧石粉 → 中毒药水 II
    addMix(Potions::POISON, Items::GLOWSTONE_DUST, Potions::STRONG_POISON);

    // 中毒药水 + 发酵蛛眼 → 瞬间伤害药水（MC 1.16.5）
    addMix(Potions::POISON, Items::FERMENTED_SPIDER_EYE, Potions::HARMING);

    // 长效中毒药水 + 发酵蛛眼 → 瞬间伤害药水（MC 1.16.5）
    addMix(Potions::LONG_POISON, Items::FERMENTED_SPIDER_EYE, Potions::HARMING);

    // 强效中毒药水 + 发酵蛛眼 → 瞬间伤害药水 II（MC 1.16.5）
    addMix(Potions::STRONG_POISON, Items::FERMENTED_SPIDER_EYE, Potions::STRONG_HARMING);

    // ========== 生命恢复药水升级 ==========
    // 生命恢复药水 + 红石 → 长效生命恢复药水
    addMix(Potions::REGENERATION, Items::REDSTONE, Potions::LONG_REGENERATION);

    // 生命恢复药水 + 荧石粉 → 生命恢复药水 II
    addMix(Potions::REGENERATION, Items::GLOWSTONE_DUST, Potions::STRONG_REGENERATION);

    // ========== 力量药水升级 ==========
    // 力量药水 + 红石 → 长效力量药水
    addMix(Potions::STRENGTH, Items::REDSTONE, Potions::LONG_STRENGTH);

    // 力量药水 + 荧石粉 → 力量药水 II
    addMix(Potions::STRENGTH, Items::GLOWSTONE_DUST, Potions::STRONG_STRENGTH);

    // ========== 虚弱药水升级 ==========
    // 虚弱药水 + 红石 → 长效虚弱药水
    addMix(Potions::WEAKNESS, Items::REDSTONE, Potions::LONG_WEAKNESS);

    // ========== 海龟大师药水升级 ==========
    // 海龟大师药水 + 红石 → 长效海龟大师药水
    addMix(Potions::TURTLE_MASTER, Items::REDSTONE, Potions::LONG_TURTLE_MASTER);

    // 海龟大师药水 + 荧石粉 → 海龟大师药水 II
    addMix(Potions::TURTLE_MASTER, Items::GLOWSTONE_DUST, Potions::STRONG_TURTLE_MASTER);

    // ========== 缓降药水升级 ==========
    // 缓降药水 + 红石 → 长效缓降药水
    addMix(Potions::SLOW_FALLING, Items::REDSTONE, Potions::LONG_SLOW_FALLING);

    // ========== 缓慢药水升级 ==========
    // 缓慢药水 + 红石 → 长效缓慢药水
    addMix(Potions::SLOWNESS, Items::REDSTONE, Potions::LONG_SLOWNESS);

    // 缓慢药水 + 荧石粉 → 缓慢药水 IV
    addMix(Potions::SLOWNESS, Items::GLOWSTONE_DUST, Potions::STRONG_SLOWNESS);

    // 长效缓慢药水 + 发酵蛛眼 → （无转换）

    // ========== 瞬间伤害药水升级 ==========
    // 伤害药水 + 荧石粉 → 伤害药水 II
    addMix(Potions::HARMING, Items::GLOWSTONE_DUST, Potions::STRONG_HARMING);

    // 伤害药水 + 发酵蛛眼 → （反转治疗药水的转换已在上面）
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

    return hasPotionConversion(PotionUtils::getPotion(potionStack), reagentStack) ||
        hasItemConversion(potionStack.getItem(), reagentStack);
}

ItemStack PotionBrewing::brew(const ItemStack& potionStack, const ItemStack& reagentStack)
{
    if (potionStack.isEmpty()) {
        return potionStack;
    }

    // 尝试容器转换
    const Item* outputItem = doItemConversion(potionStack.getItem(), reagentStack);
    if (outputItem != nullptr) {
        const Potion* potion = PotionUtils::getPotion(potionStack);
        ItemStack result(outputItem);
        PotionUtils::setPotion(result, potion);
        return result;
    }

    // 尝试药水类型转换
    const Potion* outputPotion = doPotionConversion(PotionUtils::getPotion(potionStack), reagentStack);
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

// ========== 私有方法 ==========

void PotionBrewing::addContainer(const Item* item)
{
    if (item != nullptr) {
        s_potionItems.push_back(crafting::Ingredient::fromItem(item));
    }
}

void PotionBrewing::addContainerRecipe(const Item* input, const Item* reagent, const Item* output)
{
    if (input != nullptr && reagent != nullptr && output != nullptr) {
        s_itemMixes.emplace_back(input, crafting::Ingredient::fromItem(reagent), output);
    }
}

void PotionBrewing::addMix(const Potion* input, const Item* reagent, const Potion* output)
{
    if (input != nullptr && reagent != nullptr && output != nullptr) {
        s_potionMixes.emplace_back(input, crafting::Ingredient::fromItem(reagent), output);
    }
}

bool PotionBrewing::hasPotionConversion(const Potion* potion, const ItemStack& reagent)
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

bool PotionBrewing::hasItemConversion(const Item* item, const ItemStack& reagent)
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

const Potion* PotionBrewing::doPotionConversion(const Potion* potion, const ItemStack& reagent)
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

const Item* PotionBrewing::doItemConversion(const Item* item, const ItemStack& reagent)
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
