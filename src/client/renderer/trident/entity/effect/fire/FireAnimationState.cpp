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

#include "FireAnimationState.hpp"

namespace mc::client::renderer::entity::effect::fire {

void FireAnimationState::init(const resource::metadata::AnimationMetadata& md, u32 frames)
{
    metadata = md;
    frameCount = frames;
    frameCounter = 0;
    tickCounter = 0;
    currentFrameTime = md.getFrameTime(0);
    if (currentFrameTime < 1) {
        currentFrameTime = 1;
    }
}

void FireAnimationState::tick()
{
    if (frameCount == 0) {
        return;
    }

    ++tickCounter;

    if (tickCounter >= currentFrameTime) {
        tickCounter = 0;

        // 切换到下一帧
        const Size sequenceLength = metadata.frames.empty() ? frameCount : metadata.frames.size();
        if (sequenceLength > 0) {
            frameCounter = (frameCounter + 1) % sequenceLength;
        }

        // 更新当前帧时间
        currentFrameTime = metadata.getFrameTime(frameCounter);
        if (currentFrameTime < 1) {
            currentFrameTime = 1;
        }
    }
}

i32 FireAnimationState::currentFrameIndex() const noexcept
{
    if (metadata.frames.empty()) {
        return static_cast<i32>(frameCounter);
    }
    return metadata.frames[frameCounter % metadata.frames.size()].index;
}

f32 FireAnimationState::frameProgress() const noexcept
{
    if (currentFrameTime <= 0) {
        return 0.0f;
    }
    return static_cast<f32>(tickCounter) / static_cast<f32>(currentFrameTime);
}

} // namespace mc::client::renderer::entity::effect::fire
