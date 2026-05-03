#include "EntitySoundHandler.hpp"
#include "client/sound/SoundEngine.hpp"
#include "common/sound/SoundEvents.hpp"
#include <spdlog/spdlog.h>

namespace mc::client::sound {

// ============================================================================
// 内部声音类 - 使用状态快照而非实体引用
// ============================================================================

/**
 * @brief 蜜蜂飞行声音（使用状态快照）
 */
class BeeFlightSoundStateful : public TickableSound {
public:
    BeeFlightSoundStateful(const EntitySoundState& state, bool isAngry)
        : TickableSound(
              isAngry ? SoundEvents::ENTITY_BEE_LOOP_AGGRESSIVE : SoundEvents::ENTITY_BEE_LOOP,
              SoundCategory::Neutral,
              state.position,
              0.0f,   // 初始音量
              isAngry ? 0.8f : 0.7f,  // 音调（根据愤怒状态调整）
              true,   // 循环
              AttenuationType::Linear,
              16.0f   // 衰减距离
          )
        , m_isAngry(isAngry)
    {
    }

    void tick() override {
        // 声音在 tick() 时更新位置和音量
        // EntitySoundHandler 会更新状态快照
        // 这里只需要检查是否完成
        // 实际的位置/音量更新由 SoundEngine 根据 ISoundInstance 的属性处理
    }

    [[nodiscard]] bool canBeSilent() const override { return true; }

    void updateState(const EntitySoundState& state) {
        setPosition(state.position);

        // 计算速度
        f32 horizontalSpeed = std::sqrt(state.velocity.x * state.velocity.x + state.velocity.z * state.velocity.z);

        // 音调范围（幼年蜜蜂更高）
        f32 minPitch = state.isChild ? 1.1f : 0.7f;
        f32 maxPitch = state.isChild ? 1.5f : 1.1f;

        if (horizontalSpeed >= 0.01f) {
            f32 pitch = minPitch + (maxPitch - minPitch) * std::clamp(horizontalSpeed, minPitch, maxPitch) / (maxPitch - minPitch);
            setPitch(pitch);

            f32 volume = std::clamp(horizontalSpeed, 0.0f, 0.5f) * 2.4f;
            setVolume(volume);
        } else {
            setPitch(0.0f);
            setVolume(0.0f);
        }

        // 检查是否需要切换声音（愤怒状态变化）
        if (state.isAngry != m_isAngry) {
            markDone();
        }
    }

private:
    bool m_isAngry;
};

/**
 * @brief 守卫者攻击声音（使用状态快照）
 */
class GuardianSoundStateful : public TickableSound {
public:
    GuardianSoundStateful(const EntitySoundState& state)
        : TickableSound(
              SoundEvents::ENTITY_GUARDIAN_ATTACK,
              SoundCategory::Hostile,
              state.position,
              0.0f,   // 初始音量
              0.7f,   // 音调
              true,   // 循环
              AttenuationType::None,  // 无衰减（全局声音）
              16.0f   // 衰减距离
          )
    {
    }

    void tick() override {
        // 位置和音量由 updateState 更新
    }

    [[nodiscard]] bool canBeSilent() const override { return true; }

    void updateState(const EntitySoundState& state) {
        setPosition(state.position);

        // 根据攻击动画计算音量和音调
        // MC 1.16.5: volume = 0.0F + 1.0F * f * f, pitch = 0.7F + 0.5F * f
        f32 f = state.attackAnimScale;
        setVolume(f * f);
        setPitch(0.7f + 0.5f * f);
    }
};

/**
 * @brief 鞘翅飞行声音（使用状态快照）
 */
class ElytraSoundStateful : public TickableSound {
public:
    ElytraSoundStateful(const EntitySoundState& state)
        : TickableSound(
              SoundEvents::ITEM_ELYTRA_FLYING,
              SoundCategory::Players,
              state.position,
              0.1f,   // 初始音量
              1.0f,   // 音调
              true,   // 循环
              AttenuationType::Linear,
              16.0f   // 衰减距离
          )
        , m_time(0)
    {
    }

    void tick() override {
        ++m_time;
    }

    [[nodiscard]] bool canBeSilent() const override { return true; }

