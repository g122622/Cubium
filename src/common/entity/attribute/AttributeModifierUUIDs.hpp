#pragma once

#include "../../core/Types.hpp"
#include <string>

namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 属性修改器 UUID 常量
 *
 * 定义 MC 1.16.5 标准的属性修改器 UUID。
 * 这些 UUID 用于标识装备和状态效果提供的属性修改器。
 *
 * 参考: MC 1.16.5 Item.java, LivingEntity.java
 */
namespace uuids {

// ============================================================================
// 装备槽位 UUID (MC 1.16.5 标准 - ArmorItem.java:28)
// 数组索引顺序: FEET(0), LEGS(1), CHEST(2), HEAD(3)
// ============================================================================

/// 靴子护甲修饰器 UUID (FEET - 索引0)
constexpr const char* ARMOR_MODIFIER_UUID_FEET = "845DB27C-C624-495F-8C9F-6020A9A58B6B";

/// 护腿护甲修饰器 UUID (LEGS - 索引1)
constexpr const char* ARMOR_MODIFIER_UUID_LEGS = "D8499B04-0E66-4726-AB29-64469D734E0D";

/// 胸甲护甲修饰器 UUID (CHEST - 索引2)
constexpr const char* ARMOR_MODIFIER_UUID_CHEST = "9F3D476D-C118-4544-8365-64846904B48E";

/// 头盔护甲修饰器 UUID (HEAD - 索引3)
constexpr const char* ARMOR_MODIFIER_UUID_HEAD = "2AD3F246-FEE1-4E67-B886-69FD380BB150";

// ============================================================================
// 武器 UUID (MC 1.16.5 标准)
// ============================================================================

/// 武器攻击伤害修饰器 UUID
constexpr const char* ATTACK_DAMAGE_MODIFIER_UUID = "CB3F55D3-645C-4F38-A497-9C13A33DB5CF";

/// 武器攻击速度修饰器 UUID
constexpr const char* ATTACK_SPEED_MODIFIER_UUID = "FA233E1C-4180-4865-B01B-BCCE9785ACA3";

// ============================================================================
// 状态效果 UUID (MC 1.16.5 标准)
// ============================================================================

/// 疾跑速度加成修饰器 UUID
constexpr const char* SPRINTING_SPEED_BOOST_UUID = "662A6B8D-DA3E-4C1C-8813-96EA6097278D";

/// 缓降效果修饰器 UUID
constexpr const char* SLOW_FALLING_UUID = "A5B6CF2A-2F7C-31F5-9035-7078DCEA8A67";

/// 速度效果修饰器 UUID
constexpr const char* SPEED_BOOST_UUID = "91AEAA56-376B-4498-935B-2F7F68070635";

/// 缓慢效果修饰器 UUID
constexpr const char* SLOWNESS_UUID = "7107DE5E-7CE8-4030-803E-C3F48F5E794D";

/// 急迫效果修饰器 UUID
constexpr const char* HASTE_UUID = "AF8B6E3F-3328-4C0A-AA36-5BA2BB9DBEF3";

/// 挖掘疲劳效果修饰器 UUID
constexpr const char* MINING_FATIGUE_UUID = "55FCED67-E92A-486E-9800-B47F202C4386";

/// 力量效果修饰器 UUID
constexpr const char* STRENGTH_UUID = "648D7064-6A60-4F59-8ABE-C2C23A6DD7A9";

/// 跳跃提升效果修饰器 UUID
constexpr const char* JUMP_BOOST_UUID = "202C495C-68E2-487F-821D-6B57E97F5D2A";

/// 虚弱效果修饰器 UUID
constexpr const char* WEAKNESS_UUID = "22653B89-116E-49DC-9B6B-9971489B5BE5";

/// 生命提升效果修饰器 UUID
constexpr const char* HEALTH_BOOST_UUID = "0D2B1C3D-6E3E-4B5E-9A3D-5D5A6B7C8D9E";

/// 幸运效果修饰器 UUID
constexpr const char* LUCK_UUID = "03C3C89D-7037-4B42-869F-B735591E2D3E";

/// 霉运效果修饰器 UUID
constexpr const char* UNLUCK_UUID = "CC5AF142-2BD2-4215-B636-2605A17BEFD1";

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 从字符串创建 UUID
 * @param uuidStr UUID 字符串
 * @return String 类型的 UUID
 */
inline String fromString(const char* uuidStr) {
    return String(uuidStr);
}

} // namespace uuids
} // namespace attribute
} // namespace entity
} // namespace mc
