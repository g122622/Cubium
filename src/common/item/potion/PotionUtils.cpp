#include "PotionUtils.hpp"
#include "PotionRegistry.hpp"
#include "Potions.hpp"
#include "../Items.hpp"
#include "../core/Item.hpp"
#include "../../entity/effect/EffectType.hpp"
#include "../../util/assert/AssertAll.hpp"

namespace mc {
namespace potion {

// ========== PotionUtils 实现 ==========

const Potion* PotionUtils::getPotion(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return Potions::EMPTY;
    }

    // 检查是否为药水类物品
    const Item* item = stack.getItem();
    if (item != Items::POTION && item != Items::SPLASH_POTION && item != Items::LINGERING_POTION) {
        return Potions::EMPTY;
    }

    // 从NBT获取药水类型
    const String& potionId = stack.m_potionId;
    if (potionId.empty()) {
        return Potions::WATER;  // 默认为水瓶
    }

    // 从注册表获取药水
    auto& registry = PotionRegistry::instance();
    const Potion* potion = registry.getPotion(ResourceLocation(potionId));
    return potion != nullptr ? potion : Potions::WATER;
}

std::vector<entity::effect::EffectInstance> PotionUtils::getEffects(const ItemStack& stack) {
    std::vector<entity::effect::EffectInstance> effects;

    // 从药水获取基础效果
    const Potion* potion = getPotion(stack);
    if (potion != nullptr && potion->hasEffects()) {
        const auto& potionEffects = potion->effects();
        effects.insert(effects.end(), potionEffects.begin(), potionEffects.end());
    }

    // TODO: 从NBT获取自定义效果
    // CustomPotionEffects 标签

    return effects;
}

std::vector<entity::effect::EffectInstance> PotionUtils::getEffects(const Potion* potion) {
    if (potion == nullptr) {
        return {};
    }
    return potion->effects();
}

ItemStack PotionUtils::createPotionItem(const Potion* potion) {
    if (potion == nullptr || Items::POTION == nullptr) {
        return ItemStack();
    }

    ItemStack stack(Items::POTION, 1);
    setPotion(stack, potion);
    return stack;
}

ItemStack PotionUtils::createSplashPotionItem(const Potion* potion) {
    if (potion == nullptr || Items::SPLASH_POTION == nullptr) {
        return ItemStack();
    }

    ItemStack stack(Items::SPLASH_POTION, 1);
    setPotion(stack, potion);
    return stack;
}

ItemStack PotionUtils::createLingeringPotionItem(const Potion* potion) {
    if (potion == nullptr || Items::LINGERING_POTION == nullptr) {
        return ItemStack();
    }

    ItemStack stack(Items::LINGERING_POTION, 1);
    setPotion(stack, potion);
    return stack;
}

ItemStack& PotionUtils::setPotion(ItemStack& stack, const Potion* potion) {
    if (stack.isEmpty()) {
        return stack;
    }

    if (potion == nullptr) {
        stack.m_potionId.clear();
        return stack;
    }

    // 设置药水ID
    stack.m_potionId = potion->id().toString();
    return stack;
}

bool PotionUtils::isPotion(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return false;
    }

    const Item* item = stack.getItem();
    return item == Items::POTION ||
           item == Items::SPLASH_POTION ||
           item == Items::LINGERING_POTION;
}

bool PotionUtils::isWaterBottle(const ItemStack& stack) {
    const Potion* potion = getPotion(stack);
    return potion == Potions::WATER;
}

u32 PotionUtils::getColor(const Potion* potion) {
    if (potion == nullptr || !potion->hasEffects()) {
        // 水瓶的颜色
        return 0x385DC6FF;
    }
    return getColor(potion->effects());
}

