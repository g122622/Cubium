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

#include "EntitySoundHandler.hpp"
#include "client/sound/SoundEngine.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "client/sound/instance/MinecartSound.hpp"
#include "client/sound/instance/MovingTickableSound.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>

namespace mc::client::sound {

namespace {
/// 移动声音键的高位标记（用于区分移动声音和其他实体声音）
constexpr u32 MOVING_SOUND_KEY_MASK = 0x40000000;

/// 骑乘声音键的高位标记（用于区分骑乘声音和其他实体声音）
constexpr u32 RIDING_SOUND_KEY_MASK = 0x80000000;
} // namespace

// ============================================================================
// 内部声音类 - 使用状态快照而非实体引用
// ============================================================================

/**
 * @brief 蜜蜂飞行声音基类
 *
 * 支持通过 EntitySoundHandler 查询最新状态。
 * 声音切换流程：当愤怒状态变化时调用 markDone()，EntitySoundHandler::tick()
 * 检测到 isDone() 后根据当前状态重新创建对应类型的声音并立即播放。
 * 这比 MC 原版的 playOnNextTick 方案更优——新声音在同一帧内就播放，无间隙。
 */
class BeeSoundBase : public TickableSound {
public:
    BeeSoundBase(
        const EntitySoundState& state, const ResourceLocation& soundEventId, EntitySoundHandler* handler, bool isAngry)
        : TickableSound(
              soundEventId, SoundCategory::Neutral, state.position, 0.0f, 0.0f, true, AttenuationType::Linear, 16.0f)
        , m_handler(handler)
        , m_entityId(state.entityId)
        , m_isAngry(isAngry)
    {
        setLooping(true);
    }

    void tick() override
    {
        // 从 handler 获取最新状态
        if (m_handler) {
            const EntitySoundState* state = m_handler->getEntityState(m_entityId);
            if (state) {
                updateFromState(*state);
            }
        }
    }

    [[nodiscard]] bool canBeSilent() const override { return true; }

