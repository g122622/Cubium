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

#include "EffectAttributeModifiers.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/effect/EffectType.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {
namespace entity {
namespace effect {
namespace EffectAttributeModifiers {

// ============================================================================
// 效果到修改器映射
// ============================================================================

namespace {

/// 效果属性修改器映射表
const std::unordered_map<EffectType, std::vector<EffectModifierInfo>> s_effectModifiers = {
    // 速度：每级增加 20% 移动速度
    {EffectType::Speed,
        {{attribute::Attributes::MOVEMENT_SPEED, SPEED_UUID, 0.2, attribute::Operation::MultiplyTotal}}},
    // 缓慢：每级减少 15% 移动速度
    {EffectType::Slowness,
        {{attribute::Attributes::MOVEMENT_SPEED, SLOWNESS_UUID, -0.15, attribute::Operation::MultiplyTotal}}},
    // 急迫：每级增加 10% 攻击速度
    {EffectType::Haste, {{attribute::Attributes::ATTACK_SPEED, HASTE_UUID, 0.1, attribute::Operation::MultiplyTotal}}},
    // 挖掘疲劳：每级减少 10% 攻击速度
    {EffectType::MiningFatigue,
        {{attribute::Attributes::ATTACK_SPEED, MINING_FATIGUE_UUID, -0.1, attribute::Operation::MultiplyTotal}}},
    // 力量：每级增加 3.0 攻击伤害
    {EffectType::Strength,
        {{attribute::Attributes::ATTACK_DAMAGE, STRENGTH_UUID, 3.0, attribute::Operation::Addition}}},
    // 跳跃提升：每级增加 0.1 跳跃力
    {EffectType::JumpBoost,
        {{attribute::Attributes::JUMP_BOOST, JUMP_BOOST_UUID, 0.1, attribute::Operation::Addition}}},
    // 虚弱：每级减少 4.0 攻击伤害
    {EffectType::Weakness,
        {{attribute::Attributes::ATTACK_DAMAGE, WEAKNESS_UUID, -4.0, attribute::Operation::Addition}}},
    // 生命提升：每级增加 4.0 最大生命值
    {EffectType::HealthBoost,
        {{attribute::Attributes::MAX_HEALTH, HEALTH_BOOST_UUID, 4.0, attribute::Operation::Addition}}},
    // 伤害吸收：每级增加 4.0 最大吸收值
    {EffectType::Absorption,
        {{attribute::Attributes::MAX_ABSORPTION, ABSORPTION_UUID, 4.0, attribute::Operation::Addition}}},
    // 幸运：每级增加 1.0 幸运值
    {EffectType::Luck, {{attribute::Attributes::LUCK, LUCK_UUID, 1.0, attribute::Operation::Addition}}},
    // 霉运：每级减少 1.0 幸运值
    {EffectType::BadLuck, {{attribute::Attributes::LUCK, BAD_LUCK_UUID, -1.0, attribute::Operation::Addition}}},
    // 缓降：无属性修改（只有逻辑效果：减少摔落速度）
    // 潮涌能量：无属性修改（逻辑效果：水下呼吸+挖掘速度+视野）
    // 海豚的恩惠：无属性修改（逻辑效果：增加游泳速度）
    // 抗性提升：无属性修改（逻辑效果：减少伤害）
    // 防火：无属性修改（逻辑效果：免疫火焰）
    // 水下呼吸：无属性修改（逻辑效果：增加氧气时间）
    // 隐身：无属性修改（逻辑效果：减少敌对生物检测范围）
    // 夜视：无属性修改（逻辑效果：增加水下/暗处视野）
    // 饱和：无属性修改（瞬间效果：恢复饥饿值）
    // 漂浮：无属性修改（逻辑效果：向上漂浮）
    // 不祥之兆：无属性修改（逻辑效果：触发袭击）
    // 村庄英雄：无属性修改（逻辑效果：交易折扣）
};

// 空列表（用于没有修改器的效果）
const std::vector<EffectModifierInfo> s_emptyModifiers;

} // namespace

// ============================================================================
// 实现
// ============================================================================

const std::vector<EffectModifierInfo>& getEffectModifiers(EffectType type)
{
    auto it = s_effectModifiers.find(type);
    if (it != s_effectModifiers.end()) {
        return it->second;
    }
    return s_emptyModifiers;
}

bool hasAttributeModifiers(EffectType type)
{
    auto it = s_effectModifiers.find(type);
    return it != s_effectModifiers.end() && !it->second.empty();
}

attribute::AttributeModifier createModifier(const EffectModifierInfo& info, EffectType type, i32 amplifier)
{
    f64 amount = info.calculateAmount(amplifier);
    // 使用 MC 原版命名格式: effect.minecraft.<resource_name>.<level>
    std::string name =
        std::string("effect.minecraft.") + getEffectResourceName(type) + "." + std::to_string(amplifier + 1);
    return attribute::AttributeModifier(std::string(info.uuid), name, amount, info.operation);
}

} // namespace EffectAttributeModifiers
} // namespace effect
} // namespace entity
} // namespace mc
