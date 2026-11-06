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

#include "StrongholdStructure.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/random/WorldgenRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockPos.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/structures/StrongholdPieces.hpp"

#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

namespace {

// 要塞分布环配置
// 环 0: 3 个要塞，距离 1408-2688
// 环 1: 3 个要塞，距离 4480-5760
// 环 2: 3 个要塞，距离 7552-8832
// 环 3: 4 个要塞，距离 10624-11904
// 环 4: 6 个要塞，距离 13696-14976
// 环 5: 10 个要塞，距离 16768-18048
// 环 6: 15 个要塞，距离 19840-21120
// 环 7: 21 个要塞，距离 22912-24192
// 总计: 3+3+3+4+6+10+15+21 = 65 个要塞
constexpr i32 RING_COUNTS[] = {3, 3, 3, 4, 6, 10, 15, 21};
constexpr i32 RING_DISTANCES[] = {1408, 4480, 7552, 10624, 13696, 16768, 19840, 22912};
constexpr i32 RING_SPREADS[] = {1280, 1280, 1280, 1280, 1280, 1280, 1280, 1280};

} // namespace

using namespace mc::Biomes;

const std::string StrongholdStructure::m_name = "stronghold";

StrongholdStructure::StrongholdStructure(ResourceLocation id)
    : Structure(std::move(id))
{}

StrongholdStructure::StrongholdStructure(ResourceLocation id, const Config& config)
    : Structure(std::move(id))
    , m_config(config)
{}

const biome::BiomeTag* StrongholdStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_STRONGHOLD();
}

bool StrongholdStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(generator);
    MC_UNUSED(rng);

    /**
     * @brief 检查指定区块是否命中预计算的要塞起点。
     *
     * TODO 当前实现仍未完成 Java 版 biome locate 校正，但至少必须保证：
     * 要塞不会在任意区块都返回 true，而是只在 65 个预计算环形位置上生成。
     */
    const i64 worldSeed = static_cast<i64>(world.seed());
    for (i32 index = 0; index < 65; ++index) {
        const auto [strongholdChunkX, strongholdChunkZ] = calculateStrongholdPos(index, worldSeed);
        if (strongholdChunkX == chunkX && strongholdChunkZ == chunkZ) {
            return true;
        }
    }

    return false;
}

std::unique_ptr<StructureStart> StrongholdStructure::generate(
    IChunkGenerator& generator, math::IRandom& /*rng*/, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 起始块在区块内的偏移是 2，不是中心 8：原版用的是 chunkPos.getBlockX(2)。
    const i32 startX = chunkX * world::CHUNK_WIDTH + 2;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH + 2;

    // 原版 generatePieces 的整段重试循环。三处要点缺一不可：
    //
    //  1. 每轮用**独立**的 `setLargeFeatureSeed(世界种子 + i, 区块)` 重置随机源，
    //     并用它替换全局世界随机数。要塞用的不是区块随机流——直接沿用外部 rng
    //     会让起点、每个片段的类型与旋转全部错位。
    //  2. 每轮先 resetPieces()（权重计数与 imposedPiece 归零），再重新构建起始楼梯。
    //     短链生成的森林里权重计数与上一轮共享，第二轮起的选择分布就偏了。
    //  3. 结束时整体 moveBelowSeaLevel(海平面, 最小 Y, random, 10)。
    //     漏掉它整座要塞会悬在 Y=64 的起始高度上（实测 52 个构件全在 64..74）。
    //
    // 终止条件同样来自原版：构件集非空且已经生成了传送门房间。
    const i64 worldSeed = static_cast<i64>(generator.seed());
    const i32 startY = m_config.minY;
    const BlockPos startPos(startX, startY, startZ);

    // 防御性上限：原版 while(true) 靠「权重耗尽」保证终止，而权重恰好全部耗尽的轮次
    // 可能同时卡在传送门房间的包围盒冲突上，缺少硬上限会让探针/服务端永久挂住。
    // 触发说明组装逻辑与条件不一致，必须报错而不是静默产出残缺要塞。
    constexpr i32 kMaxAttempts = 256;
    for (i32 attempt = 0;; ++attempt) {
        MC_ASSERT_RELEASE_MSG(attempt < kMaxAttempts,
            "stronghold assembly did not produce a portal room within retry limit; "
            "the piece weight bookkeeping or the portal room registration is inconsistent");
        start->pieces().clear();
        start->recalculateStructureSize();

        math::WorldgenRandom localRng(std::make_unique<math::JavaLegacyRandom>(static_cast<u64>(worldSeed + attempt)));
        localRng.setLargeFeatureSeed(worldSeed + attempt, chunkX, chunkZ);

        StrongholdStartStairs* startStairs = _generateStrongholdPieces(localRng, startPos, start->pieces());
        const bool hasPortalRoom = startStairs != nullptr && startStairs->portalRoom() != nullptr;

        start->recalculateStructureSize();

        const i32 seaLevel = ::mc::world::SEA_LEVEL;
        const i32 minY = ::mc::world::MIN_BUILD_HEIGHT;
        const StructureBoundingBox overall = start->getBoundingBox();
        const i32 topLimit = seaLevel - 10;
        i32 topY = (overall.maxY() - overall.minY() + 1) + minY + 1;
        if (topY < topLimit) {
            topY += localRng.nextInt(topLimit - topY);
        }
        const i32 offsetY = topY - overall.maxY();
        for (auto& piece : start->pieces()) {
            piece->offset(0, offsetY, 0);
        }
        start->recalculateStructureSize();

        if (!start->pieces().empty() && hasPortalRoom) {
            break;
        }
    }

    return start;
}

