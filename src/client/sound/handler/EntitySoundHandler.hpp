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

#include "client/sound/handler/IAmbientSoundHandler.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>

namespace mc::client::sound {

class SoundEngine;
class EntitySoundHandler;

/**
 * @brief 实体状态快照（用于音频线程）
 *
 * 存储实体声音所需的状态，避免跨线程访问 ClientEntity。
 */
struct EntitySoundState {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    bool isRemoved = false;
    bool isChild = false;
    bool isFallFlying = false;
    bool isAngry = false;         // 用于蜜蜂
    f32 attackAnimScale = 0.0f;   // 用于守卫者攻击动画进度 (0.0-1.0)
    EntityInstanceId entityId{0}; // 实体ID，用于查找状态

    // 守卫者攻击目标状态
    EntityInstanceId targetEntityId{0}; // 守卫者的攻击目标ID（0表示无目标）

    // 矿车相关状态
    EntityInstanceId vehicleId{0}; // 玩家正在骑乘的载具ID（用于矿车声音）
    bool isRiding = false;         // 是否正在骑乘
};

/**
 * @brief 实体声音处理器
 *
 * 管理实体特定的可Tick声音（如蜜蜂飞行声音、守卫者攻击声音、鞘翅飞行声音）。
 * 当实体被创建或状态变化时启动相应的声音，实体移除时停止声音。
 *
 * 线程安全：从主线程接收实体状态更新，在音频线程中处理声音。
 */
class EntitySoundHandler : public IAmbientSoundHandler {
public:
    EntitySoundHandler();
    ~EntitySoundHandler() override = default;

    // 禁止拷贝
    EntitySoundHandler(const EntitySoundHandler&) = delete;
    EntitySoundHandler& operator=(const EntitySoundHandler&) = delete;

    // ========================================================================
    // 主线程调用 - 状态更新
    // ========================================================================

    /**
     * @brief 更新实体状态
     *
     * 从主线程调用，更新实体的状态快照。
     *
     * @param entityId 实体ID
     * @param state 实体状态快照
     */
    void updateEntityState(EntityInstanceId entityId, const EntitySoundState& state);

    /**
     * @brief 移除实体状态
     *
     * 当实体被移除时调用。
     *
     * @param entityId 实体ID
     */
    void removeEntityState(EntityInstanceId entityId);

    /**
     * @brief 处理实体生成事件
     *
     * @param engine 声音引擎
     * @param entityId 实体ID
     * @param typeId 实体类型ID
     */
    void onEntitySpawn(SoundEngine& engine, EntityInstanceId entityId, const std::string& typeId);

    /**
     * @brief 处理实体移除事件
     *
     * @param entityId 移除的实体ID
     */
    void onEntityRemove(EntityInstanceId entityId);

    /**
     * @brief 处理玩家鞘翅飞行状态变化
     *
     * @param engine 声音引擎
     * @param playerId 玩家实体ID
     * @param isFlying 是否正在鞘翅飞行
     */
    void onPlayerElytraFlyingChanged(SoundEngine& engine, EntityInstanceId playerId, bool isFlying);

    /**
     * @brief 处理守卫者攻击事件
     *
     * 当收到实体状态 21 时调用，创建 GuardianSound。
     *
     * @param engine 声音引擎
     * @param entityId 守卫者实体ID
     */
    void onGuardianAttack(SoundEngine& engine, EntityInstanceId entityId);

    /**
     * @brief 处理守卫者攻击目标变化
     *
     * 当 TARGET_ENTITY 元数据变化时调用。
     * 目标为 0 时停止攻击声音。
     *
     * @param entityId 守卫者实体ID
     * @param targetEntityId 攻击目标实体ID（0表示无目标）
     */
    void onGuardianTargetChanged(EntityInstanceId entityId, EntityInstanceId targetEntityId);

    /**
     * @brief 播放移动声音
     *
     * 创建跟随实体位置移动的声音。当实体被移除时自动停止。
     *
     * @param engine 声音引擎
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param entityId 实体ID
     * @param volume 音量
     * @param pitch 音调
     */
    void playMovingSound(SoundEngine& engine,
        const ResourceLocation& soundEventId,
        SoundCategory category,
        EntityInstanceId entityId,
        f32 volume,
        f32 pitch);

    // ========================================================================
    // 音频线程调用 - tick 更新
    // ========================================================================

    /**
     * @brief 每帧更新
     *
     * 在音频线程中调用，更新所有活动的实体声音。
     *
     * @param engine 声音引擎
     */
    void tick(SoundEngine& engine) override;

    /**
     * @brief 停止所有实体声音
     */
    void stopAll();

    /**
     * @brief 获取实体状态（只读）
     *
     * @param entityId 实体ID
     * @return 状态指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const EntitySoundState* getEntityState(EntityInstanceId entityId) const;

    /**
     * @brief 获取实体状态（可变）
     *
     * 用于更新现有状态。
     *
     * @param entityId 实体ID
     * @return 状态指针，如果不存在返回 nullptr
     */
    [[nodiscard]] EntitySoundState* getMutableEntityState(EntityInstanceId entityId);

private:
    /**
     * @brief 检查并创建声音
     */
    void _checkAndCreateSound(SoundEngine& engine, EntityInstanceId entityId, const std::string& typeId);

    // 实体状态快照（从主线程更新，在音频线程读取）
    // 使用读写锁保护跨线程访问
    mutable std::shared_mutex m_stateMutex;
    std::unordered_map<EntityInstanceId, EntitySoundState> m_entityStates;

    // 活动的声音实例（按实体ID索引）- 仅在音频线程访问
    std::unordered_map<EntityInstanceId, SoundInstanceId> m_activeSounds;

    // 已生成的实体类型（用于跟踪哪些实体需要声音）- 仅在音频线程访问
    std::unordered_map<EntityInstanceId, std::string> m_entityTypes;
};

} // namespace mc::client::sound
