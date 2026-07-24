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

#include "PotionUtils.hpp"
#include "PotionRegistry.hpp"
#include "Potions.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <optional>

namespace mc {
namespace potion {

// ============================================================================
// 从 ItemStack 读取自定义效果
// ============================================================================

std::vector<entity::effect::EffectInstance> PotionUtils::getCustomEffects(const ItemStack& stack)
{
    std::vector<entity::effect::EffectInstance> effects;

    if (stack.isEmpty()) {
        return effects;
    }

    // 从 JSON 自定义数据中读取
    const nlohmann::json* customData = stack.getTag();
    if (customData == nullptr || !customData->is_object()) {
        return effects;
    }

    auto it = customData->find(NBT_CUSTOM_POTION_EFFECTS);
    if (it == customData->end() || !it->is_array()) {
        return effects;
    }

    const auto& effectsArray = *it;
    for (const auto& effectJson : effectsArray) {
        if (!effectJson.is_object()) {
            continue;
        }

        // 读取效果类型 ID
        if (!effectJson.contains("Id") || !effectJson["Id"].is_number()) {
            continue;
        }
        auto type = static_cast<entity::effect::EffectType>(effectJson["Id"].get<i32>());

        // 读取等级
        i32 amplifier = 0;
        if (effectJson.contains("Amplifier") && effectJson["Amplifier"].is_number()) {
            amplifier = effectJson["Amplifier"].get<i32>();
        }

        // 读取持续时间
        i32 duration = 600; // 默认 30 秒
        if (effectJson.contains("Duration") && effectJson["Duration"].is_number()) {
            duration = effectJson["Duration"].get<i32>();
        }

        // 读取标志
        bool ambient = false;
        if (effectJson.contains("Ambient") && effectJson["Ambient"].is_boolean()) {
            ambient = effectJson["Ambient"].get<bool>();
        }

        bool visible = true;
        if (effectJson.contains("ShowParticles") && effectJson["ShowParticles"].is_boolean()) {
            visible = effectJson["ShowParticles"].get<bool>();
        }

        bool showIcon = true;
        if (effectJson.contains("ShowIcon") && effectJson["ShowIcon"].is_boolean()) {
            showIcon = effectJson["ShowIcon"].get<bool>();
        }

        effects.emplace_back(type, duration, amplifier, ambient, visible, showIcon);
    }

    return effects;
}

// ============================================================================
// 设置自定义效果到 ItemStack
// ============================================================================

ItemStack& PotionUtils::setCustomEffects(ItemStack& stack, const std::vector<entity::effect::EffectInstance>& effects)
{
    if (stack.isEmpty()) {
        return stack;
    }

    if (effects.empty()) {
        // 移除自定义效果
        removeCustomEffects(stack);
        return stack;
    }

    // 获取或创建自定义数据
    nlohmann::json& customData = stack.getOrCreateTag();

    // 创建效果数组
    nlohmann::json effectsArray = nlohmann::json::array();
    for (const auto& effect : effects) {
        nlohmann::json effectJson;
        effectJson["Id"] = static_cast<i32>(effect.type());
        effectJson["Amplifier"] = effect.amplifier();
        effectJson["Duration"] = effect.duration();
        effectJson["Ambient"] = effect.isAmbient();
        effectJson["ShowParticles"] = effect.isVisible();
        effectJson["ShowIcon"] = effect.showIcon();
        effectsArray.push_back(std::move(effectJson));
    }

    customData[NBT_CUSTOM_POTION_EFFECTS] = std::move(effectsArray);
    return stack;
}

// ============================================================================
// 添加单个自定义效果
// ============================================================================

ItemStack& PotionUtils::addCustomEffect(ItemStack& stack, const entity::effect::EffectInstance& effect)
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 获取现有自定义效果
    auto existingEffects = getCustomEffects(stack);

    // 查找是否已有相同类型的效果
    bool merged = false;
    for (auto& existing : existingEffects) {
        if (existing.type() == effect.type()) {
            // 合并效果
            existing.merge(effect);
            merged = true;
            break;
        }
    }

    // 如果没有合并，添加新效果
    if (!merged) {
        existingEffects.push_back(effect);
    }

    // 设置回 ItemStack
    return setCustomEffects(stack, existingEffects);
}

// ============================================================================
// 移除所有自定义效果
// ============================================================================

ItemStack& PotionUtils::removeCustomEffects(ItemStack& stack)
{
    if (stack.isEmpty()) {
        return stack;
    }

    nlohmann::json* customData = stack.getTag();
    if (customData != nullptr && customData->is_object()) {
        customData->erase(NBT_CUSTOM_POTION_EFFECTS);
    }

    return stack;
}

// ============================================================================
// 检查是否有自定义效果
// ============================================================================

bool PotionUtils::hasCustomEffects(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }

    const nlohmann::json* customData = stack.getTag();
    if (customData == nullptr || !customData->is_object()) {
        return false;
    }

    auto it = customData->find(NBT_CUSTOM_POTION_EFFECTS);
    return it != customData->end() && it->is_array() && !it->empty();
}

