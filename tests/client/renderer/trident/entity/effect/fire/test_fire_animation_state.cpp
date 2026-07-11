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

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/effect/fire/FireAnimationState.hpp"

#include "common/resource/metadata/AnimationMetadata.hpp"

namespace mc::test {
namespace {

using client::renderer::entity::effect::fire::FireAnimationState;
using resource::metadata::AnimationFrame;
using resource::metadata::AnimationMetadata;

// 构造默认动画元数据：frametime=1，无自定义帧序列
AnimationMetadata makeDefaultMetadata(i32 frametime = 1)
{
    AnimationMetadata md;
    md.frametime = frametime;
    md.width = 16;
    md.height = 16;
    return md;
}

// 构造带自定义帧序列的动画元数据
AnimationMetadata makeCustomSequenceMetadata(const std::vector<i32>& frameIndices, i32 frametime = 1)
{
    AnimationMetadata md;
    md.frametime = frametime;
    md.width = 16;
    md.height = 16;
    md.frames.reserve(frameIndices.size());
    for (i32 idx : frameIndices) {
        md.frames.emplace_back(idx, -1); // time=-1 表示用默认 frametime
    }
    return md;
}

// 构造带每帧独立时长的动画元数据
AnimationMetadata makePerFrameTimeMetadata(const std::vector<std::pair<i32, i32>>& indexTimePairs)
{
    AnimationMetadata md;
    md.frametime = 1;
    md.width = 16;
    md.height = 16;
    md.frames.reserve(indexTimePairs.size());
    for (const auto& [idx, time] : indexTimePairs) {
        md.frames.emplace_back(idx, time);
    }
    return md;
}

// ========== init() 测试 ==========

TEST(FireAnimationStateTest, InitWithDefaultMetadataSetsFrametime)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(3);
    state.init(md, 4);

    EXPECT_EQ(state.frameCount, 4u);
    EXPECT_EQ(state.frameCounter, 0u);
    EXPECT_EQ(state.tickCounter, 0);
    EXPECT_EQ(state.currentFrameTime, 3);
    EXPECT_EQ(state.currentFrameIndex(), 0);
}

TEST(FireAnimationStateTest, InitWithCustomSequenceUsesFirstFrameTime)
{
    FireAnimationState state;
    // 帧序列：[index=2 time=5, index=0 time=10]
    auto md = makePerFrameTimeMetadata({{2, 5}, {0, 10}});
    state.init(md, 3);

    // 第一帧 time=5
    EXPECT_EQ(state.currentFrameTime, 5);
    // 当前帧索引应为序列第一项的 index=2
    EXPECT_EQ(state.currentFrameIndex(), 2);
}

TEST(FireAnimationStateTest, InitWithEmptySequenceUsesFrametime)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(7);
    state.init(md, 3);

    // 无自定义帧序列，currentFrameTime 取 frametime
    EXPECT_EQ(state.currentFrameTime, 7);
    EXPECT_EQ(state.currentFrameIndex(), 0);
}

// ========== tick() 单帧无动画测试 ==========

TEST(FireAnimationStateTest, SingleFrameNoAnimationStaysAtFrame0)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(1);
    state.init(md, 1);

    // 即使 tick 多次，单帧动画应始终停留在帧 0
    for (int i = 0; i < 10; ++i) {
        state.tick();
        EXPECT_EQ(state.currentFrameIndex(), 0);
    }
}

TEST(FireAnimationStateTest, ZeroFrameCountTickIsNoop)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(1);
    state.init(md, 0);

    // frameCount=0 时 tick 不应崩溃也不应改变状态
    state.tick();
    EXPECT_EQ(state.frameCounter, 0u);
    EXPECT_EQ(state.tickCounter, 0);
}

// ========== tick() 多帧顺序播放测试 ==========

TEST(FireAnimationStateTest, SequentialPlaybackAdvancesFrameAfterFrametime)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(3); // frametime=3
    state.init(md, 4);                // 4 帧，顺序播放

    // 初始帧 0
    EXPECT_EQ(state.currentFrameIndex(), 0);

    // tick 1：仍在帧 0
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 0);
    EXPECT_EQ(state.tickCounter, 1);

    // tick 2：仍在帧 0
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 0);
    EXPECT_EQ(state.tickCounter, 2);

    // tick 3：达到 frametime=3，切换到帧 1
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 1);
    EXPECT_EQ(state.tickCounter, 0);
}

TEST(FireAnimationStateTest, SequentialPlaybackCyclesBackToFrame0)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(1); // frametime=1，每 tick 切帧
    state.init(md, 3);                // 3 帧：0, 1, 2

    // 逐 tick 验证循环：0 → 1 → 2 → 0 → 1 → 2 → ...
    for (int cycle = 0; cycle < 3; ++cycle) {
        EXPECT_EQ(state.currentFrameIndex(), cycle % 3);
        state.tick();
    }
    // 3 次 tick 后应回到帧 0
    EXPECT_EQ(state.currentFrameIndex(), 0);
}

