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

#include "common/core/Types.hpp"
#include "common/world/gen/density/DensityFunction.hpp"

#include <memory>

namespace mc::world::gen {
class RandomState;
} // namespace mc::world::gen

namespace mc::world::gen::density {

/**
 * @brief 噪声绑定访问者 — 数据驱动 DF 树的 UnboundNoiseLeaf/UnboundEndIslands 占位替换
 *
 * 对应原版 1.21.11 RandomState.NoiseWiringHelper（mapAll visitor）。
 * RandomState::create / createRouterCopy 对数据驱动 noise_router 的 15 槽 DF 树调用
 * mapAll(*this)，本访问者遍历到每个节点：
 *
 * - UnboundNoiseLeaf（NormalNoise 系：noise/shifted_noise/shift_a/shift_b/shift/
 *   mapped_noise/weird_scaled_sampler）→ 调 rs.getOrCreateNoiseShared(name) 取
 *   shared_ptr<const NormalNoise>（name-hash via fromHashOf，与原版 getOrCreateNoise 一致），
 *   构造真实叶子 NoiseDensity/ShiftedNoise/ShiftNoise/MappedNoise/WeirdScaledSampler。
 * - UnboundNoiseLeaf（OldBlendedNoise）→ BlendedNoise，种子由
 *   positionalRandom.fromHashOf("minecraft:terrain") 派生（原版 wrapNew 走 fromHashOf("terrain")）。
 * - UnboundEndIslands → EndIslands(worldSeed)（原版 wrapNew 走 EndIslandDensityFunction(seed)）。
 * - 其余节点 → 原样返回（mapAll 的 make_unique 已深拷贝，保留新节点）。
 *
 * 关键：mapAll 各子类 override 已 make_unique 出新节点再传入 apply，故 apply 收到的是
 * 深拷贝后的节点；对占位节点替换为真实叶子，对普通节点直接返回（已是新拷贝）。
 * 这样一次 mapAll 同时完成深拷贝 + 占位绑定，createRouterCopy 每区块得到独立绑定树。
 */
class NoiseBindingVisitor final : public DensityFunction::Visitor {
public:
    NoiseBindingVisitor(const RandomState& randomState, u64 worldSeed);

    [[nodiscard]] std::unique_ptr<DensityFunction> apply(std::unique_ptr<DensityFunction> function) override;

private:
    /// 替换 UnboundNoiseLeaf 为真实噪声叶子（按 Kind 分发；消费占位并移出其子 DF）
    [[nodiscard]] std::unique_ptr<DensityFunction> bindUnbound(class UnboundNoiseLeaf leaf);

    const RandomState& m_randomState;
    u64 m_worldSeed;
};

} // namespace mc::world::gen::density
