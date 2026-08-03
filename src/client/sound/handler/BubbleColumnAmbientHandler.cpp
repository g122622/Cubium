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

#include "client/sound/handler/BubbleColumnAmbientHandler.hpp"

#include <memory>

#include "client/sound/SoundEngine.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/sound/SoundEvents.hpp"

#include <chrono>
#include <utility>

namespace mc::client::sound {

BubbleColumnAmbientHandler::BubbleColumnAmbientHandler()
    : m_rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()))
{}

void BubbleColumnAmbientHandler::tick(SoundEngine& engine)
{
    // 检测玩家是否刚刚进入气泡柱
    if (m_isInBubbleColumn && !m_wasInBubbleColumn && !m_firstTick) {
        // 刚进入气泡柱，播放音效
        if (m_isDrag) {
            // 向下气泡柱（whirlpool）
            auto sound = std::make_unique<SoundInstance>(SoundInstance::createGlobal(
                SoundEvents::BLOCK_BUBBLE_COLUMN_WHIRLPOOL_INSIDE, SoundCategory::Blocks, 1.0f, 1.0f));
            engine.play(std::move(sound));
        } else {
            // 向上气泡柱
            auto sound = std::make_unique<SoundInstance>(SoundInstance::createGlobal(
                SoundEvents::BLOCK_BUBBLE_COLUMN_UPWARDS_INSIDE, SoundCategory::Blocks, 1.0f, 1.0f));
            engine.play(std::move(sound));
        }
    }

    // 更新状态
    m_wasInBubbleColumn = m_isInBubbleColumn;
    m_firstTick = false;
}

} // namespace mc::client::sound