// ========== tick() 自定义帧序列测试 ==========

TEST(FireAnimationStateTest, CustomSequenceFollowsFrameOrder)
{
    FireAnimationState state;
    // 帧序列 [1, 0, 2]：先播帧 1，再帧 0，再帧 2，循环
    auto md = makeCustomSequenceMetadata({1, 0, 2}, 1);
    state.init(md, 3);

    EXPECT_EQ(state.currentFrameIndex(), 1);
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 0);
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 2);
    state.tick();
    // 循环回序列第一项
    EXPECT_EQ(state.currentFrameIndex(), 1);
}

TEST(FireAnimationStateTest, CustomSequenceCyclesModulo)
{
    FireAnimationState state;
    // 帧序列 [2, 1]，frametime=2
    auto md = makeCustomSequenceMetadata({2, 1}, 2);
    state.init(md, 3);

    // 帧 2，tick 1 后仍在帧 2
    EXPECT_EQ(state.currentFrameIndex(), 2);
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 2);
    // tick 2 后切换到帧 1
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 1);
    // 帧 1 持续 2 tick
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 1);
    state.tick();
    // 循环回帧 2
    EXPECT_EQ(state.currentFrameIndex(), 2);
}

// ========== tick() 每帧独立时长测试 ==========

TEST(FireAnimationStateTest, PerFrameTimeRespected)
{
    FireAnimationState state;
    // 帧 0 持续 1 tick，帧 1 持续 3 tick，帧 2 持续 2 tick
    auto md = makePerFrameTimeMetadata({{0, 1}, {1, 3}, {2, 2}});
    state.init(md, 3);

    // 帧 0：1 tick 后切换
    EXPECT_EQ(state.currentFrameIndex(), 0);
    EXPECT_EQ(state.currentFrameTime, 1);
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 1);
    EXPECT_EQ(state.currentFrameTime, 3);

    // 帧 1：3 tick 后切换
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 1);
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 1);
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 2);
    EXPECT_EQ(state.currentFrameTime, 2);

    // 帧 2：2 tick 后切换
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 2);
    state.tick();
    // 循环回帧 0
    EXPECT_EQ(state.currentFrameIndex(), 0);
    EXPECT_EQ(state.currentFrameTime, 1);
}

// ========== frameProgress() 测试 ==========

TEST(FireAnimationStateTest, FrameProgressTracksTickRatio)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(4); // frametime=4
    state.init(md, 2);

    // 初始：progress = 0/4 = 0.0
    EXPECT_FLOAT_EQ(state.frameProgress(), 0.0f);

    state.tick();
    EXPECT_FLOAT_EQ(state.frameProgress(), 0.25f); // 1/4

    state.tick();
    EXPECT_FLOAT_EQ(state.frameProgress(), 0.5f); // 2/4

    state.tick();
    EXPECT_FLOAT_EQ(state.frameProgress(), 0.75f); // 3/4

    // tick 4 后切换帧，progress 重置
    state.tick();
    EXPECT_FLOAT_EQ(state.frameProgress(), 0.0f); // 新帧起始
}

TEST(FireAnimationStateTest, FrameProgressZeroWhenFrameTimeZero)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(1);
    state.init(md, 1);
    // 手动设置 currentFrameTime=0 测试边界
    state.currentFrameTime = 0;
    EXPECT_FLOAT_EQ(state.frameProgress(), 0.0f);
}

// ========== currentFrameIndex() 边界测试 ==========

TEST(FireAnimationStateTest, CurrentFrameIndexWithEmptySequenceReturnsCounter)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(1);
    state.init(md, 5);

    // 无自定义帧序列，currentFrameIndex 应等于 frameCounter
    EXPECT_EQ(state.currentFrameIndex(), 0);
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 1);
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 2);
}

// ========== nextFrameIndex() 测试 ==========

TEST(FireAnimationStateTest, NextFrameIndexWithEmptySequenceAdvancesByOne)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(1);
    state.init(md, 4); // 4 帧顺序播放

    // 初始 frameCounter=0，下一帧应为 1
    EXPECT_EQ(state.nextFrameIndex(), 1);
    state.tick();
    EXPECT_EQ(state.nextFrameIndex(), 2);
    state.tick();
    EXPECT_EQ(state.nextFrameIndex(), 3);
    state.tick();
    // 推进到帧 3 后，下一帧应回绕到 0
    EXPECT_EQ(state.nextFrameIndex(), 0);
}

TEST(FireAnimationStateTest, NextFrameIndexCyclesBackToZero)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(1);
    state.init(md, 3); // 3 帧：0, 1, 2

    // 推进到最后一帧
    state.tick(); // 0 → 1
    state.tick(); // 1 → 2
    EXPECT_EQ(state.currentFrameIndex(), 2);
    // 在最后一帧时，下一帧应回绕到 0
    EXPECT_EQ(state.nextFrameIndex(), 0);
}

