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

#include "SimpleTreeDecorators.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/BooleanProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/AgriculturalBlocks.hpp"
#include "common/world/block/registry/NaturalBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"

#include <algorithm>
#include <set>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace tree {
namespace decorator {

namespace {

/// MC LeaveVineDecorator.addHangingVine：先放一格藤蔓，再向下最多延伸 4 格。
void addHangingVine(const TreeDecoratorContext& context, const BlockPos& start, const BooleanProperty& face)
{
    context.placeVine(start, face);
    i32 remaining = 4;
    for (BlockPos pos = start.down(); context.isAir(pos) && remaining > 0; --remaining) {
        context.placeVine(pos, face);
        pos = pos.down();
    }
}

/// MC Feature.isGrassOrDirt：仅草/土类地面会被 AlterGround 替换。
bool isGrassOrDirt(const TreeDecoratorContext& context, const BlockPos& pos)
{
    const BlockState* state = context.region().getBlockState(pos);
    if (state == nullptr) {
        return false;
    }
    return state->is(VanillaBlocks::DIRT) || state->is(VanillaBlocks::GRASS_BLOCK) ||
        state->is(VanillaBlocks::COARSE_DIRT) || state->is(VanillaBlocks::PODZOL) || state->is(VanillaBlocks::FARMLAND);
}

} // namespace

// ============================================================================
// BeehiveDecorator
// ============================================================================

BeehiveDecorator::BeehiveDecorator(f32 probability)
    : m_probability(probability)
{}

void BeehiveDecorator::place(const TreeDecoratorContext& context) const
{
    const std::vector<BlockPos>& leaves = context.leaves();
    const std::vector<BlockPos>& logs = context.logs();
    if (logs.empty()) {
        return;
    }
    math::IRandom& random = context.random();
    if (random.nextFloat() >= m_probability) {
        return;
    }

    // 有树叶时取 max(最低叶 Y - 1, 最低干 Y + 1)；否则在树干高度范围内随机取一层。
    // logs 已按 Y 升序排序，front/back 即最低/最高。
    i32 targetY = 0;
    if (!leaves.empty()) {
        targetY = std::max(leaves.front().y - 1, logs.front().y + 1);
    } else {
        targetY = std::min(logs.front().y + 1 + random.nextInt(3), logs.back().y);
    }

    // SPAWN_DIRECTIONS = Plane.HORIZONTAL 去掉 WORLDGEN_FACING(SOUTH) 的反面(NORTH)。
    // Plane.HORIZONTAL 的顺序是 NORTH,EAST,SOUTH,WEST，剔除 NORTH 后即 EAST,SOUTH,WEST；
    // 顺序直接决定 shuffle 消耗的随机数如何映射到候选点，不能随意排列。
    static constexpr Direction kSpawnDirections[] = {Direction::East, Direction::South, Direction::West};
    std::vector<BlockPos> candidates;
    for (const BlockPos& log : logs) {
        if (log.y != targetY) {
            continue;
        }
        for (Direction dir : kSpawnDirections) {
            candidates.push_back(log.offset(dir));
        }
    }
    if (candidates.empty()) {
        return;
    }

    random.shuffle(candidates);
    for (const BlockPos& pos : candidates) {
        const BlockPos front = pos.offset(Direction::South);
        if (!context.isAir(pos) || !context.isAir(front)) {
            continue;
        }
        if (block_registry::NaturalBlocks::BEE_NEST != nullptr) {
            const BlockState* nest =
                &block_registry::NaturalBlocks::BEE_NEST->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
            context.setBlock(pos, nest);
        }
        // MC: level.getBlockEntity(pos, BEEHIVE).ifPresent(h -> {
        //         int j = 2 + random.nextInt(2);
        //         for (int k = 0; k < j; k++) h.storeBee(Occupant.create(random.nextInt(599)));
        //     });
        // 【必须照数消耗】写入蜜蜂会推进随机流；不消耗的话，同一棵树上后续装饰器
        // （例如 *_leaf_litter 树型的 place_on_ground）取点会整体错开。
        // TODO: worldgen 阶段尚未创建蜂巢 BlockEntity，「入场时带随机数量蜜蜂」的实际效果未实现，
        //       此处只对齐随机数消耗；待 worldgen 支持 BlockEntity 后补齐蜜蜂写入。
        const i32 beeCount = 2 + random.nextInt(2);
        for (i32 k = 0; k < beeCount; ++k) {
            (void)random.nextInt(599);
        }
        return; // 对应原版 findFirst：放置一个即止
    }
}

// ============================================================================
// LeaveVineDecorator
// ============================================================================

LeaveVineDecorator::LeaveVineDecorator(f32 probability)
    : m_probability(probability)
{}

void LeaveVineDecorator::place(const TreeDecoratorContext& context) const
{
    math::IRandom& random = context.random();
    for (const BlockPos& leaf : context.leaves()) {
        // 四个方向的判定顺序固定为 west → east → north → south，与原版书写顺序一致——
        // 每次都消耗一次 nextFloat，顺序变了随机流就错位。
        if (random.nextFloat() < m_probability) {
            const BlockPos pos = leaf.west();
            if (context.isAir(pos)) {
                addHangingVine(context, pos, BlockStateProperties::EAST());
            }
        }
        if (random.nextFloat() < m_probability) {
            const BlockPos pos = leaf.east();
            if (context.isAir(pos)) {
                addHangingVine(context, pos, BlockStateProperties::WEST());
            }
        }
        if (random.nextFloat() < m_probability) {
            const BlockPos pos = leaf.north();
            if (context.isAir(pos)) {
                addHangingVine(context, pos, BlockStateProperties::SOUTH());
            }
        }
        if (random.nextFloat() < m_probability) {
            const BlockPos pos = leaf.south();
            if (context.isAir(pos)) {
                addHangingVine(context, pos, BlockStateProperties::NORTH());
            }
        }
    }
}

// ============================================================================
// AttachedToLeavesDecorator
// ============================================================================

AttachedToLeavesDecorator::AttachedToLeavesDecorator(f32 probability,
    i32 exclusionRadiusXZ,
    i32 exclusionRadiusY,
    std::unique_ptr<state::BlockStateProvider> blockProvider,
    i32 requiredEmptyBlocks,
    std::vector<Direction> directions)
    : m_probability(probability)
    , m_exclusionRadiusXZ(exclusionRadiusXZ)
    , m_exclusionRadiusY(exclusionRadiusY)
    , m_blockProvider(std::move(blockProvider))
    , m_requiredEmptyBlocks(requiredEmptyBlocks)
    , m_directions(std::move(directions))
{}

bool AttachedToLeavesDecorator::_hasRequiredEmptyBlocks(
    const TreeDecoratorContext& context, const BlockPos& pos, Direction direction) const
{
    for (i32 i = 1; i <= m_requiredEmptyBlocks; ++i) {
        if (!context.isAir(pos.offset(direction, i))) {
            return false;
        }
    }
    return true;
}

void AttachedToLeavesDecorator::place(const TreeDecoratorContext& context) const
{
    if (m_directions.empty()) {
        return;
    }
    // MC: Util.shuffledCopy(ctx.leaves(), random) —— 先打乱副本再遍历，
    // 打乱本身消耗随机数，且遍历顺序影响后续每次取方向的随机数序列。
    std::vector<BlockPos> shuffled = context.leaves();
    math::IRandom& random = context.random();
    random.shuffle(shuffled);

    std::set<i64> excluded;
    for (const BlockPos& leaf : shuffled) {
        // MC: Util.getRandom(directions, random) == directions.get(random.nextInt(size))
        const Direction direction = m_directions[static_cast<size_t>(random.nextInt(static_cast<i32>(m_directions.size())))];
        const BlockPos target = leaf.offset(direction);
        if (excluded.count(BlockPos::asLong(target.x, target.y, target.z)) != 0) {
            continue;
        }
        if (random.nextFloat() >= m_probability) {
            continue;
        }
        if (!_hasRequiredEmptyBlocks(context, leaf, direction)) {
            continue;
        }
        // 放置成功后把该点四周 exclusion 半径内的位置全部标记为占用。
        const BlockPos lo(target.x - m_exclusionRadiusXZ, target.y - m_exclusionRadiusY, target.z - m_exclusionRadiusXZ);
        const BlockPos hi(target.x + m_exclusionRadiusXZ, target.y + m_exclusionRadiusY, target.z + m_exclusionRadiusXZ);
        for (i32 x = lo.x; x <= hi.x; ++x) {
            for (i32 y = lo.y; y <= hi.y; ++y) {
                for (i32 z = lo.z; z <= hi.z; ++z) {
                    excluded.insert(BlockPos::asLong(x, y, z));
                }
            }
        }
        const BlockState* state = m_blockProvider->getState(context.region(), random, target.x, target.y, target.z);
        if (state != nullptr) {
            context.setBlock(target, state);
        }
    }
}

// ============================================================================
// AlterGroundDecorator
// ============================================================================

AlterGroundDecorator::AlterGroundDecorator(std::unique_ptr<state::BlockStateProvider> provider)
    : m_provider(std::move(provider))
{}

void AlterGroundDecorator::place(const TreeDecoratorContext& context) const
{
    const std::vector<BlockPos>& roots = context.roots();
    const std::vector<BlockPos>& logs = context.logs();
    // 与 place_on_ground 同源：取最低树干或树根。
    std::vector<BlockPos> lowest;
    if (roots.empty()) {
        lowest = logs;
    } else if (!logs.empty() && roots.front().y == logs.front().y) {
        lowest = logs;
        lowest.insert(lowest.end(), roots.begin(), roots.end());
    } else {
        lowest = roots;
    }
    if (lowest.empty()) {
        return;
    }

    const i32 baseY = lowest.front().y;
    math::IRandom& random = context.random();
    for (const BlockPos& pos : lowest) {
        if (pos.y != baseY) {
            continue;
        }
        // 四个角各铺一个半径 2 的圆（略去对角，由 placeCircle 的 || 条件保证）。
        _placeCircle(context, pos.west().north());
        _placeCircle(context, pos.east(2).north());
        _placeCircle(context, pos.west().south(2));
        _placeCircle(context, pos.east(2).south(2));
        // 外圈再随机撒 5 个点：nextInt(64) 后按 8x8 网格解出偏移，仅取最外一圈。
        for (i32 j = 0; j < 5; ++j) {
            const i32 k = random.nextInt(64);
            const i32 l = k % 8;
            const i32 i1 = k / 8;
            if (l == 0 || l == 7 || i1 == 0 || i1 == 7) {
                _placeCircle(context, BlockPos(pos.x - 3 + l, pos.y, pos.z - 3 + i1));
            }
        }
    }
}

void AlterGroundDecorator::_placeCircle(const TreeDecoratorContext& context, const BlockPos& center) const
{
    for (i32 i = -2; i <= 2; ++i) {
        for (i32 j = -2; j <= 2; ++j) {
            // 只跳过四个对角，形成"圆"而非方。
            if (std::abs(i) != 2 || std::abs(j) != 2) {
                _placeBlockAt(context, BlockPos(center.x + i, center.y, center.z + j));
            }
        }
    }
}

void AlterGroundDecorator::_placeBlockAt(const TreeDecoratorContext& context, const BlockPos& pos) const
{
    // 自下而上找第一个草/土方块替换；一旦遇到非空气且已低于起点则停止
    // （对应原版 for (i = 2; i >= -3; i--) 中的 `!isAir && i < 0 → break`）。
    for (i32 i = 2; i >= -3; --i) {
        const BlockPos current = pos.up(i);
        if (isGrassOrDirt(context, current)) {
            const BlockState* state =
                m_provider->getState(context.region(), context.random(), pos.x, pos.y, pos.z);
            if (state != nullptr) {
                context.setBlock(current, state);
            }
            break;
        }
        if (!context.isAir(current) && i < 0) {
            break;
        }
    }
}

// ============================================================================
// CocoaDecorator
// ============================================================================

CocoaDecorator::CocoaDecorator(f32 probability)
    : m_probability(probability)
{}

void CocoaDecorator::place(const TreeDecoratorContext& context) const
{
    math::IRandom& random = context.random();
    if (random.nextFloat() >= m_probability) {
        return;
    }
    const std::vector<BlockPos>& logs = context.logs();
    if (logs.empty() || block_registry::AgriculturalBlocks::COCOA == nullptr) {
        return;
    }

    const i32 lowestY = logs.front().y;
    for (const BlockPos& log : logs) {
        if (log.y - lowestY > 2) {
            continue;
        }
        // 水平四向依次判定，每次消耗一次 nextFloat；顺序与原版 Plane.HORIZONTAL 一致。
        for (Direction direction : Directions::horizontal()) {
            if (random.nextFloat() > 0.25F) {
                continue;
            }
            const Direction opposite = Directions::opposite(direction);
            const BlockPos pos = log.offset(opposite);
            if (!context.isAir(pos)) {
                continue;
            }
            const BlockState* cocoa = &block_registry::AgriculturalBlocks::COCOA->defaultState()
                                           .with(BlockStateProperties::AGE_0_2(), random.nextInt(3))
                                           .with(BlockStateProperties::HORIZONTAL_FACING(), direction);
            context.setBlock(pos, cocoa);
        }
    }
}

} // namespace decorator
} // namespace tree
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