StrongholdStartStairs* StrongholdStructure::_generateStrongholdPieces(
    math::IRandom& rng, const BlockPos& startPos, std::vector<std::unique_ptr<StructurePiece>>& pieces) const
{
    // 起始楼梯。权重列表由 StrongholdStartStairs 构造函数新建（计数为 0），
    // 因此每轮重试都是干净的初始状态，不需要额外的 resetPieces。
    auto startStairs = std::make_unique<StrongholdStartStairs>(rng, startPos.x, startPos.z);
    StrongholdStartStairs* startStairsPtr = startStairs.get();
    pieces.push_back(std::move(startStairs));

    // 队列驱动组装。
    //
    // 原版 generatePieces 把所有待扩展分支压进 `StartPiece.pendingChildren`，然后
    //
    //     while (!pending.isEmpty()) {
    //         int j = random.nextInt(pending.size());
    //         StructurePiece next = pending.remove(j);
    //         next.addChildren(start, builder, random);
    //     }
    //
    // 那一次 `nextInt(size)` 既决定下一次扩展哪条分支，也消耗一个随机数。顺序遍历
    // （正序或倒序）能长出同样规模的树，但每次 buildComponent 内部消耗的随机数不同，
    // 整条随机流会从第一个不同的分支开始错位，最终构件集合、位置与旋转全部与原版不同。
    // 因此必须照原版的「随机取下标 + 从待扩展队列移除」来做。
    //
    // `pieces` 存放全部已生成片段（增长顺序与原版 addPiece 一致），
    // `pendingChildren` 只存放「还没扩展过的分支」。
    //
    // 【起始楼梯必须在循环外直接扩展】原版是
    //     builder.addPiece(start); start.addChildren(start, builder, random);
    //     List<StructurePiece> list = start.pendingChildren;
    //     while (!list.isEmpty()) { int j = random.nextInt(list.size()); list.remove(j).addChildren(...); }
    // 起始楼梯**不在** pendingChildren 里——它是在进循环之前直接展开的。若把它也塞进
    // 队列，队列长度为 1 时那次 `nextInt(1)` 就白白多消耗一个随机数（Java 的
    // `nextInt(1)` 虽然恒返回 0，但仍要消费一次 `next(31)`），整座要塞从第一个子构件
    // 起错位。
    std::vector<size_t> pendingChildren;

    // 终止条件来自权重耗尽（原版 canAddStructurePieces 返回 false，各分支开不出新片段）
    // 与每个片段各自的分支深度上限（原版 MAX_DEPTH=50，见 StrongholdPieces 的 depth > 50）。
    // 这里**不能**再加一层全局片段数上限：原版要塞的构件数可以到三百以上，多一层上限
    // 会在到达真实规模之前把它凭空截断（实测被截到 113 个而原版 316 个）。
    {
        const size_t before = pieces.size();
        pieces[0]->buildComponent(pieces[0].get(), pieces, rng);
        for (size_t i = before; i < pieces.size(); ++i) {
            pendingChildren.push_back(i);
        }
    }

    while (!pendingChildren.empty()) {
        const i32 pick = rng.nextInt(static_cast<i32>(pendingChildren.size()));
        MC_ASSERT_RELEASE(pick >= 0 && static_cast<size_t>(pick) < pendingChildren.size());
        const size_t index = pendingChildren[static_cast<size_t>(pick)];
        pendingChildren.erase(pendingChildren.begin() + pick);

        StructurePiece* current = pieces[index].get();
        if (current == nullptr) {
            continue;
        }

        const size_t before = pieces.size();
        current->buildComponent(current, pieces, rng);
        if (pieces.size() == before) {
        }
        // 新生成的片段加入待扩展队列，顺序与 addPiece 的追加顺序一致。
        for (size_t i = before; i < pieces.size(); ++i) {
            pendingChildren.push_back(i);
        }
    }

    return startStairsPtr;
}

std::pair<i32, i32> StrongholdStructure::calculateStrongholdPos(i32 index, i64 worldSeed)
{
    // 要塞分布算法
    // 8 个环，每个环有不同数量的要塞
    i32 ring = getRing(index);
    i32 ringIndex = index;
    for (i32 i = 0; i < ring; ++i) {
        ringIndex -= RING_COUNTS[i];
    }

    i32 count = RING_COUNTS[ring];
    i32 distance = RING_DISTANCES[ring];
    i32 spread = RING_SPREADS[ring];

    // 计算角度
    math::Random rng(worldSeed);
    [[maybe_unused]] const i32 skippedValue0 = rng.nextInt(); // 跳过一些值
    [[maybe_unused]] const i32 skippedValue1 = rng.nextInt();

    // 计算该要塞的角度
    f64 angleStep = 2.0 * mc::math::PI_DOUBLE / count;
    f64 angle = angleStep * ringIndex;

    // 添加随机偏移
    f64 randomOffset = (rng.nextDouble() - 0.5) * angleStep * 0.5;
    angle += randomOffset;

    // 计算距离
    i32 actualDistance = distance + rng.nextInt(spread);

    // 计算坐标
    i32 x = static_cast<i32>(std::cos(angle) * actualDistance);
    i32 z = static_cast<i32>(std::sin(angle) * actualDistance);

    // 转换为区块坐标
    return {x >> mc::world::CHUNK_SHIFT, z >> mc::world::CHUNK_SHIFT};
}

i32 StrongholdStructure::getRing(i32 index) noexcept
{
    i32 cumulative = 0;
    for (i32 ring = 0; ring < 8; ++ring) {
        cumulative += RING_COUNTS[ring];
        if (index < cumulative) {
            return ring;
        }
    }
    return 7;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