TEST(FireAnimationStateTest, NextFrameIndexWithCustomSequenceFollowsOrder)
{
    FireAnimationState state;
    // 帧序列 [1, 0, 2]：下一帧依次为 0, 2, 1
    auto md = makeCustomSequenceMetadata({1, 0, 2}, 1);
    state.init(md, 3);

    // 当前帧 1，下一帧应为序列下一项的 index=0
    EXPECT_EQ(state.currentFrameIndex(), 1);
    EXPECT_EQ(state.nextFrameIndex(), 0);

    state.tick();
    // 当前帧 0，下一帧应为序列下一项的 index=2
    EXPECT_EQ(state.currentFrameIndex(), 0);
    EXPECT_EQ(state.nextFrameIndex(), 2);

    state.tick();
    // 当前帧 2，下一帧应回绕到序列首项 index=1
    EXPECT_EQ(state.currentFrameIndex(), 2);
    EXPECT_EQ(state.nextFrameIndex(), 1);
}

TEST(FireAnimationStateTest, NextFrameIndexWithSingleFrameReturnsSameIndex)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(1);
    state.init(md, 1); // 单帧

    // 单帧时下一帧索引应回绕到 0（即与当前帧相同）
    EXPECT_EQ(state.currentFrameIndex(), 0);
    EXPECT_EQ(state.nextFrameIndex(), 0);
}

TEST(FireAnimationStateTest, NextFrameIndexZeroFrameCountIsSafe)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(1);
    state.init(md, 0); // frameCount=0

    // frameCount=0 时 nextFrameIndex 不应崩溃，返回 0
    EXPECT_EQ(state.nextFrameIndex(), 0);
}

TEST(FireAnimationStateTest, NextFrameIndexWithPerFrameTimeRespectsSequence)
{
    FireAnimationState state;
    // 帧 0 (time=3), 帧 1 (time=2), 帧 2 (time=1)
    auto md = makePerFrameTimeMetadata({{0, 3}, {1, 2}, {2, 1}});
    state.init(md, 3);

    // 当前帧 0，下一帧应为 1
    EXPECT_EQ(state.currentFrameIndex(), 0);
    EXPECT_EQ(state.nextFrameIndex(), 1);

    // 推进 3 tick 切换到帧 1
    state.tick();
    state.tick();
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 1);
    EXPECT_EQ(state.nextFrameIndex(), 2);
}

// ========== interpolate 字段语义测试 ==========

TEST(FireAnimationStateTest, InterpolateFlagPreservedFromMetadata)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(2);
    md.interpolate = true;
    state.init(md, 3);

    // metadata.interpolate 应被 FireAnimationState 保留
    EXPECT_TRUE(state.metadata.interpolate);

    // 即使 interpolate=true，currentFrameIndex 仍按离散逻辑返回
    EXPECT_EQ(state.currentFrameIndex(), 0);
    state.tick();
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 1);
}

TEST(FireAnimationStateTest, InterpolateDoesNotAffectTickAdvance)
{
    FireAnimationState state;
    auto md = makeDefaultMetadata(2);
    md.interpolate = true;
    state.init(md, 3);

    // interpolate=true 不应改变 tick 推进节奏
    // 帧 0 持续 2 tick
    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 0);
    EXPECT_FLOAT_EQ(state.frameProgress(), 0.5f);

    state.tick();
    EXPECT_EQ(state.currentFrameIndex(), 1);
    EXPECT_FLOAT_EQ(state.frameProgress(), 0.0f);
}

// ========== 插值场景下的 nextFrameIndex 综合测试 ==========

TEST(FireAnimationStateTest, InterpolationPairCurrentAndNextFrameIndices)
{
    FireAnimationState state;
    // 4 帧顺序播放，interpolate=true，frametime=3
    auto md = makeDefaultMetadata(3);
    md.interpolate = true;
    state.init(md, 4);

    // 帧 0 progress=0/3：当前=0，下一=1
    EXPECT_EQ(state.currentFrameIndex(), 0);
    EXPECT_EQ(state.nextFrameIndex(), 1);
    EXPECT_FLOAT_EQ(state.frameProgress(), 0.0f);

    state.tick();
    // 帧 0 progress=1/3：当前=0，下一=1
    EXPECT_EQ(state.currentFrameIndex(), 0);
    EXPECT_EQ(state.nextFrameIndex(), 1);
    EXPECT_FLOAT_EQ(state.frameProgress(), 1.0f / 3.0f);

    state.tick();
    // 帧 0 progress=2/3：当前=0，下一=1
    EXPECT_EQ(state.currentFrameIndex(), 0);
    EXPECT_EQ(state.nextFrameIndex(), 1);
    EXPECT_FLOAT_EQ(state.frameProgress(), 2.0f / 3.0f);

    state.tick();
    // 切换到帧 1，progress 重置
    EXPECT_EQ(state.currentFrameIndex(), 1);
    EXPECT_EQ(state.nextFrameIndex(), 2);
    EXPECT_FLOAT_EQ(state.frameProgress(), 0.0f);
}

} // namespace
} // namespace mc::test
