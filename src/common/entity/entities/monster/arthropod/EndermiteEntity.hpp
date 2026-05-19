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

#include "../../../../core/Types.hpp"
#include "../MonsterEntity.hpp"

namespace mc {

// Forward declarations
namespace entity::ai::goal {
class SilverfishSummonOthersGoal;
}

/**
 * @brief 末影螨实体
 *
 * 末影人瞬移时有概率生成的敌对小生物。
 * 非持久化的末影螨会在约2分钟（2400 ticks）后自动消失。
 *
 * 参考 MC 1.16.5 EndermiteEntity
 */
class EndermiteEntity : public MonsterEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    EndermiteEntity(EntityId id);
    ~EndermiteEntity() override = default;

    // ========== 生命周期 ==========

    void tick() override;

    // ========== 末影螨特有 ==========

    /**
     * @brief 检查是否由玩家生成
     * MC 1.16.5: isSpawnedByPlayer()
     */
    [[nodiscard]] bool isSpawnedByPlayer() const { return m_playerSpawned; }

    /**
     * @brief 设置是否由玩家生成
     * MC 1.16.5: setSpawnedByPlayer()
     */
    void setSpawnedByPlayer(bool playerSpawned) { m_playerSpawned = playerSpawned; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    i32 m_lifetime = 0;           // 存活时间（ticks）
    bool m_playerSpawned = false; // 是否由玩家生成

    // 末影螨消失时间：2400 ticks = 120秒 = 2分钟
    static constexpr i32 DESPAWN_TIME = 2400;
};

/**
 * @brief 蠹虫实体
 *
 * 生成于要塞的敌对小生物，可以唤起更多蠹虫。
 * 当受到伤害时会召唤周围虫蚀方块中的蠹虫。
 *
 * 参考 MC 1.16.5 SilverfishEntity
 */
class SilverfishEntity : public MonsterEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    SilverfishEntity(EntityId id);
    ~SilverfishEntity() override = default;

    // ========== 生命周期 ==========

    /**
     * @brief 重写tick以处理特殊行为
     * MC 1.16.5: 同步renderYawOffset和rotationYaw
     */
    void tick() override;

    /**
     * @brief 重写受伤以触发召唤同伴
     * MC 1.16.5: 受伤时通知SummonSilverfishGoal
     */
    bool hurt(DamageSource& source, f32 amount) override;

    // ========== AI 目标访问 ==========

    /**
     * @brief 获取召唤同伴目标
     * @return 召唤同伴目标的指针
     */
    [[nodiscard]] entity::ai::goal::SilverfishSummonOthersGoal* getSummonGoal() { return m_summonGoal; }

    /**
     * @brief 通知召唤同伴目标
     * MC 1.16.5: 当受到伤害时调用
     *
     * 这是一个便捷方法，内部调用 getSummonGoal()->notifyHurt()
     */
    void notifySummonCooldown();

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    // 召唤同伴目标（用于受伤时触发）
    entity::ai::goal::SilverfishSummonOthersGoal* m_summonGoal = nullptr;
};

} // namespace mc
