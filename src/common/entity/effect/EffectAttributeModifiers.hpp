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

#include "core/Types.hpp"
#include "entity/attribute/Attribute.hpp"
#include "entity/attribute/AttributeModifier.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/effect/EffectType.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {
namespace entity {
namespace effect {

/**
 * @brief 效果属性修改器定义
 *
 * 定义每种效果对应的属性修改器。
 */
namespace EffectAttributeModifiers {

// ============================================================================
// 修改器UUID常量（与MC原版一致，来自AttributeModifierIdFix数据映射）
// ============================================================================

// 速度效果
constexpr const char* SPEED_UUID = "91AEAA56-376B-4498-935B-2F7F68070635";
// 缓慢效果
constexpr const char* SLOWNESS_UUID = "7107DE5E-7CE8-4030-940E-514C1F160890";
// 急迫效果
constexpr const char* HASTE_UUID = "AF8B6E3F-3328-4C0A-AA36-5BA2BB9DBEF3";
// 挖掘疲劳
constexpr const char* MINING_FATIGUE_UUID = "55FCED67-E92A-486E-9800-B47F202C4386";
// 力量效果
constexpr const char* STRENGTH_UUID = "648D7064-6A60-4F59-8ABE-C2C23A6DD7A9";
// 跳跃提升——安全摔落距离修饰符（每级 +1，MobEffects.JUMP_BOOST 仅挂此修饰符）
constexpr const char* JUMP_BOOST_SAFE_FALL_UUID = "C0105BF3-AEF8-46B0-9EBC-92943757CCBF";
// 虚弱效果
constexpr const char* WEAKNESS_UUID = "22653B89-116E-49DC-9B6B-9971489B5BE5";
// 生命提升
constexpr const char* HEALTH_BOOST_UUID = "5D6F0BA2-1186-46AC-B896-C61C5CEE99CC";
// 伤害吸收
constexpr const char* ABSORPTION_UUID = "EAE29CF0-701E-4ED6-883A-96F798F3DAB5";
// 幸运
constexpr const char* LUCK_UUID = "03C3C89D-7037-4B42-869F-B146BCB64D2E";
// 霉运
constexpr const char* BAD_LUCK_UUID = "CC5AF142-2BD2-4215-B636-2605AED11727";

// ============================================================================
// 效果属性修改器信息
// ============================================================================

/**
 * @brief 效果属性修改器信息
 */
struct EffectModifierInfo {
    const char* attributeName;      // 属性名称
    const char* uuid;               // 修改器UUID
    f64 baseAmount;                 // 基础修改量
    attribute::Operation operation; // 操作类型

    /**
     * @brief 计算实际修改量
     * @param amplifier 效果等级（0-based）
     * @return 实际修改量
     */
    [[nodiscard]] f64 calculateAmount(i32 amplifier) const
    {
        // MC 1.16.5: 修改量 = baseAmount * (amplifier + 1)
        return baseAmount * static_cast<f64>(amplifier + 1);
    }
};

/**
 * @brief 获取效果的属性修改器列表
 * @param type 效果类型
 * @return 属性修改器列表（可能为空）
 */
[[nodiscard]] const std::vector<EffectModifierInfo>& getEffectModifiers(EffectType type);

/**
 * @brief 检查效果是否有属性修改器
 * @param type 效果类型
 */
[[nodiscard]] bool hasAttributeModifiers(EffectType type);

/**
 * @brief 创建属性修改器
 * @param info 修改器信息
 * @param type 效果类型（用于生成修改器名称）
 * @param amplifier 效果等级（0-based）
 * @return 属性修改器
 */
[[nodiscard]] attribute::AttributeModifier createModifier(
    const EffectModifierInfo& info, EffectType type, i32 amplifier);

} // namespace EffectAttributeModifiers

} // namespace effect
} // namespace entity
} // namespace mc
