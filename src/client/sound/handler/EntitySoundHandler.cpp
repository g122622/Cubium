#include "EntitySoundHandler.hpp"
#include "client/sound/SoundEngine.hpp"
#include "client/sound/SoundPool.hpp"
#include "common/sound/SoundEvents.hpp"
#include <spdlog/spdlog.h>

namespace mc::client::sound {

// ============================================================================
// 内部声音类 - 使用状态快照而非实体引用
// ============================================================================

/**
 * @brief 蜜蜂飞行声音基类
 *
 * 支持通过 EntitySoundHandler 查询最新状态
 */
class BeeSoundBase : public TickableSound {
public:
    BeeSoundBase(const EntitySoundState& state, const ResourceLocation& soundEventId,
                 EntitySoundHandler* handler, bool isAngry)
        : TickableSound(soundEventId, SoundCategory::Neutral, state.position, 0.0f, 0.0f,
                        true, AttenuationType::Linear, 16.0f)
        , m_handler(handler)
        , m_entityId(state.entityId)
        , m_isAngry(isAngry)
    {
        setLooping(true);
    }

    void tick() override {
        // 从 handler 获取最新状态
        if (m_handler) {
            const EntitySoundState* state = m_handler->getEntityState(m_entityId);
            if (state) {
                updateFromState(*state);
            }
        }
    }

    [[nodiscard]] bool canBeSilent() const override { return true; }

    void updateFromState(const EntitySoundState& state) {
        // 检查是否需要切换声音（愤怒状态变化）
        if (state.isAngry != m_isAngry) {
            switchSound();
            return;
        }

        // 检查实体是否已移除
        if (state.isRemoved) {
            markDone();
            return;
        }

        // 更新位置
        setPosition(state.position);

        // 根据水平速度计算音量和音调
        f32 horizontalSpeed = std::sqrt(state.velocity.x * state.velocity.x +
                                        state.velocity.z * state.velocity.z);

        if (horizontalSpeed >= 0.01f) {
            // 音调范围（幼年蜜蜂更高）
            f32 minPitch = state.isChild ? 1.1f : 0.7f;
            f32 maxPitch = state.isChild ? 1.5f : 1.1f;

            f32 pitch = minPitch + (maxPitch - minPitch) *
                         std::clamp(horizontalSpeed, minPitch, maxPitch) / (maxPitch - minPitch);
            setPitch(pitch);

            f32 volume = std::clamp(horizontalSpeed, 0.0f, 0.5f) * 2.4f;
            setVolume(volume);
        } else {
            setPitch(0.0f);
            setVolume(0.0f);
        }
    }

protected:
    /**
     * @brief 切换到另一个声音（飞行<->愤怒）
     */
    virtual void switchSound() = 0;

    EntitySoundHandler* m_handler = nullptr;
    EntityId m_entityId;
    bool m_isAngry;
};

/**
 * @brief 蜜蜂飞行声音
 */
class BeeFlightSoundStateful : public BeeSoundBase {
public:
    BeeFlightSoundStateful(const EntitySoundState& state, EntitySoundHandler* handler)
        : BeeSoundBase(state, SoundEvents::ENTITY_BEE_LOOP, handler, false)
    {
    }

protected:
    void switchSound() override {
        // 切换到愤怒声音
        // 注意：这里不能直接创建新声音，需要通过 SoundEngine::playOnNextTick
        // 但由于 TickableSound 没有访问 SoundEngine 的权限，
        // 我们标记为完成，EntitySoundHandler 会在下一 tick 检测并创建新声音
        markDone();
        m_needsSwitch = true;
    }

public:
    [[nodiscard]] bool needsSwitchToAngry() const { return m_needsSwitch; }

private:
    bool m_needsSwitch = false;
};

/**
 * @brief 蜜蜂愤怒声音
 */
class BeeAngrySoundStateful : public BeeSoundBase {
public:
    BeeAngrySoundStateful(const EntitySoundState& state, EntitySoundHandler* handler)
        : BeeSoundBase(state, SoundEvents::ENTITY_BEE_LOOP_AGGRESSIVE, handler, true)
    {
    }

protected:
    void switchSound() override {
        // 切换回飞行声音
        markDone();
        m_needsSwitch = true;
    }

public:
    [[nodiscard]] bool needsSwitchToFlight() const { return m_needsSwitch; }

private:
    bool m_needsSwitch = false;
};

/**
 * @brief 守卫者攻击声音（使用状态快照）
 */
class GuardianSoundStateful : public TickableSound {
public:
    GuardianSoundStateful(const EntitySoundState& state, EntitySoundHandler* handler)
        : TickableSound(SoundEvents::ENTITY_GUARDIAN_ATTACK, SoundCategory::Hostile,
                        state.position, 0.0f, 0.7f, true, AttenuationType::None, 16.0f)
        , m_handler(handler)
        , m_entityId(state.entityId)
    {
    }

