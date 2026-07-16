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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/world/gen/density/NoiseBindingVisitor.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"

namespace mc::world::gen::density {

NoiseBindingVisitor::NoiseBindingVisitor(const RandomState& randomState, u64 worldSeed)
    : m_randomState(randomState)
    , m_worldSeed(worldSeed)
{}

std::unique_ptr<DensityFunction> NoiseBindingVisitor::apply(std::unique_ptr<DensityFunction> function)
{
    if (!function) {
        return nullptr;
    }

    // UnboundEndIslands → EndIslands(worldSeed)
    // 原版 NoiseWiringHelper: EndIslandDensityFunction 经 wrapNew 用 seed 构造。
    if (dynamic_cast<UnboundEndIslands*>(function.get()) != nullptr) {
        return std::make_unique<EndIslands>(m_worldSeed);
    }

    // UnboundNoiseLeaf → 真实噪声叶子（消费本占位，移出其子 DF 所有权）
    if (auto* leafPtr = dynamic_cast<UnboundNoiseLeaf*>(function.get())) {
        // 取回所有权：把 unique_ptr<DensityFunction> 里的 UnboundNoiseLeaf 转为独占，
        // 以便调用非 const releaseXxx() 移出子 DF。
        auto leaf = std::unique_ptr<UnboundNoiseLeaf>(leafPtr);
        function.release(); // 解除原 unique_ptr 所有权（leaf 现在持有）
        return bindUnbound(std::move(*leaf));
    }

    // 其余节点：mapAll 已 make_unique 出深拷贝新节点，原样返回
    return function;
}

std::unique_ptr<DensityFunction> NoiseBindingVisitor::bindUnbound(UnboundNoiseLeaf leaf)
{
    switch (leaf.kind()) {
        case UnboundNoiseLeaf::Kind::Noise: {
            // NoiseDensity: noise(x*xzScale, y*yScale, z*xzScale)
            // NormalNoise 由 rs.getOrCreateNoiseShared(name) 取（name-hash via fromHashOf）。
            auto noise = m_randomState.getOrCreateNoiseShared(leaf.noiseName());
            return std::make_unique<NoiseDensity>(noise, leaf.xzScale(), leaf.yScale());
        }
        case UnboundNoiseLeaf::Kind::MappedNoise: {
            // MappedNoise: fromValue + noise(x*xzScale, y*yScale, z*xzScale) * (toValue - fromValue)
            auto noise = m_randomState.getOrCreateNoiseShared(leaf.noiseName());
            const f64 fromValue = leaf.fromValue().value_or(0.0);
            const f64 toValue = leaf.toValue().value_or(0.0);
            return std::make_unique<MappedNoise>(noise, leaf.xzScale(), leaf.yScale(), fromValue, toValue);
        }
        case UnboundNoiseLeaf::Kind::ShiftedNoise: {
            // ShiftedNoise: noise(x*xzScale + shiftX, y*yScale + shiftY, z*xzScale + shiftZ)
            // 子 DF 经 UnboundNoiseLeaf::mapAll 已深拷贝为独立节点，移出供 ShiftedNoise 持有。
            auto noise = m_randomState.getOrCreateNoiseShared(leaf.noiseName());
            return std::make_unique<ShiftedNoise>(
                noise, leaf.xzScale(), leaf.yScale(), leaf.releaseShiftX(), leaf.releaseShiftY(), leaf.releaseShiftZ());
        }
        case UnboundNoiseLeaf::Kind::ShiftA: {
            auto noise = m_randomState.getOrCreateNoiseShared(leaf.noiseName());
            return std::make_unique<ShiftNoise>(noise, ShiftType::ShiftA);
        }
        case UnboundNoiseLeaf::Kind::ShiftB: {
            auto noise = m_randomState.getOrCreateNoiseShared(leaf.noiseName());
            return std::make_unique<ShiftNoise>(noise, ShiftType::ShiftB);
        }
        case UnboundNoiseLeaf::Kind::Shift: {
            auto noise = m_randomState.getOrCreateNoiseShared(leaf.noiseName());
            return std::make_unique<ShiftNoise>(noise, ShiftType::Shift);
        }
        case UnboundNoiseLeaf::Kind::WeirdScaledSampler: {
            // input 经 UnboundNoiseLeaf::mapAll 已深拷贝，移出供 WeirdScaledSampler 持有
            auto noise = m_randomState.getOrCreateNoiseShared(leaf.noiseName());
            return std::make_unique<WeirdScaledSampler>(leaf.releaseInput(), noise, leaf.weirdType());
        }
        case UnboundNoiseLeaf::Kind::OldBlendedNoise: {
            // 原版 NoiseWiringHelper: BlendedNoise 经 wrapNew 用 fromHashOf("minecraft:terrain")
            // 派生种子构造（与 worldSeed 无关）。
            auto terrainRng = m_randomState.positionalRandom().fromHashOf("minecraft:terrain");
            const u64 seed = static_cast<u64>(terrainRng->nextLong());
            return std::make_unique<BlendedNoise>(
                seed, leaf.xzScale(), leaf.yScale(), leaf.xzFactor(), leaf.yFactor(), leaf.smearScaleMultiplier());
        }
    }
    // Kind 枚举已全覆盖，到达此处的唯一可能是枚举越界（数据损坏）。fail-fast 而非静默返回空叶子，
    // 否则父 mapAll 会存入 null unique_ptr 子节点，后续 compute/mapAll 解引用空指针崩溃。
    MC_ASSERT_MSG(false, "NoiseBindingVisitor::bindUnbound: unhandled UnboundNoiseLeaf::Kind");
    return nullptr;
}

} // namespace mc::world::gen::density
