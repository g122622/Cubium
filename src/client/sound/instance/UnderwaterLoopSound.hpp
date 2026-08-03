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

#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::sound {

/**
 * @brief 水下循环音效
 *
 * 播放水下环境循环音效，具有音量渐入渐出效果。
 * 音量根据玩家在水中的时间渐变（40 ticks = 2秒）。
 *
 * 特点:
 * - 全局声音（无衰减）
 * - 循环播放
 * - 音量渐变 40 ticks
 * - 在水中时渐入（每tick +1）
 * - 不在水中时更快淡出（每tick -2）
 * - canBeSilent() 返回 true
 *
 * 使用示例:
 * @code
 * auto sound = std::make_unique<UnderwaterLoopSound>();
 * sound->setCanSwim(true);  // 玩家进入水中
 * engine.play(std::move(sound));
 * // 每帧调用 tick() 更新状态
 * @endcode
 */
class UnderwaterLoopSound : public TickableSound {
public:
    /**
     * @brief 构造函数
     */
    UnderwaterLoopSound();

    /**
     * @brief 每帧更新
     *
     * 根据玩家是否在水中更新音量渐变。
     */
    void tick() override;

    /**
     * @brief 设置玩家是否在水中
     *
     * @param canSwim 是否能游泳（是否在水中）
     */
    void setCanSwim(bool canSwim) { m_canSwim = canSwim; }

    /**
     * @brief 获取玩家是否在水中
     */
    [[nodiscard]] bool canSwim() const noexcept { return m_canSwim; }

    /**
     * @brief 是否可以静音播放
     */
    [[nodiscard]] bool canBeSilent() const override { return true; }

private:
    /// 在水中的tick计数（用于音量渐变）
    i32 m_ticksInWater = 0;

    /// 是否在水中
    bool m_canSwim = false;

    /// 渐变时间（ticks）
    static constexpr i32 FADE_TICKS = 40;
};

} // namespace mc::client::sound
