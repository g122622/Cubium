#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>

namespace mc {
namespace world {
namespace biome {

/**
 * @brief 心境音效配置
 *
 * 定义心境音效的参数。心境音效是在黑暗环境中随机播放的环境音效。
 *
 * 参考: net.minecraft.world.biome.MoodSoundAmbience
 *
 * 使用示例:
 * @code
 * MoodSoundAmbience caveMood(
 *     ResourceLocation("minecraft:ambient.cave"),
 *     6000,  // tick_delay - 用于光照计算
 *     8,     // block_search_extent - 随机位置范围
 *     2.0    // offset - 距离玩家的偏移
 * );
 * @endcode
 */
class MoodSoundAmbience {
public:
    MoodSoundAmbience() = default;

    MoodSoundAmbience(const ResourceLocation& soundEvent,
                      i32 tickDelay,
                      i32 blockSearchExtent,
                      f64 offset)
        : m_soundEvent(soundEvent)
        , m_tickDelay(tickDelay)
        , m_blockSearchExtent(blockSearchExtent)
        , m_offset(offset)
    {
    }

    [[nodiscard]] const ResourceLocation& soundEvent() const { return m_soundEvent; }
    [[nodiscard]] i32 tickDelay() const { return m_tickDelay; }
    [[nodiscard]] i32 blockSearchExtent() const { return m_blockSearchExtent; }
    [[nodiscard]] f64 offset() const { return m_offset; }

    /// 默认洞穴心境音效 (MC 1.16.5)
    static MoodSoundAmbience defaultCaveMood() {
        return MoodSoundAmbience(
            ResourceLocation("minecraft:ambient.cave"),
            6000,  // tick_delay
            8,     // block_search_extent
            2.0    // offset
        );
    }

private:
    ResourceLocation m_soundEvent;
    i32 m_tickDelay = 6000;
    i32 m_blockSearchExtent = 8;
    f64 m_offset = 2.0;
};

/**
 * @brief 附加音效配置
 *
 * 定义附加音效的参数。附加音效是按概率随机播放的环境音效。
 *
 * 参考: net.minecraft.world.biome.SoundAdditionsAmbience
 *
 * 使用示例:
 * @code
 * SoundAdditionsAmbience netherAdditions(
 *     ResourceLocation("minecraft:ambient.nether_wastes.additions"),
 *     0.0111  // tick_chance - 每tick播放概率
 * );
 * @endcode
 */
class SoundAdditionsAmbience {
public:
    SoundAdditionsAmbience() = default;

    SoundAdditionsAmbience(const ResourceLocation& soundEvent, f64 tickChance)
        : m_soundEvent(soundEvent)
        , m_tickChance(tickChance)
    {
    }

    [[nodiscard]] const ResourceLocation& soundEvent() const { return m_soundEvent; }
    [[nodiscard]] f64 tickChance() const { return m_tickChance; }

private:
    ResourceLocation m_soundEvent;
    f64 m_tickChance = 0.0;
};

/**
 * @brief 生物群系环境音效配置
 *
 * 存储生物群系的三种环境音效：循环音效、心境音效和附加音效。
 *
 * 参考: net.minecraft.world.biome.BiomeAmbience 的音效相关字段
 */
class BiomeAmbientSounds {
public:
    BiomeAmbientSounds() = default;

    // === Getters ===

    [[nodiscard]] const std::optional<ResourceLocation>& loopSound() const { return m_loopSound; }
    [[nodiscard]] const std::optional<MoodSoundAmbience>& moodSound() const { return m_moodSound; }
    [[nodiscard]] const std::optional<SoundAdditionsAmbience>& additionsSound() const { return m_additionsSound; }

    // === Setters ===

    void setLoopSound(const ResourceLocation& sound) { m_loopSound = sound; }
    void setMoodSound(const MoodSoundAmbience& mood) { m_moodSound = mood; }
    void setAdditionsSound(const SoundAdditionsAmbience& additions) { m_additionsSound = additions; }

    /// 是否有任何环境音效
    [[nodiscard]] bool hasAnySound() const {
        return m_loopSound.has_value() ||
               m_moodSound.has_value() ||
               m_additionsSound.has_value();
    }

private:
    std::optional<ResourceLocation> m_loopSound;
    std::optional<MoodSoundAmbience> m_moodSound;
    std::optional<SoundAdditionsAmbience> m_additionsSound;
};

} // namespace biome
} // namespace world
} // namespace mc