u32 PotionUtils::getColor(const std::vector<entity::effect::EffectInstance>& effects) {
    if (effects.empty()) {
        return 0x385DC6FF;
    }

    // 计算所有效果颜色的平均值
    f32 r = 0.0f, g = 0.0f, b = 0.0f;
    u32 count = 0;

    for (const auto& effect : effects) {
        u32 effectColor = getEffectColor(effect.type());
        r += static_cast<f32>((effectColor >> 16) & 0xFF);
        g += static_cast<f32>((effectColor >> 8) & 0xFF);
        b += static_cast<f32>(effectColor & 0xFF);
        ++count;
    }

    if (count > 0) {
        r /= count;
        g /= count;
        b /= count;
    }

    return (0xFF << 24) |
           (static_cast<u32>(r) << 16) |
           (static_cast<u32>(g) << 8) |
           static_cast<u32>(b);
}

u32 PotionUtils::getEffectColor(entity::effect::EffectType type) {
    // 返回各种效果的颜色 (RGB)
    // 参考: net.minecraft.potion.Effect
    using namespace entity::effect;
    switch (type) {
        case EffectType::Speed:           return 0x7CAFC6FF;  // 速度 - 淡蓝色
        case EffectType::Slowness:        return 0x5A6C81FF;  // 缓慢 - 灰蓝色
        case EffectType::Haste:           return 0xD9C043FF;  // 急迫 - 黄色
        case EffectType::MiningFatigue:   return 0x4A7210FF;  // 挖掘疲劳 - 暗绿色
        case EffectType::Strength:        return 0x932423FF;  // 力量 - 深红色
        case EffectType::InstantHealth:   return 0xF82423FF;  // 瞬间治疗 - 红色
        case EffectType::InstantDamage:   return 0x430A09FF;  // 瞬间伤害 - 深红棕色
        case EffectType::JumpBoost:       return 0x22FF4CFF;  // 跳跃提升 - 绿色
        case EffectType::Nausea:          return 0x551D4AFF;  // 恶心 - 紫色
        case EffectType::Regeneration:    return 0xCD5CABFF;  // 生命恢复 - 粉红色
        case EffectType::Resistance:      return 0x99453AFF;  // 抗性提升 - 棕色
        case EffectType::FireResistance:  return 0xE49A3AFF;  // 防火 - 橙色
        case EffectType::WaterBreathing:  return 0x2E5299FF;  // 水下呼吸 - 蓝色
        case EffectType::Invisibility:    return 0x7F8392FF;  // 隐身 - 灰色
        case EffectType::Blindness:       return 0x1F1F23FF;  // 失明 - 深灰色
        case EffectType::NightVision:     return 0x1F1FA1FF;  // 夜视 - 深蓝色
        case EffectType::Hunger:          return 0x587653FF;  // 饥饿 - 暗绿色
        case EffectType::Weakness:        return 0x484D48FF;  // 虚弱 - 深灰色
        case EffectType::Poison:          return 0x4E9331FF;  // 中毒 - 绿色
        case EffectType::Wither:          return 0x352A27FF;  // 凋零 - 深灰色
        case EffectType::HealthBoost:     return 0xF87D23FF;  // 生命提升 - 橙色
        case EffectType::Absorption:      return 0x2552A5FF;  // 伤害吸收 - 蓝色
        case EffectType::Saturation:      return 0xF82423FF;  // 饱和 - 红色
        case EffectType::Levitation:      return 0xCEFFFFDFF; // 漂浮 - 青白色
        case EffectType::Luck:            return 0x339900FF;  // 幸运 - 绿色
        case EffectType::BadLuck:         return 0xC0A44DFF;  // 霉运 - 棕黄色
        case EffectType::SlowFalling:     return 0xFEFEFEFF;  // 缓降 - 白色
        case EffectType::ConduitPower:    return 0x1DC2D1FF;  // 潮涌能量 - 青色
        case EffectType::DolphinsGrace:   return 0x9AC0F8FF;  // 海豚的恩惠 - 淡蓝色
        case EffectType::BadOmen:         return 0x0B74B2FF;  // 不祥之兆 - 蓝色
        case EffectType::HeroOfTheVillage:return 0x44FF44FF;  // 村庄英雄 - 绿色
        default: return 0x385DC6FF;  // 默认 - 水瓶颜色
    }
}

} // namespace potion
} // namespace mc
