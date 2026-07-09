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

#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {

// 前向声明
class Entity;
class MobEntity;

namespace server {
class ServerWorld;
}

namespace world::spawn {

/**
 * @brief 生物消失管理器
 *
 * Mob.checkDespawn 的消失决策：
 * - 和平难度下不允许的生物立即消失
 * - 非持久化生物距玩家 > despawnDistance 立即消失
 * - 非持久化生物距玩家 > noDespawnDistance(32) 且 noActionTime>600 时，1/800 概率消失
 * - 距玩家 < 32 重置 noActionTime
 * - 持久化生物重置 noActionTime，永不消失
 * - 无玩家时保留（getNearestPlayer 返回 null 不做任何事）
 */
class DespawnManager {
public:
    /**
     * @brief 构造函数
     */
    DespawnManager() = default;

    /**
     * @brief 析构函数
     */
    ~DespawnManager() = default;

    // 禁止拷贝
    DespawnManager(const DespawnManager&) = delete;
    DespawnManager& operator=(const DespawnManager&) = delete;

    // 允许移动
    DespawnManager(DespawnManager&&) noexcept = default;
    DespawnManager& operator=(DespawnManager&&) noexcept = default;

    /**
     * @brief 每tick调用，检查实体的消失条件
     *
     * 遍历所有生物实体，检查其消失条件。每实体每 tick 都检查，无每 tick 上限。
     *
     * @param world 世界引用
     */
    void tick(::mc::server::ServerWorld& world);

    /**
     * @brief 设置是否启用消失检查
     * @param enabled 是否启用
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief 获取是否启用消失检查
     */
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    /**
     * @brief 纯函数消失决策（Mob.checkDespawn）
     *
     * 不依赖 ServerWorld，便于覆盖边界。无玩家时用 kNoPlayer（负值哨兵）表示，
     * 此时 getNearestPlayer 返回 null 的语义——保留实体。
     *
     * @param mob 待检查生物
     * @param closestPlayerDistSq 最近玩家距离平方；无玩家时传 kNoPlayer（负值）
     * @param difficulty 当前难度
     * @param currentTick 当前游戏刻
     * @param random 随机数源
     * @return 是否应消失
     */
    [[nodiscard]] static bool shouldDespawn(
        MobEntity& mob, f64 closestPlayerDistSq, Difficulty difficulty, u64 currentTick, math::Random& random);

    /**
     * @brief 纯函数消失决策（内部自建确定性随机源）
     */
    [[nodiscard]] static bool shouldDespawn(
        MobEntity& mob, f64 closestPlayerDistSq, Difficulty difficulty, u64 currentTick);

    // ========== 常量 ==========

    /// 无玩家哨兵：closestPlayerDistSq 传此值表示无玩家（保留实体）
    static constexpr f64 kNoPlayer = -1.0;

    /// 最小空闲时间（600 tick = 30秒）
    static constexpr i32 MIN_IDLE_TIME = 600;

    /// 随机消失概率分母（800）
    static constexpr i32 DESPAWN_CHANCE_DENOMINATOR = 800;

private:
    bool m_enabled = true;
};

} // namespace world::spawn
} // namespace mc