// ============================================================================
// 获取自定义药水颜色
// ============================================================================

std::optional<u32> PotionUtils::getCustomPotionColor(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return std::nullopt;
    }

    const nlohmann::json* customData = stack.getTag();
    if (customData == nullptr || !customData->is_object()) {
        return std::nullopt;
    }

    auto it = customData->find(NBT_CUSTOM_POTION_COLOR);
    if (it == customData->end() || !it->is_number()) {
        return std::nullopt;
    }

    return static_cast<u32>(it->get<i32>());
}

// ============================================================================
// 设置自定义药水颜色
// ============================================================================

ItemStack& PotionUtils::setCustomPotionColor(ItemStack& stack, std::optional<u32> color)
{
    if (stack.isEmpty()) {
        return stack;
    }

    if (!color.has_value()) {
        // 移除自定义颜色
        nlohmann::json* customData = stack.getTag();
        if (customData != nullptr && customData->is_object()) {
            customData->erase(NBT_CUSTOM_POTION_COLOR);
        }
        return stack;
    }

    // 设置自定义颜色
    nlohmann::json& customData = stack.getOrCreateTag();
    customData[NBT_CUSTOM_POTION_COLOR] = static_cast<i32>(color.value());
    return stack;
}

// ============================================================================
// 获取 ItemStack 的药水颜色
// ============================================================================

u32 PotionUtils::getColor(const ItemStack& stack)
{
    // 优先使用自定义颜色
    auto customColor = getCustomPotionColor(stack);
    if (customColor.has_value()) {
        return customColor.value();
    }

    // 计算效果颜色的平均值
    auto effects = getEffects(stack);
    return getColor(effects);
}

// ============================================================================
// 从 ItemStack 获取效果列表（包含基础效果和自定义效果）
// ============================================================================

std::vector<entity::effect::EffectInstance> PotionUtils::getEffects(const ItemStack& stack)
{
    std::vector<entity::effect::EffectInstance> effects;

    // 从药水获取基础效果
    const Potion* potion = getPotion(stack);
    if (potion != nullptr && potion->hasEffects()) {
        const auto& potionEffects = potion->effects();
        effects.insert(effects.end(), potionEffects.begin(), potionEffects.end());
    }

    // 从 NBT 获取自定义效果
    auto customEffects = getCustomEffects(stack);
    effects.insert(effects.end(), customEffects.begin(), customEffects.end());

    return effects;
}

// ============================================================================
// 从药水获取效果列表
// ============================================================================

std::vector<entity::effect::EffectInstance> PotionUtils::getEffects(const Potion* potion)
{
    if (potion == nullptr) {
        return {};
    }
    return potion->effects();
}

// ============================================================================
// 从 ItemStack 获取药水
// ============================================================================

const Potion* PotionUtils::getPotion(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return Potions::EMPTY;
    }

    // 检查是否为药水类物品
    const Item* item = stack.getItem();
    if (item != Items::POTION && item != Items::SPLASH_POTION && item != Items::LINGERING_POTION) {
        return Potions::EMPTY;
    }

    // 从NBT获取药水类型
    const std::string& potionId = stack.getPotionId();
    if (potionId.empty()) {
        return Potions::WATER; // 默认为水瓶
    }

    // 从注册表获取药水
    auto& registry = PotionRegistry::instance();
    const Potion* potion = registry.getPotion(ResourceLocation(potionId));
    return potion != nullptr ? potion : Potions::WATER;
}

// ============================================================================
// 创建药水物品
// ============================================================================

ItemStack PotionUtils::createPotionItem(const Potion* potion)
{
    if (potion == nullptr || Items::POTION == nullptr) {
        return ItemStack();
    }

    ItemStack stack(Items::POTION, 1);
    setPotion(stack, potion);
    return stack;
}

ItemStack PotionUtils::createSplashPotionItem(const Potion* potion)
{
    if (potion == nullptr || Items::SPLASH_POTION == nullptr) {
        return ItemStack();
    }

    ItemStack stack(Items::SPLASH_POTION, 1);
    setPotion(stack, potion);
    return stack;
}

ItemStack PotionUtils::createLingeringPotionItem(const Potion* potion)
{
    if (potion == nullptr || Items::LINGERING_POTION == nullptr) {
        return ItemStack();
    }

    ItemStack stack(Items::LINGERING_POTION, 1);
    setPotion(stack, potion);
    return stack;
}

// ============================================================================
// 设置药水
// ============================================================================

ItemStack& PotionUtils::setPotion(ItemStack& stack, const Potion* potion)
{
    if (stack.isEmpty()) {
        return stack;
    }

    if (potion == nullptr) {
        stack.setPotionId(std::string{});
        return stack;
    }

    // 设置药水ID
    stack.setPotionId(potion->id().toString());
    return stack;
}

// ============================================================================
// 检查是否为药水
// ============================================================================

bool PotionUtils::isPotion(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }

    const Item* item = stack.getItem();
    return item == Items::POTION || item == Items::SPLASH_POTION || item == Items::LINGERING_POTION;
}

