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
 * @brief 噪声绑定访问者 — 数据驱动 DF 树的 UnboundNoiseLeaf/UnboundEndIslands 占位替换 +
 *        纯拓扑子树跨区块共享
 *
 * 对应原版 1.21.11 RandomState.NoiseWiringHelper（mapAll visitor）。
 *
 * 使用路径：create 期 buildRouterFromTemplate（唯一构造点）。对 DimensionSettings::m_routerDfs
 * 15 槽模板调用 mapAll(*this)，本访问者遍历到每个节点：
 * - UnboundNoiseLeaf（NormalNoise 系：noise/shifted_noise/shift_a/shift_b/shift/
 *   mapped_noise/weird_scaled_sampler）→ 调 rs.getOrCreateNoiseShared(name) 取
 *   shared_ptr<const NormalNoise>（name-hash via fromHashOf，与原版 getOrCreateNoise 一致），
 *   构造真实叶子 NoiseDensity/ShiftedNoise/ShiftNoise/MappedNoise/WeirdScaledSampler。
 * - UnboundNoiseLeaf（OldBlendedNoise）→ BlendedNoise，种子由
 *   positionalRandom.fromHashOf("minecraft:terrain") 派生（原版 wrapNew 走 fromHashOf("terrain")）。
 * - UnboundEndIslands → EndIslands(worldSeed)（原版 wrapNew 走 EndIslandDensityFunction(seed)）。
 * - 其余节点：若纯拓扑（isShareable）→ 包 SharedTopology 跨区块共享；否则原样返回。
 * 产出 m_router 共享化树（纯拓扑子树共享，Marker 路径独立）。
 *
 * 关键：mapAll 各子类 override 已 make_unique 出新节点再传入 apply，故 apply 收到的是
 * 深拷贝后的节点；对占位节点替换为真实叶子，对纯拓扑节点包装共享，对其余节点直接返回。
 */
class NoiseBindingVisitor final : public DensityFunction::Visitor {
public:
    /**
     * @brief 构造噪声绑定访问者
     *
     * @param randomState 随机状态（提供 getOrCreateNoiseShared / positionalRandom）
     * @param worldSeed 世界种子（EndIslands 构造用）
     *
     * 绑定 + 共享化：对 m_routerDfs 模板遍历，替换占位为真实叶子，纯拓扑子树包 SharedTopology
     * 跨区块共享，产出 m_router 共享化树。仅 create 期 buildRouterFromTemplate 单一调用点。
     */
    NoiseBindingVisitor(const RandomState& randomState, u64 worldSeed);

    [[nodiscard]] std::unique_ptr<DensityFunction> apply(std::unique_ptr<DensityFunction> function) override;

    /**
     * @brief 判定密度函数子树是否为纯拓扑（可跨区块共享只读）
     *
     * 纯拓扑子树：所有节点 compute() 无 this 上的可变状态（无 mutable 缓存、无 per-chunk
     * 绑定），计算结果只依赖输入坐标与不可变配置（含 shared_ptr<const NormalNoise>）。
     * 这类子树与区块无关，可被多个 NoiseChunk 并发只读共享。
     *
     * 判定策略（白名单正向判定，保守安全）：
     * - 可共享叶子（直接 true）：Constant / YClampedGradient / MappedNoise / NoiseDensity /
     *   ShiftNoise / EndIslands / BlendedNoise / SharedTopology / SharedHolder(递归 inner)
     * - 可共享复合（递归所有子 DF const getter）：Clamp / Mapped / TwoArgument / Lerp /
     *   RangeChoice / ShiftedNoise / WeirdScaledSampler / FindTopSurface / CubicSpline
     *   （CubicSpline 额外递归 points() 中 variant 持有的子 CubicSpline）
     * - 其他（直接 false）：Marker / BeardifierMarker / Beardifier / Cache2D / FlatCache /
     *   CacheAllInCell / NoiseInterpolator / CellCache / CacheOnce / UnboundNoiseLeaf /
     *   UnboundEndIslands 及任何未在白名单中的类型。保守策略：不确定时退化为不可共享，
     *   走每区块深拷贝，不破坏正确性。
     *
     * SharedTopology 视为可共享叶子（直接 true）：其语义前提是内部已纯拓扑（由本 visitor
     * 在 apply 中包装时保证），无需递归复核。自底向上提取时，子节点已被处理为 SharedTopology，
     * 父复合节点经此判定可整棵共享。
     *
     * @param function 待判定的密度函数（非空）
     * @return true 若子树全部纯拓扑可共享
     */
    [[nodiscard]] static bool isShareable(const DensityFunction& function);

private:
    /// 替换 UnboundNoiseLeaf 为真实噪声叶子（按 Kind 分发；消费占位并移出其子 DF）
    [[nodiscard]] std::unique_ptr<DensityFunction> bindUnbound(class UnboundNoiseLeaf leaf);

    const RandomState& m_randomState;
    u64 m_worldSeed;
};

} // namespace mc::world::gen::density
