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
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/density/BlendedNoise.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"
#include <memory>
#include <utility>
#include <variant>

namespace mc::world::gen::density {

NoiseBindingVisitor::NoiseBindingVisitor(const RandomState& randomState, u64 worldSeed, bool enableSharing)
    : m_randomState(randomState)
    , m_worldSeed(worldSeed)
    , m_enableSharing(enableSharing)
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

    // 性能优化：纯拓扑子树跨区块共享（仅 create 期 buildRouterFromTemplate 启用）。
    // 此时 function 是 mapAll 深拷贝后的新节点，其子树已绑定完毕（占位已替换为真叶子，
    // 子节点若纯拓扑也已被本分支递归包装为 SharedTopology）。若整棵子树纯拓扑，移入
    // shared_ptr 包成 SharedTopology 返回——后续 createRouterCopy / NoiseChunk::apply 的
    // 每次 mapAll 遇到 SharedTopology 都零深拷贝（SharedTopology::mapAll 返回持同一
    // shared_ptr 的新包装）。仅含 Marker / per-chunk 可变节点的路径不共享，走原深拷贝。
    //
    // isShareable 对 SharedTopology 直接返回 true（语义不变量），故自底向上：叶子纯拓扑
    // →SharedTopology；复合节点（子已 SharedTopology）→ 整棵 SharedTopology；含 Marker
    // 的节点 → false 保留独立，但其纯拓扑子树已共享化。
    //
    // createRouterCopy（enableSharing=false）不跑此分支：m_router 已共享化，再次包装只会
    // 造成 SharedTopology 嵌套（虽数值正确但每区块冗余 make_unique）。透传分支对
    // SharedTopology 原样返回（SharedTopology::mapAll 已 make_unique 新轻量包装，内部零深拷贝）。
    if (m_enableSharing && isShareable(*function)) {
        return std::make_unique<SharedTopology>(std::shared_ptr<const DensityFunction>(std::move(function)));
    }

    // 含 Marker / per-chunk 可变节点 / 已共享化的 SharedTopology（createRouterCopy 透传）：
    // 原样返回（mapAll 的 make_unique 已深拷贝，保留新节点）
    return function;
}

bool NoiseBindingVisitor::isShareable(const DensityFunction& function)
{
    // 可共享叶子：compute 纯只读，无子 DF，无 mutable，无 per-chunk 绑定。
    if (dynamic_cast<const Constant*>(&function) != nullptr ||
        dynamic_cast<const YClampedGradient*>(&function) != nullptr ||
        dynamic_cast<const MappedNoise*>(&function) != nullptr ||
        dynamic_cast<const NoiseDensity*>(&function) != nullptr ||
        dynamic_cast<const ShiftNoise*>(&function) != nullptr ||
        dynamic_cast<const EndIslands*>(&function) != nullptr ||
        dynamic_cast<const BlendedNoise*>(&function) != nullptr ||
        dynamic_cast<const SharedTopology*>(&function) != nullptr) {
        return true;
    }

    // 可共享复合：递归所有子 DF const getter。
    if (const auto* n = dynamic_cast<const Clamp*>(&function)) {
        return isShareable(n->input());
    }
    if (const auto* n = dynamic_cast<const Mapped*>(&function)) {
        return isShareable(n->input());
    }
    if (const auto* n = dynamic_cast<const TwoArgument*>(&function)) {
        return isShareable(n->arg1()) && isShareable(n->arg2());
    }
    if (const auto* n = dynamic_cast<const Lerp*>(&function)) {
        return isShareable(n->delta()) && isShareable(n->start()) && isShareable(n->end());
    }
    if (const auto* n = dynamic_cast<const RangeChoice*>(&function)) {
        return isShareable(n->input()) && isShareable(n->whenInRange()) && isShareable(n->whenOutOfRange());
    }
    if (const auto* n = dynamic_cast<const ShiftedNoise*>(&function)) {
        return isShareable(n->shiftX()) && isShareable(n->shiftY()) && isShareable(n->shiftZ());
    }
    if (const auto* n = dynamic_cast<const WeirdScaledSampler*>(&function)) {
        return isShareable(n->input());
    }
    if (const auto* n = dynamic_cast<const FindTopSurface*>(&function)) {
        return isShareable(n->density()) && isShareable(n->upperBound());
    }
    if (const auto* n = dynamic_cast<const SharedHolder*>(&function)) {
        return isShareable(n->inner());
    }
    if (const auto* n = dynamic_cast<const CubicSpline*>(&function)) {
        // 递归 input + points() 中 variant 持有的嵌套子 CubicSpline
        if (!isShareable(n->input())) {
            return false;
        }
        for (const auto& point : n->points()) {
            const auto* childSpline = std::get_if<std::shared_ptr<CubicSpline>>(&point.value);
            if (childSpline != nullptr && *childSpline != nullptr) {
                if (!isShareable(**childSpline)) {
                    return false;
                }
            }
        }
        return true;
    }

    // 其他类型（Marker / BeardifierMarker / Beardifier / Cache2D / FlatCache /
    // CacheAllInCell / NoiseInterpolator / CellCache / CacheOnce / UnboundNoiseLeaf /
    // UnboundEndIslands / 未知）：保守判否，走每区块深拷贝。
    return false;
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
