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

#include "FallenTreeFeature.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/parser/BlockStateProviderParser.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace tree {

namespace {

/// MC FallenTreeFeature.getSidewaysStateModifier(direction)：
/// 把原木 axis 切到 direction 的轴（水平倒木）。非轴方块（无 AXIS 属性）保持原态
/// （对齐 MC trySetValue 语义：属性不存在时返回原状态）。
const BlockState* applySidewaysAxis(const BlockState* state, Direction direction)
{
    if (state == nullptr) {
        return state;
    }
    if (!state->hasProperty(BlockStateProperties::AXIS())) {
        return state;
    }
    return &state->with(BlockStateProperties::AXIS(), getAxis(direction));
}

/// MC Function.identity() 状态修饰：原样返回。
const BlockState* identityState(const BlockState* state)
{
    return state;
}

} // namespace

bool FallenTreeFeature::validTreePos(WorldGenRegion& region, const BlockPos& pos)
{
    // MC TreeFeature.validTreePos: isAir() || is(BlockTags.REPLACEABLE_BY_TREES)。
    const BlockState* state = region.getBlockState(pos);
    if (state == nullptr || state->isAir()) {
        return true;
    }
    return BlockTags::REPLACEABLE_BY_TREES().contains(*state);
}

bool FallenTreeFeature::isOverSolidGround(WorldGenRegion& region, const BlockPos& pos)
{
    // MC: getBlockState(below).isFaceSturdy(level, below, UP)。
    // MC isFaceSturdy 默认 SupportType.FULL（isFaceSturdy(reader,pos,dir) 三参重载）。
    const BlockState* below = region.getBlockState(pos.down());
    if (below == nullptr) {
        return false;
    }
    return below->isFaceSturdy(region, pos.down(), Direction::Up, SupportType::Full);
}

bool FallenTreeFeature::mayPlaceOn(WorldGenRegion& region, const BlockPos& pos)
{
    // MC: validTreePos(level, pos) && isOverSolidGround(level, pos)。
    return validTreePos(region, pos) && isOverSolidGround(region, pos);
}

BlockPos FallenTreeFeature::placeLogBlock(WorldGenRegion& region,
    const FallenTreeConfig& config,
    math::Random& random,
    const BlockPos& pos,
    const std::function<const BlockState*(const BlockState*)>& stateModifier)
{
    // MC: setBlock(pos, stateModifier.apply(trunkProvider.getState(random, pos)), 3)。
    const BlockState* sampled = config.trunkProvider->getState(region, random, pos.x, pos.y, pos.z);
    if (sampled == nullptr) {
        return pos;
    }
    const BlockState* finalState = stateModifier(sampled);
    region.setBlockState(pos, finalState, 3);
    return pos;
}

void FallenTreeFeature::decorateLogs(WorldGenRegion& region,
    math::Random& random,
    const std::vector<BlockPos>& logs,
    const std::vector<std::unique_ptr<decorator::TreeDecorator>>& decorators)
{
    // MC: if (decorators.isEmpty()) return;
    //     TreeDecorator.Context ctx = new Context(level, getDecorationSetter(level), random, logs, Set.of(), Set.of());
    //     decorators.forEach(d -> d.place(ctx));
    if (decorators.empty()) {
        return;
    }
    decorator::TreeDecoratorContext::DecorationSetter setter = [&region](const BlockPos& pos, const BlockState* state) {
        // MC getDecorationSetter: setBlock(pos, state, 19)。项目 flags 语义不同，用 3（通知+客户端）。
        region.setBlockState(pos, state, 3);
    };
    decorator::TreeDecoratorContext context(region, setter, random, logs, {}, {});
    for (const auto& decorator : decorators) {
        decorator->place(context);
    }
}

bool FallenTreeFeature::place(WorldGenRegion& region,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& origin,
    const FallenTreeConfig& config)
{
    // MC placeFallenTree：placeStump → 选水平方向 → setGroundHeight → canPlace → placeFallenLog。

    // 1. placeStump：在 origin 放 1 格原木（identity 修饰），跑 stumpDecorators。
    const BlockPos stump = placeLogBlock(region, config, random, origin, identityState);
    decorateLogs(region, random, {stump}, config.stumpDecorators);

    // 2. direction = 水平随机方向；i = logLength.sample - 2。
    const std::array<Direction, 4> horiz = Directions::horizontal();
    const Direction direction = horiz[static_cast<size_t>(random.nextInt(4))];
    const i32 logCount = config.logLength->sample(random) - 2;

    // 3. 倒木起点 = origin.relative(direction, 2 + nextInt(2))。
    BlockPosMutable cursor(origin.offset(direction, 2 + random.nextInt(2)));

    // setGroundHeightForFallenLogStartPos：上移 1，再最多下移 6 格找 mayPlaceOn。
    cursor.move(Direction::Up, 1);
    for (i32 step = 0; step < 6; ++step) {
        if (mayPlaceOn(region, cursor)) {
            break;
        }
        cursor.move(Direction::Down, 1);
    }

    // 4. canPlaceEntireFallenLog：沿 direction 遍历 logCount 格，累计非实地格数 >2 则放弃。
    i32 gaps = 0;
    bool canPlace = true;
    {
        BlockPosMutable scan(cursor);
        for (i32 j = 0; j < logCount; ++j) {
            if (!validTreePos(region, scan)) {
                canPlace = false;
                break;
            }
            if (!isOverSolidGround(region, scan)) {
                if (++gaps > 2) {
                    canPlace = false;
                    break;
                }
            } else {
                gaps = 0;
            }
            scan.move(direction, 1);
        }
    }

    if (!canPlace) {
        return true; // MC place() 始终 return true（已放树桩）。
    }

    // 5. placeFallenLog：沿 direction 放 logCount 格原木（axis 切水平），跑 logDecorators。
    std::vector<BlockPos> logs;
    if (logCount > 0) {
        logs.reserve(static_cast<size_t>(logCount));
    }
    {
        BlockPosMutable placeCursor(cursor);
        for (i32 j = 0; j < logCount; ++j) {
            const BlockPos placed =
                placeLogBlock(region, config, random, placeCursor, [direction](const BlockState* s) {
                    return applySidewaysAxis(s, direction);
                });
            logs.push_back(placed);
            placeCursor.move(direction, 1);
        }
    }
    decorateLogs(region, random, logs, config.logDecorators);

    return true;
}

// ============================================================================
// ConfiguredFallenTreeFeature
// ============================================================================

ConfiguredFallenTreeFeature::ConfiguredFallenTreeFeature(
    std::unique_ptr<FallenTreeConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredFallenTreeFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    // MC FallenTreeFeature.place 直接调 placeFallenTree 并 return true。
    return m_feature.place(region, generator, random, pos, *m_config);
}

} // namespace tree
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