    void tick() override {
        if (m_handler) {
            const EntitySoundState* state = m_handler->getEntityState(m_entityId);
            if (state) {
                updateFromState(*state);
            }
        }
    }

    [[nodiscard]] bool canBeSilent() const override { return true; }

    void updateFromState(const EntitySoundState& state) {
        if (state.isRemoved) {
            markDone();
            return;
        }

        setPosition(state.position);

        // 根据攻击动画计算音量和音调
        // MC 1.16.5: volume = 0.0F + 1.0F * f * f, pitch = 0.7F + 0.5F * f
        f32 f = state.attackAnimScale;
        setVolume(f * f);
        setPitch(0.7f + 0.5f * f);
    }

private:
    EntitySoundHandler* m_handler = nullptr;
    EntityId m_entityId;
};

/**
 * @brief 鞘翅飞行声音（使用状态快照）
 */
class ElytraSoundStateful : public TickableSound {
public:
    ElytraSoundStateful(const EntitySoundState& state, EntitySoundHandler* handler)
        : TickableSound(SoundEvents::ITEM_ELYTRA_FLYING, SoundCategory::Players,
                        state.position, 0.1f, 1.0f, true, AttenuationType::Linear, 16.0f)
        , m_handler(handler)
        , m_entityId(state.entityId)
    {
    }

    void tick() override {
        ++m_time;
        if (m_handler) {
            const EntitySoundState* state = m_handler->getEntityState(m_entityId);
            if (state) {
                updateFromState(*state);
            }
        }
    }

    [[nodiscard]] bool canBeSilent() const override { return true; }

    void updateFromState(const EntitySoundState& state) {
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
        f32 speedSquared = state.velocity.x * state.velocity.x +
                          state.velocity.y * state.velocity.y +
                          state.velocity.z * state.velocity.z;

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
    EntitySoundHandler* m_handler = nullptr;
    EntityId m_entityId;
    i32 m_time = 0;
};

// ============================================================================
// EntitySoundHandler 实现
// ============================================================================

EntitySoundHandler::EntitySoundHandler() = default;

void EntitySoundHandler::updateEntityState(EntityId entityId, const EntitySoundState& state) {
    std::unique_lock lock(m_stateMutex);
    m_entityStates[entityId] = state;
    m_entityStates[entityId].entityId = entityId;
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
            state.entityId = playerId;
            auto sound = std::make_unique<ElytraSoundStateful>(state, this);
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
            m_activeSounds.erase(playerId);
        }
    }
}

void EntitySoundHandler::tick(SoundEngine& engine) {
    // 更新所有活动声音的状态
    // 注意：SoundEngine 会在自己的 tick() 中调用 ISoundInstance::tick()
    // 这里我们检查声音切换和状态更新

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

    // 检查声音切换（蜜蜂愤怒状态变化）
    // 由于 TickableSound::tick() 已经在 SoundEngine::tick() 中被调用，
    // 我们需要检查是否有声音因为状态切换而完成
    for (auto it = m_activeSounds.begin(); it != m_activeSounds.end(); ) {
        EntityId entityId = it->first;
        SoundInstanceId soundId = it->second;

        // 获取声音实例检查是否完成
        ISoundInstance* sound = engine.getSoundInstance(soundId);
        if (!sound || sound->isDone()) {
            // 检查是否需要切换声音（蜜蜂愤怒状态变化）
            std::shared_lock lock(m_stateMutex);
            auto stateIt = m_entityStates.find(entityId);
            auto typeIt = m_entityTypes.find(entityId);

            if (stateIt != m_entityStates.end() && typeIt != m_entityTypes.end()) {
                const EntitySoundState& state = stateIt->second;
                const String& typeId = typeIt->second;

                // 如果实体类型是蜜蜂，检查是否需要切换
                if (typeId == "minecraft:bee" && !state.isRemoved) {
                    // 声音完成，可能是因为状态切换
                    // 重新创建声音（根据当前愤怒状态）
                    it = m_activeSounds.erase(it);
                    lock.unlock();
                    checkAndCreateSound(engine, entityId, typeId);
                    continue;
                }
            }
            // 移除完成的声音
            it = m_activeSounds.erase(it);
        } else {
            ++it;
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
        // 根据愤怒状态选择声音
        if (state.isAngry) {
            auto sound = std::make_unique<BeeAngrySoundStateful>(state, this);
            SoundInstanceId soundId = engine.play(std::move(sound));
            if (soundId != 0) {
                m_activeSounds[entityId] = soundId;
            }
        } else {
            auto sound = std::make_unique<BeeFlightSoundStateful>(state, this);
            SoundInstanceId soundId = engine.play(std::move(sound));
            if (soundId != 0) {
                m_activeSounds[entityId] = soundId;
            }
        }
    } else if (typeId == "minecraft:guardian" || typeId == "minecraft:elder_guardian") {
        // TODO: 需要攻击目标状态
        // auto sound = std::make_unique<GuardianSoundStateful>(state, this);
        // engine.play(std::move(sound));
    }
}

} // namespace mc::client::sound
