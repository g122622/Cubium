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

namespace mc::client::sound {

class SoundEngine;

/**
 * @brief 环境音效处理器接口
 *
 * 环境音效处理器负责根据游戏状态播放环境音效，
 * 如生物群系背景音、水下音效、气泡柱音效等。
 *
 * 使用示例:
 * @code
 * class BiomeAmbientHandler : public IAmbientSoundHandler {
 * public:
 *     void tick(SoundEngine& engine) override {
 *         // 检查玩家所在群系
 *         auto biome = player.getBiome();
 *         if (biome == BiomeId::Swamp) {
 *             // 播放沼泽环境音
 *         }
 *     }
 * };
 * @endcode
 */
class IAmbientSoundHandler {
public:
    virtual ~IAmbientSoundHandler() = default;

    /**
     * @brief 每帧更新
     *
     * 检查游戏状态并在需要时播放环境音效。
     * 此方法每帧调用一次，应避免昂贵的操作。
     *
     * @param engine 声音引擎
     */
    virtual void tick(SoundEngine& engine) = 0;
};

} // namespace mc::client::sound
