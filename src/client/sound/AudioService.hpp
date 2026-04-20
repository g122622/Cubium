#pragma once

#include "client/sound/instance/ISoundInstance.hpp"

#include <glm/glm.hpp>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

namespace mc {

class ResourcePackList;

namespace client {
class ClientSettings;

namespace sound {

class BiomeAmbientHandler;
class SoundEngine;
class SoundHandler;
class UnderwaterAmbientHandler;

/**
 * @brief 音频服务
 *
 * 客户端唯一的音频入口。所有 OpenAL 调用、SoundEngine 生命周期、环境音与音乐逻辑
 * 都在独立的“音频引擎线程”内执行。
 *
 * 主线程只负责投递命令，不直接接触 SoundEngine，也不直接触碰 OpenAL。
 */
class AudioService {
public:
    explicit AudioService(ResourcePackList& resourcePacks, ClientSettings& settings);
    ~AudioService();

    AudioService(const AudioService&) = delete;
    AudioService& operator=(const AudioService&) = delete;

    [[nodiscard]] Result<void> initialize();
    void shutdown();

    [[nodiscard]] bool isLoaded() const noexcept { return m_loaded.load(); }

    void play(std::unique_ptr<ISoundInstance> sound);
    void playDelayed(std::unique_ptr<ISoundInstance> sound, u32 delayTicks);
    void stop(SoundInstanceId id);
    void stop(const ResourceLocation& soundEventId);
    void stop(SoundCategory category);
    void stopAll();
    void pause();
    void resume();
    void setPaused(bool paused);
    void updateListener(const glm::vec3& position,
                        const glm::vec3& forward,
                        const glm::vec3& up);
    void setListenerVelocity(const glm::vec3& velocity);
    void setVolume(SoundCategory category, f32 volume);
    void reloadSoundDefinitions();
    void setBiomeId(u32 biomeId);
    void setUnderwater(bool underwater);

private:
    enum class CommandType : u8 {
        Play,
        PlayDelayed,
        StopId,
        StopEvent,
        StopCategory,
        StopAll,
        SetPaused,
        UpdateListener,
        SetListenerVelocity,
        SetVolume,
        ReloadSounds,
        SetBiomeId,
        SetUnderwater,
    };

    struct Command {
        CommandType type = CommandType::Play;
        std::unique_ptr<ISoundInstance> sound;
        SoundInstanceId soundId = 0;
        ResourceLocation soundEventId;
        SoundCategory category = SoundCategory::Master;
        glm::vec3 position{0.0f};
        glm::vec3 forward{0.0f};
        glm::vec3 up{0.0f};
        glm::vec3 velocity{0.0f};
        f32 volume = 1.0f;
        u32 delayTicks = 0;
        u32 biomeId = 0;
        bool underwater = false;
        bool paused = false;
    };

    void enqueue(Command command);
    void runWorker();
    void processCommand(Command& command);

    ResourcePackList& m_resourcePacks;
    ClientSettings& m_settings;

    std::thread m_workerThread;
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
    std::deque<Command> m_commands;

    std::unique_ptr<SoundHandler> m_soundHandler;
    std::unique_ptr<SoundEngine> m_soundEngine;
    BiomeAmbientHandler* m_biomeAmbientHandler = nullptr;
    UnderwaterAmbientHandler* m_underwaterAmbientHandler = nullptr;

    std::mutex m_initMutex;
    std::condition_variable m_initConditionVariable;
    bool m_initComplete = false;
    Result<void> m_initResult = Result<void>::ok();

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_loaded{false};
    std::atomic<bool> m_paused{false};
};

} // namespace sound
} // namespace client
} // namespace mc

