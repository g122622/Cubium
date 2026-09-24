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

#include "PlaceOnGroundDecorator.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace tree {
namespace decorator {

namespace {

/**
 * @brief MC TreeFeature.getLowestTrunkOrRootOfTree
 *
 * 取"最低的树干或树根"用于定位落地区域：
 *   - 无树根 → 全部树干
 *   - 树根与树干的**最低点同高** → 两者合并（树根外露的树种，落叶铺满根与干周围）
 *   - 否则 → 仅树根
 * logs/roots 在上下文中已按 Y 升序排序，故取 front() 即为最低点。
 */
std::vector<BlockPos> getLowestTrunkOrRootOfTree(const TreeDecoratorContext& context)
{
    std::vector<BlockPos> result;
    const std::vector<BlockPos>& roots = context.roots();
    const std::vector<BlockPos>& logs = context.logs();
    if (roots.empty()) {
        result = logs;
    } else if (!logs.empty() && roots.front().y == logs.front().y) {
        result = logs;
        result.insert(result.end(), roots.begin(), roots.end());
    } else {
        result = roots;
    }
    return result;
}

} // namespace

PlaceOnGroundDecorator::PlaceOnGroundDecorator(
    i32 tries, i32 radius, i32 height, std::unique_ptr<state::BlockStateProvider> blockStateProvider)
    : m_tries(tries)
    , m_radius(radius)
    , m_height(height)
    , m_blockStateProvider(std::move(blockStateProvider))
{}

void PlaceOnGroundDecorator::place(const TreeDecoratorContext& context) const
{
    const std::vector<BlockPos> lowest = getLowestTrunkOrRootOfTree(context);
    if (lowest.empty()) {
        return;
    }

    // 以最低层（Y 等于最低点的那些树干/树根）的水平张成矩形为基准。
    const i32 baseY = lowest.front().y;
    i32 minX = lowest.front().x;
    i32 maxX = lowest.front().x;
    i32 minZ = lowest.front().z;
    i32 maxZ = lowest.front().z;
    for (const BlockPos& pos : lowest) {
        if (pos.y == baseY) {
            minX = std::min(minX, pos.x);
            maxX = std::max(maxX, pos.x);
            minZ = std::min(minZ, pos.z);
            maxZ = std::max(maxZ, pos.z);
        }
    }

    // MC: new BoundingBox(minX, baseY, minZ, maxX, baseY, maxZ).inflatedBy(radius, height, radius)
    const i32 boxMinX = minX - m_radius;
    const i32 boxMaxX = maxX + m_radius;
    const i32 boxMinY = baseY - m_height;
    const i32 boxMaxY = baseY + m_height;
    const i32 boxMinZ = minZ - m_radius;
    const i32 boxMaxZ = maxZ + m_radius;

    math::IRandom& random = context.random();
    for (i32 i = 0; i < m_tries; ++i) {
        // MC: nextIntBetweenInclusive(min, max)，与本项目 nextInt(min, max) 同为闭区间。
        // 三次抽取的顺序必须是 X → Y → Z，与原版逐次对应。
        const i32 x = random.nextInt(boxMinX, boxMaxX);
        const i32 y = random.nextInt(boxMinY, boxMaxY);
        const i32 z = random.nextInt(boxMinZ, boxMaxZ);
        _attemptToPlaceBlockAbove(context, BlockPos(x, y, z));
    }
}

void PlaceOnGroundDecorator::_attemptToPlaceBlockAbove(const TreeDecoratorContext& context, const BlockPos& pos) const
{
    const BlockPos above(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = context.region().getBlockState(above);

    // 条件 1：上方格为空气或藤蔓（原版 isAir() || is(VINE)）。
    const bool abovePlaceable = aboveState == nullptr || aboveState->isAir() ||
        (VanillaBlocks::VINE != nullptr && aboveState->is(VanillaBlocks::VINE));
    if (!abovePlaceable) {
        return;
    }

    // 条件 2：脚下是完整固体（原版 checkBlock(p -> p.isSolidRender())）。
    if (!context.checkBlock(pos, [](const BlockState& state) { return state.isSolid(); })) {
        return;
    }

    // 条件 3：该列的最高阻挡方块不高于上方格——否则说明该点在实心地面**以下**，
    // 在上面放方块会把它埋进地里（原版 getHeightmapPos(MOTION_BLOCKING_NO_LEAVES).getY() <= above.getY()）。
    const i32 surfaceY = context.region().getHeight(pos.x, pos.z, HeightmapType::MotionBlockingNoLeaves);
    if (surfaceY > above.y) {
        return;
    }

    const BlockState* state =
        m_blockStateProvider->getState(context.region(), context.random(), above.x, above.y, above.z);
    if (state != nullptr) {
        context.setBlock(above, state);
    }
}

} // namespace decorator
} // namespace tree
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
