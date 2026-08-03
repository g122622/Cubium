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

#include "client/sound/instance/ISoundInstance.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/PackRepository.hpp"
#include "common/world/biome/BiomeAmbientSounds.hpp"

#include <glm/glm.hpp>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <glm/ext/vector_float3.hpp>

namespace mc {

namespace client {
class ClientSettings;

namespace sound {

class BiomeAmbientHandler;
class BubbleColumnAmbientHandler;
class EntitySoundHandler;
class MusicPlayer;
class SoundEngine;
class SoundHandler;
class UnderwaterAmbientHandler;
class WeatherSoundHandler;

/**
 * @brief 音频服务
 *
 * 客户端唯一的音频入口。所有 OpenAL 调用、SoundEngine 生命周期、环境音与音乐逻辑
 * 都在独立的"音频引擎线程"内执行。
 *
 * 主线程只负责投递命令，不直接接触 SoundEngine，也不直接触碰 OpenAL。
 */
class AudioService {
public:
    explicit AudioService(PackRepository& resourcePacks, ClientSettings& settings);
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
    void updateListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);
    void setListenerVelocity(const glm::vec3& velocity);
    void setVolume(SoundCategory category, f32 volume);
    void reloadSoundDefinitions();
    void setBiomeId(u32 biomeId);
    void setUnderwater(bool underwater);
    void setBubbleColumnState(bool inBubbleColumn, bool isDrag);

    /**
     * @brief 更新环境音效处理器的光照等级
     *
     * 用于心境音效的触发计算。
     *
     * @param skyLight 玩家眼睛位置天空光照等级 (0-15)
     * @param blockLight 玩家眼睛位置方块光照等级 (0-15)
     * @param moodSkyLight 心境音效采样位置天空光照等级 (0-15)
     * @param moodBlockLight 心境音效采样位置方块光照等级 (0-15)
     */
    void setAmbientLightLevel(u8 skyLight, u8 blockLight, u8 moodSkyLight, u8 moodBlockLight);

    /**
     * @brief 更新环境音效处理器的玩家位置和心境采样位置
     *
     * 用于心境音效的位置计算。心境采样位置由主线程在查询光照时一并采样得到，
     * 音频线程直接复用该位置计算声音播放方向，与 MC 原版在同一 tick 中用
     * 同一位置查询光照和播放声音的行为对齐。
     *
     * @param x 玩家X坐标
     * @param y 玩家Y坐标（眼睛高度）
     * @param z 玩家Z坐标
     * @param moodBx 心境音效采样位置X坐标（主线程已采样）
     * @param moodBy 心境音效采样位置Y坐标（主线程已采样）
     * @param moodBz 心境音效采样位置Z坐标（主线程已采样）
     */
    void setAmbientPlayerPosition(f64 x, f64 y, f64 z, i32 moodBx, i32 moodBy, i32 moodBz);

    /**
     * @brief 更新音乐播放器
     *
     * @param dimension 当前维度ID (0=主世界, -1=下界, 1=末地)
     * @param inCreative 是否在创造模式
     * @param inBossFight 是否在Boss战斗中
     * @param biomeMusic 当前生物群系的音乐配置（可选）
     */
    void updateMusicState(i32 dimension,
        bool inCreative,
        bool inBossFight,
        bool inOceanOrRiverBiome,
        const std::optional<world::biome::BiomeMusic>& biomeMusic = std::nullopt);

    /**
     * @brief 设置菜单状态
     *
     * @param inMenu 是否在菜单界面
     */
    void setInMenu(bool inMenu);

    // ========================================================================
    // 天气音效处理
    // ========================================================================

    /**
     * @brief 更新天气状态
     *
     * 用于天气音效处理器（雨声/雷声）。
     *
     * @param rainStrength 降雨强度 (0.0 - 1.0)
     * @param thunderStrength 雷暴强度 (0.0 - 1.0)
     * @param canSeeSky 玩家眼睛位置是否能看到天空（基于天空光照判断）
     */
    void updateWeatherState(f32 rainStrength, f32 thunderStrength, bool canSeeSky);

    // ========================================================================
    // 实体声音处理
    // ========================================================================

    /**
     * @brief 处理实体生成事件
     *
     * 在音频线程中创建实体特定的声音（如蜜蜂飞行声音）。
     *
     * @param entityId 实体ID
     * @param typeId 实体类型ID（如 "minecraft:bee"）
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     */
    void onEntitySpawn(u32 entityId, const std::string& typeId, f32 x, f32 y, f32 z);

    /**
     * @brief 处理实体移除事件
     *
     * @param entityId 实体ID
     */
    void onEntityRemove(u32 entityId);

    /**
     * @brief 处理玩家鞘翅飞行状态变化
     *
     * @param entityId 玩家实体ID
     * @param isFlying 是否正在鞘翅飞行
     */
    void onPlayerElytraFlyingChanged(u32 entityId, bool isFlying);

    /**
     * @brief 更新实体愤怒状态
     *
     * 用于蜜蜂等实体的声音切换。
     *
     * @param entityId 实体ID
     * @param isAngry 是否愤怒
     */
    void onEntityAngerStateChanged(u32 entityId, bool isAngry);

