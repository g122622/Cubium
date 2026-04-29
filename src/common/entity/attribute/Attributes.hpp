#pragma once

#include "Attribute.hpp"
#include <memory>
#include <unordered_map>

namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 标准属性定义
 *
 * 定义游戏中的标准属性类型。
 * 这些属性可以通过修改器进行修改。
 *
 * 参考 MC 1.16.5 Attributes
 */
namespace Attributes {

/**
 * @brief 最大生命值
 *
 * 决定实体可以承受的伤害量。
 * 默认值: 20.0 (玩家)
 * 范围: 0.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> maxHealth() {
    return std::make_unique<Attribute>("generic.max_health", 20.0, 0.0, 1024.0);
}

/**
 * @brief 跟随范围
 *
 * 决定实体追踪目标的距离。
 * 默认值: 32.0 (大多数生物)
 * 范围: 0.0 ~ 2048.0
 */
inline std::unique_ptr<Attribute> followRange() {
    return std::make_unique<Attribute>("generic.follow_range", 32.0, 0.0, 2048.0);
}

/**
 * @brief 击退抗性
 *
 * 决定实体被击退的概率。
 * 默认值: 0.0
 * 范围: 0.0 ~ 1.0
 * 1.0 表示完全免疫击退
 */
inline std::unique_ptr<Attribute> knockbackResistance() {
    return std::make_unique<Attribute>("generic.knockback_resistance", 0.0, 0.0, 1.0);
}

/**
 * @brief 移动速度
 *
 * 决定实体的移动速度。
 * 默认值: 0.7 (生物), 0.1 (玩家)
 * 范围: 0.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> movementSpeed() {
    return std::make_unique<Attribute>("generic.movement_speed", 0.7, 0.0, 1024.0);
}

/**
 * @brief 飞行速度
 *
 * 决定实体的飞行速度。
 * 默认值: 0.4 (飞行玩家)
 * 范围: 0.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> flyingSpeed() {
    return std::make_unique<Attribute>("generic.flying_speed", 0.4, 0.0, 1024.0);
}

/**
 * @brief 攻击伤害
 *
 * 决定实体的近战攻击伤害。
 * 默认值: 2.0 (玩家)
 * 范围: 0.0 ~ 2048.0
 */
inline std::unique_ptr<Attribute> attackDamage() {
    return std::make_unique<Attribute>("generic.attack_damage", 2.0, 0.0, 2048.0);
}

/**
 * @brief 攻击击退
 *
 * 决定实体攻击时的击退力度。
 * 默认值: 0.0
 * 范围: 0.0 ~ 5.0
 */
inline std::unique_ptr<Attribute> attackKnockback() {
    return std::make_unique<Attribute>("generic.attack_knockback", 0.0, 0.0, 5.0);
}

/**
 * @brief 攻击速度
 *
 * 决定实体的攻击速度（每秒攻击次数）。
 * 默认值: 4.0 (玩家)
 * 范围: 0.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> attackSpeed() {
    return std::make_unique<Attribute>("generic.attack_speed", 4.0, 0.0, 1024.0);
}

/**
 * @brief 护甲值
 *
 * 决定实体的护甲防御值。
 * 默认值: 0.0
 * 范围: 0.0 ~ 30.0
 */
inline std::unique_ptr<Attribute> armor() {
    return std::make_unique<Attribute>("generic.armor", 0.0, 0.0, 30.0);
}

/**
 * @brief 护甲韧性
 *
 * 决定护甲减少伤害的效果。
 * 默认值: 0.0
 * 范围: 0.0 ~ 20.0
 */
inline std::unique_ptr<Attribute> armorToughness() {
    return std::make_unique<Attribute>("generic.armor_toughness", 0.0, 0.0, 20.0);
}

/**
 * @brief 幸运
 *
 * 影响各种随机事件的结果。
 * 默认值: 0.0
 * 范围: -1024.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> luck() {
    return std::make_unique<Attribute>("generic.luck", 0.0, -1024.0, 1024.0);
}

/**
 * @brief 最大吸收值
 *
 * 决定实体可以拥有的吸收生命值上限。
 * 默认值: 0.0
 * 范围: 0.0 ~ 2048.0
 */
inline std::unique_ptr<Attribute> maxAbsorption() {
    return std::make_unique<Attribute>("generic.max_absorption", 0.0, 0.0, 2048.0);
}

/**
 * @brief 水下呼吸时间
 *
 * 决定实体可以在水下呼吸的时间（ticks）。
 * 默认值: 300 (15秒)
 * 范围: 0 ~ 6000
 */
inline std::unique_ptr<Attribute> breathMax() {
    // 注意：使用整数属性，这里用 double 表示
    return std::make_unique<Attribute>("generic.breath_max", 300.0, 0.0, 6000.0);
}

/**
 * @brief 跳跃高度
 *
 * 决定实体的跳跃力度。
 * 默认值: 0.42
 * 范围: 0.0 ~ 8.0
 */
inline std::unique_ptr<Attribute> jumpBoost() {
    return std::make_unique<Attribute>("generic.jump_boost", 0.42, 0.0, 8.0);
}

/**
 * @brief 马匹跳跃强度
 *
 * 决定马类实体的跳跃能力。
 * 默认值: 0.7
 * 范围: 0.0 ~ 2.0
 */
inline std::unique_ptr<Attribute> horseJumpStrength() {
    return std::make_unique<Attribute>("horse.jump_strength", 0.7, 0.0, 2.0);
}

/**
 * @brief 实体重力
 *
 * 决定实体受到的重力加速度（blocks/tick²）。
 * 默认值: 0.08 (MC 标准)
 * 范围: 0.0 ~ 1.0
 * 注意：缓降药水会修改此属性
 */
inline std::unique_ptr<Attribute> entityGravity() {
    return std::make_unique<Attribute>("generic.gravity", 0.08, 0.0, 1.0);
}

/**
 * @brief 游泳速度
 *
 * 决定实体在水中的游泳速度。
 * 默认值: 1.0
 * 范围: 0.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> swimSpeed() {
    return std::make_unique<Attribute>("generic.swim_speed", 1.0, 0.0, 1024.0);
}

// ============================================================================
// 属性名称常量
// ============================================================================

constexpr const char* MAX_HEALTH = "generic.max_health";
constexpr const char* FOLLOW_RANGE = "generic.follow_range";
constexpr const char* KNOCKBACK_RESISTANCE = "generic.knockback_resistance";
constexpr const char* MOVEMENT_SPEED = "generic.movement_speed";
constexpr const char* FLYING_SPEED = "generic.flying_speed";
constexpr const char* ATTACK_DAMAGE = "generic.attack_damage";
constexpr const char* ATTACK_KNOCKBACK = "generic.attack_knockback";
constexpr const char* ATTACK_SPEED = "generic.attack_speed";
constexpr const char* ARMOR = "generic.armor";
constexpr const char* ARMOR_TOUGHNESS = "generic.armor_toughness";
constexpr const char* LUCK = "generic.luck";
constexpr const char* MAX_ABSORPTION = "generic.max_absorption";
constexpr const char* BREATH_MAX = "generic.breath_max";
constexpr const char* JUMP_BOOST = "generic.jump_boost";
constexpr const char* JUMP_STRENGTH = "horse.jump_strength";
constexpr const char* ENTITY_GRAVITY = "generic.gravity";
constexpr const char* SWIM_SPEED = "generic.swim_speed";

} // namespace Attributes

} // namespace attribute
} // namespace entity
} // namespace mc
