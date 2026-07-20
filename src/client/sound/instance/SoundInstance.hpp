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

#include <atomic>

namespace mc::client::sound {

// 从 mc::sound 引入类型
using ::mc::sound::DEFAULT_ATTENUATION_DISTANCE;

/**
 * @brief 普通声音实例实现
 *
 * 支持全局声音、位置声音、音乐和唱片机声音的创建。
 *
 * 使用示例:
 * @code
 * // 创建全局声音（如音乐）
 * auto music = SoundInstance::createGlobal(
 *     ResourceLocation("minecraft:music.game"),
 *     SoundCategory::Music
 * );
 *
 * // 创建位置声音（如方块音效）
 * auto blockSound = SoundInstance::createLocated(
 *     ResourceLocation("minecraft:block.stone.break"),
 *     SoundCategory::Blocks,
 *     pos.x, pos.y, pos.z,
 *     1.0f,  // volume
 *     1.0f   // pitch
 * );
 *
 * // 创建唱片机音乐
 * auto record = SoundInstance::createRecord(
 *     ResourceLocation("minecraft:music_disc.blocks"),
 *     pos.x, pos.y, pos.z
 * );
 * @endcode
 */
class SoundInstance : public ISoundInstance {
public:
    /**
     * @brief 默认构造函数
     */
    SoundInstance() = default;

    /**
     * @brief 构造声音实例
     *
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param position 位置
     * @param volume 音量
     * @param pitch 音调
     * @param looping 是否循环
     * @param attenuation 衰减类型
     * @param attenuationDistance 衰减距离
     */
    SoundInstance(const ResourceLocation& soundEventId,
        SoundCategory category,
        const glm::vec3& position,
        f32 volume,
        f32 pitch,
        bool looping,
        AttenuationType attenuation,
        f32 attenuationDistance);

    // ========================================================================
    // 工厂方法
    // ========================================================================

    /**
     * @brief 创建全局声音
     *
     * 全局声音不受听者位置影响，如背景音乐、菜单音效。
     *
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param volume 音量（默认1.0）
     * @param pitch 音调（默认1.0）
     */
    [[nodiscard]] static SoundInstance createGlobal(
        const ResourceLocation& soundEventId, SoundCategory category, f32 volume = 1.0f, f32 pitch = 1.0f);

    /**
     * @brief 创建位置声音
     *
     * 声音从指定位置发出，随距离衰减。
     *
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param volume 音量（默认1.0）
     * @param pitch 音调（默认1.0）
     */
    [[nodiscard]] static SoundInstance createLocated(const ResourceLocation& soundEventId,
        SoundCategory category,
        f32 x,
        f32 y,
        f32 z,
        f32 volume = 1.0f,
        f32 pitch = 1.0f);

    /**
     * @brief 创建音乐声音
     *
     * 全局、流式播放的声音，如背景音乐。
     *
     * @param soundEventId 声音事件ID
     * @param volume 音量（默认1.0）
     * @param pitch 音调（默认1.0）
     */
    [[nodiscard]] static SoundInstance createMusic(
        const ResourceLocation& soundEventId, f32 volume = 1.0f, f32 pitch = 1.0f);

    /**
     * @brief 创建唱片机声音
     *
     * 位置声音，流式播放，大音量，无衰减。
     *
     * @param soundEventId 声音事件ID
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     */
    [[nodiscard]] static SoundInstance createRecord(const ResourceLocation& soundEventId, f32 x, f32 y, f32 z);

    // ========================================================================
    // ISoundInstance 接口实现
    // ========================================================================

