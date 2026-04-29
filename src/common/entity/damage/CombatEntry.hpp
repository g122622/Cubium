/**
 * @file CombatEntry.hpp
 * @brief 战斗条目 - 记录单次伤害事件
 *
 * 参考 MC 1.16.5 CombatEntry
 */

#pragma once

#include "DamageSource.hpp"
#include "../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 战斗条目
 *
 * 记录一次伤害事件的详细信息，用于生成死亡消息。
 *
 * MC 1.16.5 字段：
 * - damageSrc: 伤害来源
 * - time: 发生时间
 * - health: 受伤前生命值
 * - damageAmount: 伤害量
 * - fallSuffix: 摔落后缀
 * - fallDistance: 摔落距离
 */
class CombatEntry {
public:
    /**
     * @brief 构造函数
     * @param source 伤害来源（会被移动）
     * @param damage 伤害值
     * @param timestamp 发生时间（tick）
     * @param health 受伤前生命值
     * @param fallSuffix 摔落后缀（如 "fall" 或 "ladder"）
     * @param fallDistance 摔落距离
     */
    CombatEntry(std::unique_ptr<DamageSource> source, f32 damage, i32 timestamp,
                f32 health = 0.0f, const String& fallSuffix = "", f32 fallDistance = 0.0f);

    /**
     * @brief 获取伤害来源
     */
    [[nodiscard]] const DamageSource* source() const { return m_source.get(); }

    /**
     * @brief 获取伤害值
     */
    [[nodiscard]] f32 damage() const { return m_damage; }

    /**
     * @brief 获取发生时间
     */
    [[nodiscard]] i32 timestamp() const { return m_timestamp; }

    /**
     * @brief 获取受伤前生命值
     */
    [[nodiscard]] f32 health() const { return m_health; }

    /**
     * @brief 获取摔落后缀
     */
    [[nodiscard]] const String& fallSuffix() const { return m_fallSuffix; }

    /**
     * @brief 获取摔落距离
     */
    [[nodiscard]] f32 fallDistance() const { return m_fallDistance; }

    /**
     * @brief 是否来自生物
     */
    [[nodiscard]] bool isLivingSource() const;

    /**
     * @brief 是否来自玩家
     */
    [[nodiscard]] bool isPlayerSource() const;

    /**
     * @brief 获取伤害量（用于摔落计算）
     *
     * 虚空伤害返回 FLT_MAX
     */
    [[nodiscard]] f32 getDamageAmount() const;

private:
    std::unique_ptr<DamageSource> m_source;
    f32 m_damage;
    i32 m_timestamp;
    f32 m_health;           // 受伤前生命值
    String m_fallSuffix;    // 摔落后缀（如 "fall", "ladder"）
    f32 m_fallDistance;     // 摔落距离
};

} // namespace mc
