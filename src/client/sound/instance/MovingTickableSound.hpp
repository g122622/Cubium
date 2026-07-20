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

#include "client/sound/handler/EntitySoundHandler.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc::client::sound {

/**
 * @brief 移动声音 - 跟随实体位置的声音
 *
 * 用于播放附加到实体上的移动声音，如闪电雷声、末影龙死亡声音等。
 * 声音会跟随实体位置更新，当实体被移除时自动停止。
 */
class MovingTickableSound : public TickableSound {
public:
    /**
     * @brief 构造移动声音
     *
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param handler 实体声音处理器（用于获取实体状态）
     * @param entityId 实体ID
     * @param volume 音量
     * @param pitch 音调
     */
    MovingTickableSound(const ResourceLocation& soundEventId,
        SoundCategory category,
        const EntitySoundHandler* handler,
        EntityInstanceId entityId,
        f32 volume,
        f32 pitch);

    /**
     * @brief 每帧更新
     *
     * 更新声音位置以跟随实体。
     * 如果实体被移除，标记声音为完成。
     */
    void tick() override;

    /**
     * @brief 是否可以在静音状态下播放
     *
     * 移动声音允许在静音状态下播放，以便正确处理声音切换。
     */
    [[nodiscard]] bool canBeSilent() const override { return true; }

private:
    const EntitySoundHandler* m_handler;
    EntityInstanceId m_entityId;
};

} // namespace mc::client::sound
