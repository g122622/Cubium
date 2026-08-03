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
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {

// 前向声明
class Entity;

namespace client::sound {

/**
 * @brief 实体跟随声音实例
 *
 * 声音位置自动跟随实体移动。
 * 当实体被移除或变为静音时自动停止。
 *
 * 使用示例:
 * @code
 * // 播放矿车声音
 * auto sound = std::make_unique<EntitySoundInstance>(
 *     ResourceLocation("minecraft:entity.minecart.inside"),
 *     SoundCategory::Blocks,
 *     minecartEntity,
 *     1.0f,  // volume
 *     1.0f   // pitch
 * );
 * sound->setLooping(true);
 * engine.play(std::move(sound));
 *
 * // 播放生物受伤声音
 * auto sound = std::make_unique<EntitySoundInstance>(
 *     ResourceLocation("minecraft:entity.cow.hurt"),
 *     SoundCategory::Neutral,
 *     cowEntity,
 *     1.0f,  // volume
 *     1.0f   // pitch
 * );
 * engine.play(std::move(sound));
 * @endcode
 */
class EntitySoundInstance : public TickableSound {
public:
    /**
     * @brief 构造实体跟随声音
     *
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param entity 实体引用（必须是有效引用）
     * @param volume 音量
     * @param pitch 音调
     */
    EntitySoundInstance(
        const ResourceLocation& soundEventId, SoundCategory category, Entity& entity, f32 volume, f32 pitch);

    /**
     * @brief 析构函数
     */
    ~EntitySoundInstance() override = default;

    // 禁止拷贝
    EntitySoundInstance(const EntitySoundInstance&) = delete;
    EntitySoundInstance& operator=(const EntitySoundInstance&) = delete;

    // 允许移动
    EntitySoundInstance(EntitySoundInstance&&) noexcept = default;
    EntitySoundInstance& operator=(EntitySoundInstance&&) noexcept = default;

    // ========================================================================
    // TickableSound 接口
    // ========================================================================

    /**
     * @brief 每帧更新
     *
     * 更新声音位置以跟随实体。
     * 如果实体被移除或静音，标记声音为完成。
     */
    void tick() override;

    // ========================================================================
    // EntitySoundInstance 特有方法
    // ========================================================================

    /**
     * @brief 获取关联的实体
     */
    [[nodiscard]] Entity& entity() const { return m_entity; }

    /**
     * @brief 检查实体是否仍然有效（未被移除且未静音）
     */
    [[nodiscard]] bool isEntityValid() const;

private:
    Entity& m_entity;
};

} // namespace client::sound
} // namespace mc
