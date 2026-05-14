#pragma once

#include "../../core/Types.hpp"
#include "../attribute/Attribute.hpp"
#include "../attribute/AttributeModifier.hpp"
#include "../attribute/Attributes.hpp"
#include "EffectType.hpp"
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
 * 参考 MC 1.16.5 Effect.addAttributesModifier()
 */
namespace EffectAttributeModifiers {

// ============================================================================
// 修改器UUID常量（与MC 1.16.5一致）
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
// 跳跃提升
constexpr const char* JUMP_BOOST_UUID = "01CD8E33-6D5F-4B69-8E53-7CAB1BC7A1D8";
// 凋零效果（攻击伤害减少）
constexpr const char* WEAKNESS_UUID = "22653B89-116E-49DC-9B6B-9971489B5BE5";
// 生命提升
constexpr const char* HEALTH_BOOST_UUID = "5D6F0BA2-1186-46AC-B896-C61C5CEE99CC";
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
 * @param amplifier 效果等级
 * @return 属性修改器
 */
[[nodiscard]] attribute::AttributeModifier createModifier(const EffectModifierInfo& info, i32 amplifier);

} // namespace EffectAttributeModifiers

} // namespace effect
} // namespace entity
} // namespace mc