    void updateState(const EntitySoundState& state) {
        setPosition(state.position);

        // 前 20 tick 静音
        if (m_time < 20) {
            setVolume(0.0f);
            return;
        }

        // 检查是否仍在飞行
        if (!state.isFallFlying || state.isRemoved) {
            markDone();
            return;
        }

        // 计算速度平方
        f32 speedSquared = state.velocity.x * state.velocity.x + state.velocity.y * state.velocity.y + state.velocity.z * state.velocity.z;

        if (speedSquared >= 1.0e-7f) {
            f32 volume = std::clamp(speedSquared / 4.0f, 0.0f, 1.0f);
            setVolume(volume);

            // 音调根据音量调整
            if (volume > 0.8f) {
                setPitch(1.0f + (volume - 0.8f));
            } else {
                setPitch(1.0f);
            }
        } else {
            setVolume(0.0f);
        }

        // 渐入效果 (20-40 tick)
        if (m_time < 40) {
            f32 fadeIn = static_cast<f32>(m_time - 20) / 20.0f;
            setVolume(getVolume() * fadeIn);
        }
    }

private:
    i32 m_time = 0;
};

// ============================================================================
// EntitySoundHandler 实现
// ============================================================================

EntitySoundHandler::EntitySoundHandler() = default;

void EntitySoundHandler::updateEntityState(EntityId entityId, const EntitySoundState& state) {
    std::unique_lock lock(m_stateMutex);
    m_entityStates[entityId] = state;
}

void EntitySoundHandler::removeEntityState(EntityId entityId) {
    std::unique_lock lock(m_stateMutex);
    m_entityStates.erase(entityId);
    m_entityTypes.erase(entityId);
}

void EntitySoundHandler::onEntitySpawn(SoundEngine& engine, EntityId entityId, const String& typeId) {
    // 记录实体类型
    m_entityTypes[entityId] = typeId;

    // 如果已有状态，立即创建声音
    std::shared_lock lock(m_stateMutex);
    auto stateIt = m_entityStates.find(entityId);
    if (stateIt != m_entityStates.end()) {
        lock.unlock();
        checkAndCreateSound(engine, entityId, typeId);
    }
}

void EntitySoundHandler::onEntityRemove(EntityId entityId) {
    // 标记实体为已移除
    {
        std::unique_lock lock(m_stateMutex);
        auto stateIt = m_entityStates.find(entityId);
        if (stateIt != m_entityStates.end()) {
            stateIt->second.isRemoved = true;
        }
    }

    // 声音会在 tick() 中自动终止
    m_activeSounds.erase(entityId);
}

void EntitySoundHandler::onPlayerElytraFlyingChanged(SoundEngine& engine, EntityId playerId, bool isFlying) {
    std::shared_lock lock(m_stateMutex);
    auto stateIt = m_entityStates.find(playerId);

    if (isFlying) {
        // 开始鞘翅飞行，创建声音
        if (m_activeSounds.find(playerId) == m_activeSounds.end()) {
            EntitySoundState state = stateIt != m_entityStates.end() ? stateIt->second : EntitySoundState{};
            lock.unlock();
            auto sound = std::make_unique<ElytraSoundStateful>(state);
            SoundInstanceId soundId = engine.play(std::move(sound));
            if (soundId != 0) {
                m_activeSounds[playerId] = soundId;
            }
        }
    } else {
        lock.unlock();
        // 停止鞘翅飞行
        auto soundIt = m_activeSounds.find(playerId);
        if (soundIt != m_activeSounds.end()) {
            engine.stop(soundIt->second);
            m_activeSounds.erase(soundIt);
        }
    }
}

void EntitySoundHandler::tick(SoundEngine& engine) {
    // 更新所有活动声音的状态
    // 注意：SoundEngine 会在自己的 tick() 中调用 ISoundInstance::tick()
    // 这里我们只需要处理声音的创建和销毁逻辑

    // 检查是否需要为新生成的实体创建声音
    for (const auto& [entityId, typeId] : m_entityTypes) {
        if (m_activeSounds.find(entityId) == m_activeSounds.end()) {
            std::shared_lock lock(m_stateMutex);
            auto stateIt = m_entityStates.find(entityId);
            if (stateIt != m_entityStates.end() && !stateIt->second.isRemoved) {
                lock.unlock();
                checkAndCreateSound(engine, entityId, typeId);
            }
        }
    }
}

void EntitySoundHandler::stopAll() {
    m_activeSounds.clear();
    std::unique_lock lock(m_stateMutex);
    m_entityStates.clear();
    m_entityTypes.clear();
}

const EntitySoundState* EntitySoundHandler::getEntityState(EntityId entityId) const {
    std::shared_lock lock(m_stateMutex);
    auto it = m_entityStates.find(entityId);
    return it != m_entityStates.end() ? &it->second : nullptr;
}

EntitySoundState* EntitySoundHandler::getMutableEntityState(EntityId entityId) {
    std::unique_lock lock(m_stateMutex);
    auto it = m_entityStates.find(entityId);
    return it != m_entityStates.end() ? &it->second : nullptr;
}

void EntitySoundHandler::checkAndCreateSound(SoundEngine& engine, EntityId entityId, const String& typeId) {
    std::shared_lock lock(m_stateMutex);
    auto stateIt = m_entityStates.find(entityId);
    if (stateIt == m_entityStates.end() || stateIt->second.isRemoved) {
        return;
    }

    const EntitySoundState& state = stateIt->second;

    if (typeId == "minecraft:bee") {
        auto sound = std::make_unique<BeeFlightSoundStateful>(state, state.isAngry);
        engine.play(std::move(sound));
    } else if (typeId == "minecraft:guardian" || typeId == "minecraft:elder_guardian") {
        // TODO: 需要攻击目标状态
        // auto sound = std::make_unique<GuardianSoundStateful>(state);
        // engine.play(std::move(sound));
    }
}

} // namespace mc::client::sound
