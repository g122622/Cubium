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
#include <vector>

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
 * 负责管理生物的自然消失机制。
 * 根据 MC 1.16.5 的消失规则：
 * - 距离玩家 > 128 格：立即消失
 * - 距离玩家 > 32 格 且 空闲时间 > 600 tick：1/800 概率消失
 * - 和平难度：怪物立即消失
 *
 * 参考 MC 1.16.5 WorldEntitySpawner.EntityDespawnManager
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
    DespawnManager(DespawnManager&&) = default;
    DespawnManager& operator=(DespawnManager&&) = default;

    /**
     * @brief 每tick调用，检查实体的消失条件
     *
     * 遍历所有生物实体，检查其消失条件。
     * 参考 MC 1.16.5 WorldEntitySpawner#tick
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

    // ========== 常量 ==========

    /// 最小空闲时间（MC 1.16.5: 600 tick = 30秒）
    static constexpr i32 MIN_IDLE_TIME = 600;

    /// 随机消失概率分母（MC 1.16.5: 800）
    static constexpr i32 DESPAWN_CHANCE_DENOMINATOR = 800;

    /// 每tick检查的实体数量限制
    static constexpr i32 MAX_CHECKS_PER_TICK = 50;

private:
    /**
     * @brief 检查单个生物是否应该消失
     *
     * @param mob 生物实体
     * @param world 世界引用
     * @return 是否应该消失
     */
    [[nodiscard]] bool shouldDespawn(MobEntity& mob, ::mc::server::ServerWorld& world) const;

    /**
     * @brief 检查距离玩家的消失条件
     *
     * @param mob 生物实体
     * @param world 世界引用
     * @return 是否应该消失
     */
    [[nodiscard]] bool checkDistanceDespawn(MobEntity& mob, ::mc::server::ServerWorld& world) const;

    bool m_enabled = true;
};

} // namespace world::spawn
} // namespace mc