// ============================================================================
// 检查是否为水瓶
// ============================================================================

bool PotionUtils::isWaterBottle(const ItemStack& stack)
{
    const Potion* potion = getPotion(stack);
    return potion == Potions::WATER;
}

// ============================================================================
// 获取药水颜色
// ============================================================================

u32 PotionUtils::getColor(const Potion* potion)
{
    if (potion == nullptr || !potion->hasEffects()) {
        // 水瓶的颜色
        return 0x385DC6FF;
    }
    return getColor(potion->effects());
}

// ============================================================================
// 获取效果列表的颜色
// ============================================================================

u32 PotionUtils::getColor(const std::vector<entity::effect::EffectInstance>& effects)
{
    if (effects.empty()) {
        return 0x385DC6FF;
    }

    // 计算所有效果颜色的平均值
    // 颜色格式为 ARGB
    f32 a = 0.0f, r = 0.0f, g = 0.0f, b = 0.0f;
    u32 count = 0;

    for (const auto& effect : effects) {
        u32 effectColor = getEffectColor(effect.type());
        a += static_cast<f32>((effectColor >> 24) & 0xFF);
        r += static_cast<f32>((effectColor >> 16) & 0xFF);
        g += static_cast<f32>((effectColor >> 8) & 0xFF);
        b += static_cast<f32>(effectColor & 0xFF);
        ++count;
    }

    if (count > 0) {
        a /= count;
        r /= count;
        g /= count;
        b /= count;
    }

    return (static_cast<u32>(a) << 24) | (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | static_cast<u32>(b);
}

// ============================================================================
// 获取单个效果的颜色
// ============================================================================

u32 PotionUtils::getEffectColor(entity::effect::EffectType type)
{
    // 返回各种效果的颜色 (RGB)
    using namespace entity::effect;
    switch (type) {
        case EffectType::Speed:
            return 0x7CAFC6FF; // 速度 - 淡蓝色
        case EffectType::Slowness:
            return 0x5A6C81FF; // 缓慢 - 灰蓝色
        case EffectType::Haste:
            return 0xD9C043FF; // 急迫 - 黄色
        case EffectType::MiningFatigue:
            return 0x4A7210FF; // 挖掘疲劳 - 暗绿色
        case EffectType::Strength:
            return 0x932423FF; // 力量 - 深红色
        case EffectType::InstantHealth:
            return 0xF82423FF; // 瞬间治疗 - 红色
        case EffectType::InstantDamage:
            return 0x430A09FF; // 瞬间伤害 - 深红棕色
        case EffectType::JumpBoost:
            return 0x22FF4CFF; // 跳跃提升 - 绿色
        case EffectType::Nausea:
            return 0x551D4AFF; // 恶心 - 紫色
        case EffectType::Regeneration:
            return 0xCD5CABFF; // 生命恢复 - 粉红色
        case EffectType::Resistance:
            return 0x99453AFF; // 抗性提升 - 棕色
        case EffectType::FireResistance:
            return 0xE49A3AFF; // 防火 - 橙色
        case EffectType::WaterBreathing:
            return 0x2E5299FF; // 水下呼吸 - 蓝色
        case EffectType::Invisibility:
            return 0x7F8392FF; // 隐身 - 灰色
        case EffectType::Blindness:
            return 0x1F1F23FF; // 失明 - 深灰色
        case EffectType::NightVision:
            return 0x1F1FA1FF; // 夜视 - 深蓝色
        case EffectType::Hunger:
            return 0x587653FF; // 饥饿 - 暗绿色
        case EffectType::Weakness:
            return 0x484D48FF; // 虚弱 - 深灰色
        case EffectType::Poison:
            return 0x4E9331FF; // 中毒 - 绿色
        case EffectType::Wither:
            return 0x352A27FF; // 凋零 - 深灰色
        case EffectType::HealthBoost:
            return 0xF87D23FF; // 生命提升 - 橙色
        case EffectType::Absorption:
            return 0x2552A5FF; // 伤害吸收 - 蓝色
        case EffectType::Saturation:
            return 0xF82423FF; // 饱和 - 红色
        case EffectType::Levitation:
            return 0xFFCEFFFF; // 漂浮 - 青白色
        case EffectType::Luck:
            return 0x339900FF; // 幸运 - 绿色
        case EffectType::BadLuck:
            return 0xC0A44DFF; // 霉运 - 棕黄色
        case EffectType::SlowFalling:
            return 0xFEFEFEFF; // 缓降 - 白色
        case EffectType::ConduitPower:
            return 0x1DC2D1FF; // 潮涌能量 - 青色
        case EffectType::DolphinsGrace:
            return 0x9AC0F8FF; // 海豚的恩惠 - 淡蓝色
        case EffectType::BadOmen:
            return 0x0B74B2FF; // 不祥之兆 - 蓝色
        case EffectType::HeroOfTheVillage:
            return 0x44FF44FF; // 村庄英雄 - 绿色
        default:
            return 0x385DC6FF; // 默认 - 水瓶颜色
    }
}

} // namespace potion
} // namespace mc
