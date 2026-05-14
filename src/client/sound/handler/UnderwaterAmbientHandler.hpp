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

#include "client/sound/SoundEngine.hpp"
#include "client/sound/handler/IAmbientSoundHandler.hpp"
#include "common/util/math/random/Random.hpp"

#include <optional>

namespace mc::client::sound {

// 前向声明
class UnderwaterLoopSound;

/**
 * @brief 水下环境音效处理器
 *
 * 当玩家在水下时播放水下环境音效。
 * 包括水下循环音效和三个稀有度级别的附加音效。
 *
 * 参考: net.minecraft.client.audio.UnderwaterAmbientSoundHandler
 * 参考: net.minecraft.client.entity.player.ClientPlayerEntity.updateEyesInWaterPlayer()
 *
 * 水下循环音效 (UnderwaterLoopSound):
 * - 玩家进入水时启动
 * - 自动淡入淡出（40 ticks）
 *
 * 附加音效稀有度概率:
 * - 普通音效: 0.9% 每tick
 * - 稀有音效: 0.09% 每tick
 * - 超稀有音效: 0.01% 每tick
 *
 * 使用示例:
 * @code
 * auto handler = std::make_unique<UnderwaterAmbientHandler>();
 * engine.addAmbientHandler(std::move(handler));
 * // 当玩家进入/离开水时:
 * handler->setUnderwater(inWater);
 * @endcode
 */
class UnderwaterAmbientHandler : public IAmbientSoundHandler {
public:
    UnderwaterAmbientHandler();
    ~UnderwaterAmbientHandler() override;

    /**
     * @brief 每帧更新
     *
     * 如果玩家在水下，按概率播放水下环境音效。
     * 更新水下循环音效的状态。
     *
     * @param engine 声音引擎
     */
    void tick(SoundEngine& engine) override;

    /**
     * @brief 设置玩家是否在水下
     *
     * 当状态从 false 变为 true 时，播放入水音效并启动水下循环音效。
     * 当状态从 true 变为 false 时，播放出水音效。
     *
     * @param underwater 是否在水下
     */
    void setUnderwater(bool underwater);

    /**
     * @brief 检查玩家是否在水下
     */
    [[nodiscard]] bool isUnderwater() const noexcept { return m_isUnderwater; }

private:
    /// 是否在水下
    bool m_isUnderwater = false;

    /// 上一帧是否在水下（用于检测状态变化）
    bool m_wasUnderwater = false;

    /// 水下循环音效ID
    SoundInstanceId m_underwaterLoopSoundId = 0;

    /// 随机数生成器
    math::Random m_rng{0};
};

} // namespace mc::client::sound