    /**
     * @brief 更新实体位置和速度
     *
     * @param entityId 实体ID
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param vx X速度
     * @param vy Y速度
     * @param vz Z速度
     */
    void updateEntityPosition(u32 entityId, f32 x, f32 y, f32 z, f32 vx, f32 vy, f32 vz);

    /**
     * @brief 更新实体骑乘状态
     *
     * 当玩家骑乘/离开矿车时调用，触发相应的声音。
     *
     * @param entityId 玩家实体ID
     * @param isRiding 是否正在骑乘
     * @param vehicleId 载具实体ID（矿车ID）
     */
    void updateEntityRidingState(u32 entityId, bool isRiding, u32 vehicleId);

    /**
     * @brief 处理守卫者攻击事件（实体状态21）
     *
     * 创建 GuardianSound 并开始攻击动画。
     *
     * @param entityId 守卫者实体ID
     */
    void onGuardianAttack(u32 entityId);

    /**
     * @brief 更新守卫者攻击目标
     *
     * 当 TARGET_ENTITY 元数据变化时调用。
     * 如果 targetEntityId 为 0，停止攻击声音。
     *
     * @param entityId 守卫者实体ID
     * @param targetEntityId 攻击目标实体ID（0表示无目标）
     */
    void updateGuardianTarget(u32 entityId, u32 targetEntityId);

    /**
     * @brief 播放移动声音
     *
     * 创建跟随实体位置移动的声音。当实体被移除时自动停止。
     *
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param entityId 实体ID
     * @param volume 音量
     * @param pitch 音调
     */
    void playMovingSound(
        const ResourceLocation& soundEventId, SoundCategory category, u32 entityId, f32 volume, f32 pitch);

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
        SetBubbleColumnState,
        UpdateMusicState,
        SetAmbientLightLevel,
        SetAmbientPlayerPosition,
        SetInMenu,
        UpdateWeatherState,
        // 实体声音
        EntitySpawn,
        EntityRemove,
        ElytraFlyingChanged,
        EntityAngerStateChanged,
        UpdateEntityPosition,
        GuardianAttack,
        GuardianTargetUpdate,
        RidingStateChanged,
        MovingSound,
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
        f32 pitch = 1.0f;
        u32 delayTicks = 0;
        u32 biomeId = 0;
        bool underwater = false;
        bool paused = false;
        // 气泡柱状态
        struct BubbleColumnState {
            bool inBubbleColumn = false;
            bool isDrag = false;
        } bubbleColumn;
        // 音乐状态
        i32 dimension = 0;
        bool inCreative = false;
        bool inBossFight = false;
        bool inOceanOrRiverBiome = false;
        bool inMenu = false;
        // 生物群系音乐
        std::optional<world::biome::BiomeMusic> biomeMusic;
        // 环境音效光照等级
        u8 skyLight = 15;
        u8 blockLight = 15;
        // 心境音效采样位置光照等级
        u8 moodSkyLight = 0;
        u8 moodBlockLight = 0;
        // 环境音效玩家位置
        f64 playerX = 0.0;
        f64 playerY = 0.0;
        f64 playerZ = 0.0;
        // 心境音效采样位置（主线程已采样，音频线程直接复用）
        i32 moodBx = 0;
        i32 moodBy = 0;
        i32 moodBz = 0;
        // 实体声音
        u32 entityId = 0;
        std::string entityTypeId;
        bool isFlying = false;
        bool isAngry = false;
        // 实体速度
        f32 vx = 0.0f;
        f32 vy = 0.0f;
        f32 vz = 0.0f;
        // 守卫者目标
        u32 targetEntityId = 0;
        // 骑乘状态
        bool isRiding = false;
        u32 vehicleId = 0;
        // 天气状态
        f32 rainStrength = 0.0f;
        f32 thunderStrength = 0.0f;
        bool canSeeSky = true;
    };

    void _enqueue(Command command);
    void _runWorker();
    void _processCommand(Command& command);

    PackRepository& m_resourcePacks;
    ClientSettings& m_settings;

    std::thread m_workerThread;
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
    std::deque<Command> m_commands;

    std::unique_ptr<SoundHandler> m_soundHandler;
    std::unique_ptr<SoundEngine> m_soundEngine;
    std::unique_ptr<MusicPlayer> m_musicPlayer;
    BiomeAmbientHandler* m_biomeAmbientHandler = nullptr;
    UnderwaterAmbientHandler* m_underwaterAmbientHandler = nullptr;
    BubbleColumnAmbientHandler* m_bubbleColumnAmbientHandler = nullptr;
    EntitySoundHandler* m_entitySoundHandler = nullptr;
    WeatherSoundHandler* m_weatherSoundHandler = nullptr;

    // 音乐状态（跨线程共享）
    std::atomic<i32> m_savedDimension{0};
    std::atomic<bool> m_savedUnderwater{false};
    std::atomic<bool> m_savedCreative{false};
    std::atomic<bool> m_savedBossFight{false};
    std::atomic<bool> m_savedInOceanOrRiverBiome{false};
    std::atomic<bool> m_savedInMenu{false};

    // 生物群系音乐（需要互斥锁保护）
    std::mutex m_biomeMusicMutex;
    std::optional<world::biome::BiomeMusic> m_savedBiomeMusic;

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
