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

namespace mc::client::sound {

/**
 * @brief 气泡柱环境音效处理器
 *
 * 当玩家进入气泡柱时播放相应音效。
 * 气泡柱有两种类型：
 * - 向上气泡柱（drag=false）：播放 BLOCK_BUBBLE_COLUMN_UPWARDS_INSIDE
 * - 向下气泡柱（drag=true）：播放 BLOCK_BUBBLE_COLUMN_WHIRLPOOL_INSIDE
 *
 * 使用示例:
 * @code
 * auto handler = std::make_unique<BubbleColumnAmbientHandler>();
 * engine.addAmbientHandler(std::move(handler));
 * // 每帧更新玩家位置
 * handler->setPlayerPosition(x, y, z);
 * @endcode
 */
class BubbleColumnAmbientHandler : public IAmbientSoundHandler {
public:
    BubbleColumnAmbientHandler();
    ~BubbleColumnAmbientHandler() override = default;

    /**
     * @brief 每帧更新
     *
     * 检测玩家是否进入气泡柱，播放相应音效。
     *
     * @param engine 声音引擎
     */
    void tick(SoundEngine& engine) override;

    /**
     * @brief 设置玩家是否在气泡柱中
     *
     * @param inBubbleColumn 是否在气泡柱中
     * @param isDrag 是否是向下气泡柱（whirlpool）
     */
    void setBubbleColumnState(bool inBubbleColumn, bool isDrag)
    {
        m_isInBubbleColumn = inBubbleColumn;
        m_isDrag = isDrag;
    }

    /**
     * @brief 检查玩家是否在气泡柱中
     */
    [[nodiscard]] bool isInBubbleColumn() const noexcept { return m_isInBubbleColumn; }

private:
    /// 是否在气泡柱中
    bool m_isInBubbleColumn = false;
    /// 是否是向下气泡柱（whirlpool）
    bool m_isDrag = false;
    /// 上一帧是否在气泡柱中
    bool m_wasInBubbleColumn = false;
    /// 是否是第一帧（跳过进入音效）
    bool m_firstTick = true;
    /// 随机数生成器
    math::Random m_rng{0};
};

} // namespace mc::client::sound
