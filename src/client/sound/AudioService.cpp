#include "client/sound/AudioService.hpp"

#include "client/settings/ClientSettings.hpp"
#include "client/sound/SoundEngine.hpp"
#include "client/sound/SoundHandler.hpp"
#include "client/sound/MusicPlayer.hpp"
#include "client/sound/handler/BiomeAmbientHandler.hpp"
#include "client/sound/handler/UnderwaterAmbientHandler.hpp"
#include "client/sound/handler/BubbleColumnAmbientHandler.hpp"
#include "client/sound/handler/EntitySoundHandler.hpp"

#include "common/resource/ResourcePackList.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/perfetto/PerfettoManager.hpp"

#include <spdlog/spdlog.h>

#include <chrono>
#include <utility>

namespace mc::client::sound {

namespace {
constexpr std::chrono::milliseconds AUDIO_TICK_INTERVAL{50};
using AudioClock = std::chrono::steady_clock;
}

AudioService::AudioService(ResourcePackList& resourcePacks, ClientSettings& settings)
    : m_resourcePacks(resourcePacks)
    , m_settings(settings)
{
}

AudioService::~AudioService()
{
    shutdown();
}

Result<void> AudioService::initialize()
{
    MC_TRACE_EVENT("client.initialization", "AudioService::initialize");

    if (m_running.load()) {
        return Error(ErrorCode::AlreadyExists, "Audio service already initialized");
    }

    m_stopRequested.store(false);
    m_initComplete = false;
    m_initResult = Result<void>::ok();

    m_workerThread = std::thread(&AudioService::runWorker, this);

    std::unique_lock lock(m_initMutex);
    m_initConditionVariable.wait(lock, [this]() { return m_initComplete; });

    if (m_initResult.failed()) {
        shutdown();
    }

    return m_initResult;
}

void AudioService::shutdown()
{
    MC_TRACE_EVENT("client.sound", "AudioService::shutdown");

    if (!m_running.load() && !m_workerThread.joinable()) {
        return;
    }

    m_stopRequested.store(true);
    m_conditionVariable.notify_all();

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    m_loaded.store(false);
    m_entitySoundHandler = nullptr;
    m_running.store(false);
    m_biomeAmbientHandler = nullptr;
    m_underwaterAmbientHandler = nullptr;
}

void AudioService::play(std::unique_ptr<ISoundInstance> sound)
{
    MC_TRACE_EVENT("client.sound", "AudioService::play");

    if (!sound || !m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::Play;
    command.sound = std::move(sound);
    enqueue(std::move(command));
}

void AudioService::playDelayed(std::unique_ptr<ISoundInstance> sound, u32 delayTicks)
{
    MC_TRACE_EVENT("client.sound", "AudioService::playDelayed");

    if (!sound || !m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::PlayDelayed;
    command.sound = std::move(sound);
    command.delayTicks = delayTicks;
    enqueue(std::move(command));
}

void AudioService::stop(SoundInstanceId id)
{
    MC_TRACE_EVENT("client.sound", "AudioService::stop");

    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::StopId;
    command.soundId = id;
    enqueue(std::move(command));
}

void AudioService::stop(const ResourceLocation& soundEventId)
{
    MC_TRACE_EVENT("client.sound", "AudioService::stop", "soundEventId", soundEventId.toString());

    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::StopEvent;
    command.soundEventId = soundEventId;
    enqueue(std::move(command));
}

void AudioService::stop(SoundCategory category)
{
    MC_TRACE_EVENT("client.sound", "AudioService::stop", "category", static_cast<u8>(category));

    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::StopCategory;
    command.category = category;
    enqueue(std::move(command));
}

void AudioService::stopAll()
{
    MC_TRACE_EVENT("client.sound", "AudioService::stopAll");

    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::StopAll;
    enqueue(std::move(command));
}

void AudioService::pause()
{
    setPaused(true);
}

void AudioService::resume()
{
    setPaused(false);
}

void AudioService::setPaused(bool paused)
{
    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::SetPaused;
    command.paused = paused;
    enqueue(std::move(command));
}

void AudioService::updateListener(const glm::vec3& position,
                                  const glm::vec3& forward,
                                  const glm::vec3& up)
{
    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::UpdateListener;
    command.position = position;
    command.forward = forward;
    command.up = up;
    enqueue(std::move(command));
}

void AudioService::setListenerVelocity(const glm::vec3& velocity)
{
    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::SetListenerVelocity;
    command.velocity = velocity;
    enqueue(std::move(command));
}

void AudioService::setVolume(SoundCategory category, f32 volume)
{
    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::SetVolume;
    command.category = category;
    command.volume = volume;
    enqueue(std::move(command));
}

void AudioService::reloadSoundDefinitions()
{
    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::ReloadSounds;
    enqueue(std::move(command));
}

void AudioService::setBiomeId(u32 biomeId)
{
    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::SetBiomeId;
    command.biomeId = biomeId;
    enqueue(std::move(command));
}

void AudioService::setUnderwater(bool underwater)
{
    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::SetUnderwater;
    command.underwater = underwater;
    enqueue(std::move(command));
}

void AudioService::setBubbleColumnState(bool inBubbleColumn, bool isDrag)
{
    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::SetBubbleColumnState;
    command.bubbleColumn.inBubbleColumn = inBubbleColumn;
    command.bubbleColumn.isDrag = isDrag;
    enqueue(std::move(command));
}

void AudioService::updateMusicState(i32 dimension, bool inCreative, bool inBossFight)
{
    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::UpdateMusicState;
    command.dimension = dimension;
    command.inCreative = inCreative;
    command.inBossFight = inBossFight;
    enqueue(std::move(command));
}

void AudioService::setAmbientLightLevel(u8 skyLight, u8 blockLight)
{
    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::SetAmbientLightLevel;
    command.skyLight = skyLight;
    command.blockLight = blockLight;
    enqueue(std::move(command));
}

void AudioService::setAmbientPlayerPosition(f64 x, f64 y, f64 z)
{
    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::SetAmbientPlayerPosition;
    command.playerX = x;
    command.playerY = y;
    command.playerZ = z;
    enqueue(std::move(command));
}

void AudioService::setInMenu(bool inMenu)
{
    if (!m_loaded.load()) {
        return;
    }

    Command command;
    command.type = CommandType::SetInMenu;
    command.inMenu = inMenu;
    enqueue(std::move(command));
}

void AudioService::onEntitySpawn(u32 entityId, const String& typeId, f32 x, f32 y, f32 z)
{
    if (!m_loaded.load() || !m_entitySoundHandler) {
        return;
    }

    // 更新实体状态快照
    EntitySoundState state;
    state.position = glm::vec3(x, y, z);
    m_entitySoundHandler->updateEntityState(static_cast<EntityId>(entityId), state);

    // 发送生成事件
    Command command;
    command.type = CommandType::EntitySpawn;
    command.entityId = entityId;
    command.entityTypeId = typeId;
    enqueue(std::move(command));
}

void AudioService::onEntityRemove(u32 entityId)
{
    if (!m_loaded.load() || !m_entitySoundHandler) {
        return;
    }

    // 发送移除事件
    Command command;
    command.type = CommandType::EntityRemove;
    command.entityId = entityId;
    enqueue(std::move(command));

    // 清理状态快照
    m_entitySoundHandler->removeEntityState(static_cast<EntityId>(entityId));
}

void AudioService::onPlayerElytraFlyingChanged(u32 entityId, bool isFlying)
{
    if (!m_loaded.load() || !m_entitySoundHandler) {
        return;
    }

    // 更新状态快照
    if (auto* state = m_entitySoundHandler->getMutableEntityState(static_cast<EntityId>(entityId))) {
        state->isFallFlying = isFlying;
    }

    // 发送飞行状态变化事件
    Command command;
    command.type = CommandType::ElytraFlyingChanged;
    command.entityId = entityId;
    command.isFlying = isFlying;
    enqueue(std::move(command));
}

void AudioService::onEntityAngerStateChanged(u32 entityId, bool isAngry)
{
    if (!m_loaded.load() || !m_entitySoundHandler) {
        return;
    }

    // 更新状态快照中的愤怒状态
    if (auto* state = m_entitySoundHandler->getMutableEntityState(static_cast<EntityId>(entityId))) {
        state->isAngry = isAngry;
    }
}

void AudioService::updateEntityPosition(u32 entityId, f32 x, f32 y, f32 z, f32 vx, f32 vy, f32 vz)
{
    if (!m_loaded.load() || !m_entitySoundHandler) {
        return;
    }

    // 更新状态快照中的位置和速度
    if (auto* state = m_entitySoundHandler->getMutableEntityState(static_cast<EntityId>(entityId))) {
        state->position = glm::vec3(x, y, z);
        state->velocity = glm::vec3(vx, vy, vz);
    }
}

void AudioService::onGuardianAttack(u32 entityId)
{
    if (!m_loaded.load() || !m_entitySoundHandler) {
        return;
    }

    // 发送守卫者攻击事件到音频线程
    Command command;
    command.type = CommandType::GuardianAttack;
    command.entityId = entityId;
    enqueue(std::move(command));
}

void AudioService::updateGuardianTarget(u32 entityId, u32 targetEntityId)
{
    if (!m_loaded.load() || !m_entitySoundHandler) {
        return;
    }

    // 更新状态快照中的目标
    if (auto* state = m_entitySoundHandler->getMutableEntityState(static_cast<EntityId>(entityId))) {
        state->targetEntityId = static_cast<EntityId>(targetEntityId);
    }

    // 发送目标更新到音频线程
    Command command;
    command.type = CommandType::GuardianTargetUpdate;
    command.entityId = entityId;
    command.targetEntityId = targetEntityId;
    enqueue(std::move(command));
}

void AudioService::enqueue(Command command)
{
    {
        std::lock_guard lock(m_mutex);
        m_commands.push_back(std::move(command));
    }

    m_conditionVariable.notify_one();
}

void AudioService::runWorker()
{
    const std::string threadName = "AudioEngineWorker";
    mc::perfetto::PerfettoManager::instance().setThreadName(threadName);

    MC_TRACE_EVENT("client.initialization", "AudioService::runWorker");

    try {
        m_soundHandler = std::make_unique<SoundHandler>(m_resourcePacks);
        m_soundEngine = std::make_unique<SoundEngine>(*m_soundHandler, m_settings);

        auto initResult = m_soundEngine->initialize();
        if (initResult.failed()) {
            m_initResult = initResult;
            {
                std::lock_guard lock(m_initMutex);
                m_initComplete = true;
            }
            m_initConditionVariable.notify_all();
            m_soundEngine.reset();
            m_soundHandler.reset();
            return;
        }

        auto biomeAmbientHandler = std::make_unique<BiomeAmbientHandler>();
        m_biomeAmbientHandler = biomeAmbientHandler.get();
        m_soundEngine->addAmbientHandler(std::move(biomeAmbientHandler));

        auto underwaterAmbientHandler = std::make_unique<UnderwaterAmbientHandler>();
        m_underwaterAmbientHandler = underwaterAmbientHandler.get();
        m_soundEngine->addAmbientHandler(std::move(underwaterAmbientHandler));

        auto bubbleColumnAmbientHandler = std::make_unique<BubbleColumnAmbientHandler>();
        m_bubbleColumnAmbientHandler = bubbleColumnAmbientHandler.get();
        m_soundEngine->addAmbientHandler(std::move(bubbleColumnAmbientHandler));

        // 创建实体声音处理器
        auto entitySoundHandler = std::make_unique<EntitySoundHandler>();
        m_entitySoundHandler = entitySoundHandler.get();
        m_soundEngine->addAmbientHandler(std::move(entitySoundHandler));

        // 创建音乐播放器
        m_musicPlayer = std::make_unique<MusicPlayer>(*m_soundEngine);

        m_loaded.store(true);
        m_running.store(true);
        m_initResult = Result<void>::ok();
        {
            std::lock_guard lock(m_initMutex);
            m_initComplete = true;
        }
        m_initConditionVariable.notify_all();

        // 固定间隔 tick：用于环境音、音乐与延迟播放等逻辑推进。
        // 命令会唤醒线程立即处理，但不会因为命令洪泛而导致每条命令都触发一次 tick。
        auto nextTickTime = AudioClock::now() + AUDIO_TICK_INTERVAL;

        while (!m_stopRequested.load()) {
            std::deque<Command> localCommands;
            {
                std::unique_lock lock(m_mutex);
                if (m_commands.empty() && !m_stopRequested.load()) {
                    m_conditionVariable.wait_until(lock, nextTickTime, [this]() {
                        return m_stopRequested.load() || !m_commands.empty();
                    });
                }

                localCommands.swap(m_commands);
            }

            for (auto& command : localCommands) {
                processCommand(command);
            }

            if (m_stopRequested.load()) {
                break;
            }

            const auto now = AudioClock::now();
            if (now >= nextTickTime) {
                if (m_soundEngine) {
                    m_soundEngine->tick(m_paused.load());
                }

                // 更新音乐播放器
                // 注意：inWater 状态通过 setUnderwater 传递
                // dimension/inCreative/inBossFight/inMenu 通过相应命令更新
                if (m_musicPlayer) {
                    m_musicPlayer->tick(m_paused.load(), m_savedInMenu.load(), m_savedDimension, m_savedUnderwater,
                                        m_savedCreative, m_savedBossFight);
                }

                nextTickTime = now + AUDIO_TICK_INTERVAL;
            }
        }

        if (m_soundEngine) {
            m_soundEngine->shutdown();
        }

        m_soundEngine.reset();
        m_soundHandler.reset();
        m_musicPlayer.reset();
        m_biomeAmbientHandler = nullptr;
        m_underwaterAmbientHandler = nullptr;
        m_loaded.store(false);
        m_running.store(false);
    } catch (const std::exception& e) {
        spdlog::error("[AudioService] Worker thread failed: {}", e.what());
        m_initResult = Error(ErrorCode::OperationFailed, e.what(), "AudioService::runWorker");
        {
            std::lock_guard lock(m_initMutex);
            m_initComplete = true;
        }
        m_initConditionVariable.notify_all();
        m_loaded.store(false);
        m_running.store(false);
    }
}

void AudioService::processCommand(Command& command)
{
    if (!m_soundEngine) {
        return;
    }

    switch (command.type) {
        case CommandType::Play:
            m_soundEngine->play(std::move(command.sound));
            break;

        case CommandType::PlayDelayed:
            m_soundEngine->playDelayed(std::move(command.sound), command.delayTicks);
            break;

        case CommandType::StopId:
            m_soundEngine->stop(command.soundId);
            break;

        case CommandType::StopEvent:
            m_soundEngine->stop(command.soundEventId);
            break;

        case CommandType::StopCategory:
            m_soundEngine->stop(command.category);
            break;

        case CommandType::StopAll:
            m_soundEngine->stopAll();
            break;

        case CommandType::SetPaused:
            m_paused.store(command.paused);
            break;

        case CommandType::UpdateListener:
            m_soundEngine->updateListener(command.position, command.forward, command.up);
            break;

        case CommandType::SetListenerVelocity:
            m_soundEngine->setListenerVelocity(command.velocity);
            break;

        case CommandType::SetVolume:
            m_soundEngine->setVolume(command.category, command.volume);
            break;

        case CommandType::ReloadSounds:
            if (m_soundHandler) {
                auto result = m_soundHandler->reload();
                if (result.failed()) {
                    spdlog::warn("[AudioService] Reload sounds failed: {}", result.error().toString());
                }
            }
            break;

        case CommandType::SetBiomeId:
            if (m_biomeAmbientHandler) {
                m_biomeAmbientHandler->setBiomeId(command.biomeId);
            }
            break;

        case CommandType::SetUnderwater:
            m_savedUnderwater.store(command.underwater);
            if (m_underwaterAmbientHandler) {
                m_underwaterAmbientHandler->setUnderwater(command.underwater);
            }
            break;

        case CommandType::SetBubbleColumnState:
            if (m_bubbleColumnAmbientHandler) {
                m_bubbleColumnAmbientHandler->setBubbleColumnState(
                    command.bubbleColumn.inBubbleColumn,
                    command.bubbleColumn.isDrag
                );
            }
            break;

        case CommandType::UpdateMusicState:
            m_savedDimension.store(command.dimension);
            m_savedCreative.store(command.inCreative);
            m_savedBossFight.store(command.inBossFight);
            break;

        case CommandType::SetAmbientLightLevel:
            if (m_biomeAmbientHandler) {
                m_biomeAmbientHandler->setLightLevel(command.skyLight, command.blockLight);
            }
            break;

        case CommandType::SetAmbientPlayerPosition:
            if (m_biomeAmbientHandler) {
                m_biomeAmbientHandler->setPlayerPosition(command.playerX, command.playerY, command.playerZ);
            }
            break;

        case CommandType::SetInMenu:
            m_savedInMenu.store(command.inMenu);
            break;

        case CommandType::EntitySpawn:
            if (m_entitySoundHandler && m_soundEngine) {
                m_entitySoundHandler->onEntitySpawn(*m_soundEngine, static_cast<EntityId>(command.entityId), command.entityTypeId);
            }
            break;

        case CommandType::EntityRemove:
            if (m_entitySoundHandler) {
                m_entitySoundHandler->onEntityRemove(static_cast<EntityId>(command.entityId));
            }
            break;

        case CommandType::ElytraFlyingChanged:
            if (m_entitySoundHandler && m_soundEngine) {
                m_entitySoundHandler->onPlayerElytraFlyingChanged(*m_soundEngine, static_cast<EntityId>(command.entityId), command.isFlying);
            }
            break;

        case CommandType::EntityAngerStateChanged:
            // 愤怒状态已通过 getMutableEntityState 更新
            break;

        case CommandType::UpdateEntityPosition:
            // 位置和速度已通过 getMutableEntityState 更新
            break;

        case CommandType::GuardianAttack:
            // 守卫者攻击事件：创建 GuardianSound
            if (m_entitySoundHandler && m_soundEngine) {
                m_entitySoundHandler->onGuardianAttack(*m_soundEngine, static_cast<EntityId>(command.entityId));
            }
            break;

        case CommandType::GuardianTargetUpdate:
            // 守卫者目标更新：更新 attackAnimScale
            if (m_entitySoundHandler) {
                m_entitySoundHandler->onGuardianTargetChanged(
                    static_cast<EntityId>(command.entityId),
                    static_cast<EntityId>(command.targetEntityId)
                );
            }
            break;
    }
}

} // namespace mc::client::sound
