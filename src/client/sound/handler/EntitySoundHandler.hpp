#pragma once

#include "client/sound/handler/IAmbientSoundHandler.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <shared_mutex>

namespace mc::client::sound {

class SoundEngine;

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
    bool isAngry = false;       // 用于蜜蜂
    f32 attackAnimScale = 0.0f; // 用于守卫者
};

/**
 * @brief 实体声音处理器
 *
 * 管理实体特定的可Tick声音（如蜜蜂飞行声音、守卫者攻击声音、鞘翅飞行声音）。
 * 当实体被创建或状态变化时启动相应的声音，实体移除时停止声音。
 *
 * 线程安全：从主线程接收实体状态更新，在音频线程中处理声音。
 *
 * 参考: net.minecraft.client.audio.SoundHandler 中的实体声音管理
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
    void updateEntityState(EntityId entityId, const EntitySoundState& state);

    /**
     * @brief 移除实体状态
     *
     * 当实体被移除时调用。
     *
     * @param entityId 实体ID
     */
    void removeEntityState(EntityId entityId);

    /**
     * @brief 处理实体生成事件
     *
     * @param engine 声音引擎
     * @param entityId 实体ID
     * @param typeId 实体类型ID
     */
    void onEntitySpawn(SoundEngine& engine, EntityId entityId, const String& typeId);

    /**
     * @brief 处理实体移除事件
     *
     * @param entityId 移除的实体ID
     */
    void onEntityRemove(EntityId entityId);

    /**
     * @brief 处理玩家鞘翅飞行状态变化
     *
     * @param engine 声音引擎
     * @param playerId 玩家实体ID
     * @param isFlying 是否正在鞘翅飞行
     */
    void onPlayerElytraFlyingChanged(SoundEngine& engine, EntityId playerId, bool isFlying);

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
    [[nodiscard]] const EntitySoundState* getEntityState(EntityId entityId) const;

    /**
     * @brief 获取实体状态（可变）
     *
     * 用于更新现有状态。
     *
     * @param entityId 实体ID
     * @return 状态指针，如果不存在返回 nullptr
     */
    [[nodiscard]] EntitySoundState* getMutableEntityState(EntityId entityId);

private:
    /**
     * @brief 检查并创建声音
     */
    void checkAndCreateSound(SoundEngine& engine, EntityId entityId, const String& typeId);

    // 实体状态快照（从主线程更新，在音频线程读取）
    // 使用读写锁保护跨线程访问
    mutable std::shared_mutex m_stateMutex;
    std::unordered_map<EntityId, EntitySoundState> m_entityStates;

    // 活动的声音实例（按实体ID索引）- 仅在音频线程访问
    std::unordered_map<EntityId, SoundInstanceId> m_activeSounds;

    // 已生成的实体类型（用于跟踪哪些实体需要声音）- 仅在音频线程访问
    std::unordered_map<EntityId, String> m_entityTypes;
};

} // namespace mc::client::sound