    void updateFromState(const EntitySoundState& state)
    {
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
        f32 horizontalSpeed = std::sqrt(state.velocity.x * state.velocity.x + state.velocity.z * state.velocity.z);

        if (horizontalSpeed >= 0.01f) {
            // 音调范围（幼年蜜蜂更高）
            f32 minPitch = state.isChild ? 1.1f : 0.7f;
            f32 maxPitch = state.isChild ? 1.5f : 1.1f;

            f32 pitch = minPitch +
                (maxPitch - minPitch) * std::clamp(horizontalSpeed, minPitch, maxPitch) / (maxPitch - minPitch);
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
    EntityInstanceId m_entityId;
    bool m_isAngry;
};

/**
 * @brief 蜜蜂飞行声音
 */
class BeeFlightSoundStateful : public BeeSoundBase {
public:
    BeeFlightSoundStateful(const EntitySoundState& state, EntitySoundHandler* handler)
        : BeeSoundBase(state, SoundEvents::ENTITY_BEE_LOOP, handler, false)
    {}

protected:
    void switchSound() override
    {
        // 标记当前声音完成，EntitySoundHandler::tick() 会在同一帧内
        // 检测到 isDone() 并根据当前 isAngry 状态创建 BeeAngrySoundStateful
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
    {}

protected:
    void switchSound() override
    {
        // 标记当前声音完成，EntitySoundHandler::tick() 会在同一帧内
        // 检测到 isDone() 并根据当前 isAngry 状态创建 BeeFlightSoundStateful
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
 *
 * 当守卫者未被移除且有攻击目标时播放。
 * 音量根据攻击动画进度 (attackAnimScale) 变化。
 * 当 targetEntityId 为 0 时停止。
 * 音量 = f * f, 音调 = 0.7 + 0.5 * f
 *
 * 注意：我们在这里自己管理 attackAnimScale，模拟客户端的 clientSideAttackTime
 */
class GuardianSoundStateful : public TickableSound {
public:
    GuardianSoundStateful(const EntitySoundState& state, EntitySoundHandler* handler)
        : TickableSound(SoundEvents::ENTITY_GUARDIAN_ATTACK,
              SoundCategory::Hostile,
              state.position,
              0.0f,
              0.7f,
              true,
              AttenuationType::None,
              16.0f)
        , m_handler(handler)
        , m_entityId(state.entityId)
    {}

    void tick() override
    {
        if (m_handler) {
            const EntitySoundState* state = m_handler->getEntityState(m_entityId);
            if (state) {
                updateFromState(*state);
            }
        }
    }

    [[nodiscard]] bool canBeSilent() const override { return true; }

    void updateFromState(const EntitySoundState& state)
    {
        if (state.isRemoved) {
            markDone();
            return;
        }

        // 当无攻击目标时停止
        if (state.targetEntityId == 0) {
            markDone();
            return;
        }

        setPosition(state.position);

        // 计算攻击动画进度
        // attackAnimScale = clientSideAttackTime / attackDuration (80 ticks)
        // 当目标改变时，onGuardianTargetChanged 会重置 attackAnimScale 为 0
        f32 f = state.attackAnimScale;
        if (f <= 0.0f) {
            // 目标刚切换，重置计数器
            m_clientSideAttackTime = 0;
        }

        // 模拟客户端的 clientSideAttackTime 递增
        ++m_clientSideAttackTime;
        if (m_clientSideAttackTime > ATTACK_DURATION) {
            m_clientSideAttackTime = ATTACK_DURATION;
        }

        f = static_cast<f32>(m_clientSideAttackTime) / static_cast<f32>(ATTACK_DURATION);

        // volume = f * f, pitch = 0.7 + 0.5 * f
        f32 volume = f * f;
        f32 pitch = 0.7f + 0.5f * f;

        setVolume(volume);
        setPitch(pitch);
    }

private:
    EntitySoundHandler* m_handler = nullptr;
    EntityInstanceId m_entityId;
    i32 m_clientSideAttackTime = 0;

    static constexpr i32 ATTACK_DURATION = 80;
};

/**
 * @brief 鞘翅飞行声音（使用状态快照）
 */
class ElytraSoundStateful : public TickableSound {
public:
    ElytraSoundStateful(const EntitySoundState& state, EntitySoundHandler* handler)
        : TickableSound(SoundEvents::ITEM_ELYTRA_FLYING,
              SoundCategory::Players,
              state.position,
              0.1f,
              1.0f,
              true,
              AttenuationType::Linear,
              16.0f)
        , m_handler(handler)
        , m_entityId(state.entityId)
    {}

    void tick() override
    {
        ++m_time;
        if (m_handler) {
            const EntitySoundState* state = m_handler->getEntityState(m_entityId);
            if (state) {
                updateFromState(*state);
            }
        }
    }

    [[nodiscard]] bool canBeSilent() const override { return true; }

    void updateFromState(const EntitySoundState& state)
    {
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
        f32 speedSquared = state.velocity.x * state.velocity.x + state.velocity.y * state.velocity.y +
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
    EntityInstanceId m_entityId;
    i32 m_time = 0;
};

// ============================================================================
// EntitySoundHandler 实现
// ============================================================================

EntitySoundHandler::EntitySoundHandler() = default;

void EntitySoundHandler::updateEntityState(EntityInstanceId entityId, const EntitySoundState& state)
{
    std::unique_lock lock(m_stateMutex);
    m_entityStates[entityId] = state;
    m_entityStates[entityId].entityId = entityId;
}

void EntitySoundHandler::removeEntityState(EntityInstanceId entityId)
{
    std::unique_lock lock(m_stateMutex);
    m_entityStates.erase(entityId);
    m_entityTypes.erase(entityId);
}

void EntitySoundHandler::onEntitySpawn(SoundEngine& engine, EntityInstanceId entityId, const std::string& typeId)
{
    // 记录实体类型
    m_entityTypes[entityId] = typeId;

    // 如果已有状态，立即创建声音
    std::shared_lock lock(m_stateMutex);
    auto stateIt = m_entityStates.find(entityId);
    if (stateIt != m_entityStates.end()) {
        lock.unlock();
        _checkAndCreateSound(engine, entityId, typeId);
    }
}

void EntitySoundHandler::onEntityRemove(EntityInstanceId entityId)
{
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

void EntitySoundHandler::onPlayerElytraFlyingChanged(SoundEngine& engine, EntityInstanceId playerId, bool isFlying)
{
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

void EntitySoundHandler::tick(SoundEngine& engine)
{
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
                _checkAndCreateSound(engine, entityId, typeId);
            }
        }
    }

    // 检查声音切换（蜜蜂愤怒状态变化等）
    // 当 TickableSound 检测到状态变化时调用 markDone()，
    // EntitySoundHandler 在此检测 isDone() 并根据当前状态重新创建声音
    // 由于此代码在 SoundEngine::tick() 的活动声音更新之后执行，
    // 新声音在同一帧内就播放，实现了无缝切换
    for (auto it = m_activeSounds.begin(); it != m_activeSounds.end();) {
        EntityInstanceId entityId = it->first;
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
                const std::string& typeId = typeIt->second;

                // 如果实体类型是蜜蜂，检查是否需要切换
                if (typeId == "minecraft:bee" && !state.isRemoved) {
                    // 蜜蜂声音因愤怒状态切换而完成，根据当前状态重建
                    it = m_activeSounds.erase(it);
                    lock.unlock();
                    _checkAndCreateSound(engine, entityId, typeId);
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

void EntitySoundHandler::stopAll()
{
    m_activeSounds.clear();
    std::unique_lock lock(m_stateMutex);
    m_entityStates.clear();
    m_entityTypes.clear();
}

const EntitySoundState* EntitySoundHandler::getEntityState(EntityInstanceId entityId) const
{
    std::shared_lock lock(m_stateMutex);
    auto it = m_entityStates.find(entityId);
    return it != m_entityStates.end() ? &it->second : nullptr;
}

EntitySoundState* EntitySoundHandler::getMutableEntityState(EntityInstanceId entityId)
{
    std::unique_lock lock(m_stateMutex);
    auto it = m_entityStates.find(entityId);
    return it != m_entityStates.end() ? &it->second : nullptr;
}

void EntitySoundHandler::_checkAndCreateSound(SoundEngine& engine, EntityInstanceId entityId, const std::string& typeId)
{
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
        // 守卫者声音通过 onGuardianAttack 动态创建
        // 这里不创建，因为声音只在攻击时播放
    } else if (typeId.find("minecart") != std::string::npos) {
        // 矿车实体 - 创建矿车行驶声音
        auto sound = std::make_unique<MinecartSoundStateful>(state, this);
        SoundInstanceId soundId = engine.play(std::move(sound));
        if (soundId != 0) {
            m_activeSounds[entityId] = soundId;
        }

        // 检查是否有玩家正在骑乘此矿车
        // 遍历所有实体状态，查找骑乘此矿车的玩家
        for (const auto& [playerId, playerState] : m_entityStates) {
            if (playerState.isRiding && playerState.vehicleId == entityId) {
                // 创建玩家骑乘矿车声音
                auto ridingSound = std::make_unique<RidingMinecartSoundStateful>(playerState, state, this);
                SoundInstanceId ridingSoundId = engine.play(std::move(ridingSound));
                if (ridingSoundId != 0) {
                    // 使用复合键存储骑乘声音
                    m_activeSounds[static_cast<EntityInstanceId>(static_cast<u32>(playerId) | RIDING_SOUND_KEY_MASK)] =
                        ridingSoundId;
                }
            }
        }
    }
}

void EntitySoundHandler::onGuardianAttack(SoundEngine& engine, EntityInstanceId entityId)
{
    // 检查是否已有守卫者声音
    auto soundIt = m_activeSounds.find(entityId);
    if (soundIt != m_activeSounds.end()) {
        // 已有声音，重置攻击动画
        std::shared_lock lock(m_stateMutex);
        auto stateIt = m_entityStates.find(entityId);
        if (stateIt != m_entityStates.end()) {
            // 允许修改，需要升级锁
            lock.unlock();
            std::unique_lock writeLock(m_stateMutex);
            stateIt->second.attackAnimScale = 0.0f;
        }
        return;
    }

    // 获取实体状态
    std::shared_lock lock(m_stateMutex);
    auto stateIt = m_entityStates.find(entityId);
    if (stateIt == m_entityStates.end() || stateIt->second.isRemoved) {
        return;
    }

    // 检查实体类型
    auto typeIt = m_entityTypes.find(entityId);
    if (typeIt == m_entityTypes.end()) {
        return;
    }

    const std::string& typeId = typeIt->second;
    if (typeId != "minecraft:guardian" && typeId != "minecraft:elder_guardian") {
        return;
    }

    // 创建守卫者攻击声音
    EntitySoundState state = stateIt->second;
    lock.unlock();

    auto sound = std::make_unique<GuardianSoundStateful>(state, this);
    SoundInstanceId soundId = engine.play(std::move(sound));
    if (soundId != 0) {
        m_activeSounds[entityId] = soundId;
    }
}

void EntitySoundHandler::onGuardianTargetChanged(EntityInstanceId entityId, EntityInstanceId targetEntityId)
{
    // 如果目标变为 0，停止守卫者声音
    if (targetEntityId == 0) {
        auto soundIt = m_activeSounds.find(entityId);
        if (soundIt != m_activeSounds.end()) {
            // 声音会在 tick 中自动停止（attackAnimScale 会逐渐变为 0）
            // 但我们可以立即移除活动声音记录
            // 注意：不在这里停止声音，让 GuardianSoundStateful::tick() 处理
        }
    }

    // 重置攻击动画
    std::unique_lock lock(m_stateMutex);
    auto stateIt = m_entityStates.find(entityId);
    if (stateIt != m_entityStates.end()) {
        stateIt->second.targetEntityId = targetEntityId;
        stateIt->second.attackAnimScale = 0.0f;
    }
}

void EntitySoundHandler::playMovingSound(SoundEngine& engine,
    const ResourceLocation& soundEventId,
    SoundCategory category,
    EntityInstanceId entityId,
    f32 volume,
    f32 pitch)
{
    // 检查是否已有该实体的移动声音
    // 使用特殊的键来区分移动声音和其他声音
    EntityInstanceId movingSoundKey = static_cast<EntityInstanceId>(static_cast<u32>(entityId) | MOVING_SOUND_KEY_MASK);
    auto soundIt = m_activeSounds.find(movingSoundKey);
    if (soundIt != m_activeSounds.end()) {
        // 已有移动声音，先停止
        engine.stop(soundIt->second);
        m_activeSounds.erase(soundIt);
    }

    // 创建新的移动声音
    auto sound = std::make_unique<MovingTickableSound>(soundEventId, category, this, entityId, volume, pitch);

    SoundInstanceId soundId = engine.play(std::move(sound));
    if (soundId != 0) {
        m_activeSounds[movingSoundKey] = soundId;
    }
}

} // namespace mc::client::sound