    [[nodiscard]] const ResourceLocation& getSoundEventId() const override { return m_soundEventId; }
    [[nodiscard]] SoundCategory getCategory() const override { return m_category; }
    [[nodiscard]] f32 getVolume() const override { return m_volume; }
    [[nodiscard]] f32 getPitch() const override { return m_pitch; }
    [[nodiscard]] f32 getX() const override { return m_position.x; }
    [[nodiscard]] f32 getY() const override { return m_position.y; }
    [[nodiscard]] f32 getZ() const override { return m_position.z; }
    [[nodiscard]] bool isLooping() const override { return m_looping; }
    [[nodiscard]] u32 getRepeatDelay() const override { return m_repeatDelay; }
    [[nodiscard]] AttenuationType getAttenuationType() const override { return m_attenuationType; }
    [[nodiscard]] bool isGlobal() const override { return m_attenuationType == AttenuationType::None; }
    [[nodiscard]] f32 getAttenuationDistance() const override { return m_attenuationDistance; }
    [[nodiscard]] SoundInstanceId getId() const override { return m_id; }
    void setId(SoundInstanceId id) override { m_id = id; }
    [[nodiscard]] bool isDone() const override { return m_done; }

    // ========================================================================
    // 设置方法
    // ========================================================================

    /**
     * @brief 设置位置
     */
    void setPosition(const glm::vec3& position) { m_position = position; }

    /**
     * @brief 设置音量
     */
    void setVolume(f32 volume) override { m_volume = volume; }

    /**
     * @brief 设置音调
     */
    void setPitch(f32 pitch) override { m_pitch = pitch; }

    /**
     * @brief 设置循环
     */
    void setLooping(bool looping) { m_looping = looping; }

    /**
     * @brief 设置重复延迟
     */
    void setRepeatDelay(u32 delay) { m_repeatDelay = delay; }

    /**
     * @brief 标记为已完成
     *
     * 同时取消循环，与 MC 原版行为一致。
     */
    void markDone()
    {
        m_done = true;
        m_looping = false;
    }

protected:
    ResourceLocation m_soundEventId;
    SoundCategory m_category = SoundCategory::Master;
    glm::vec3 m_position{0.0f};
    f32 m_volume = 1.0f;
    f32 m_pitch = 1.0f;
    bool m_looping = false;
    u32 m_repeatDelay = 0;
    AttenuationType m_attenuationType = AttenuationType::Linear;
    f32 m_attenuationDistance = DEFAULT_ATTENUATION_DISTANCE;
    SoundInstanceId m_id = 0;
    bool m_done = false;
};

/**
 * @brief 可更新的声音实例
 *
 * 可以每帧更新位置、音量等属性的声音。
 * 适用于移动的实体发出的声音（如矿车、射出的箭）。
 *
 * 使用示例:
 * @code
 * class MinecartSound : public TickableSound {
 * public:
 *     MinecartSound(EntityInstanceId entityId, const ResourceLocation& soundEventId)
 *         : TickableSound(soundEventId, SoundCategory::Hostile)
 *         , m_entityId(entityId)
 *     {
 *         setLooping(true);
 *     }
 *
 *     void tick() override {
 *         // 更新位置跟随实体
 *         auto entity = world.getEntity(m_entityId);
 *         if (entity) {
 *             setPosition(entity->getPosition());
 *         } else {
 *             markDone(); // 实体消失，停止声音
 *         }
 *     }
 *
 * private:
 *     EntityInstanceId m_entityId;
 * };
 * @endcode
 */
class TickableSound : public SoundInstance {
public:
    using SoundInstance::SoundInstance;

    /**
     * @brief 是否已完成
     *
     * 可更新的声音在标记完成后会自动停止。
     */
    [[nodiscard]] bool isDone() const override { return SoundInstance::m_done; }

    /**
     * @brief 每帧更新
     *
     * 子类必须实现此方法来更新声音属性。
     */
    void tick() override = 0;
};

/**
 * @brief 有位置的声音基类
 *
 * 提供位置声音的便捷基类。
 * 已经被 SoundInstance 包含，保留作为语义区分。
 */
class LocatableSound : public SoundInstance {
public:
    using SoundInstance::SoundInstance;
};

} // namespace mc::client::sound
