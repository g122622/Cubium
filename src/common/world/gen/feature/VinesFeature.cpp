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

#include "VinesFeature.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/BooleanProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"

namespace mc::world::gen::feature {

namespace {

/// MC 1.21.11 VineBlock.isAcceptableNeighbour = MultifaceBlock.canAttachTo =
/// state.isFaceSturdy(reader, pos, dir.getOpposite(), SupportType.CENTER)。
bool isAcceptableNeighbour(WorldGenRegion& world, const BlockPos& pos, Direction direction)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }
    return state->isFaceSturdy(world, pos, Directions::opposite(direction), SupportType::Center);
}

/// MC 1.21.11 VineBlock.getPropertyForFace(direction)：
/// UP/NORTH/SOUTH/EAST/WEST 对应的面属性；DOWN 返回 null。
const BooleanProperty* getPropertyForFace(Direction direction)
{
    switch (direction) {
        case Direction::Up:
            return &BlockStateProperties::UP();
        case Direction::North:
            return &BlockStateProperties::NORTH();
        case Direction::South:
            return &BlockStateProperties::SOUTH();
        case Direction::East:
            return &BlockStateProperties::EAST();
        case Direction::West:
            return &BlockStateProperties::WEST();
        default:
            return nullptr;
    }
}

} // namespace

bool ConfiguredVinesFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& /*random*/,
    const BlockPos& origin) const
{
    // MC: if (!worldgenlevel.isEmptyBlock(blockpos)) return false;
    const BlockState* current = region.getBlockState(origin);
    if (current != nullptr && !current->isAir()) {
        return false;
    }

    // MC: for (Direction direction : Direction.values()) if (direction != DOWN && isAcceptableNeighbour(...))
    // Direction.values() 顺序：DOWN, UP, NORTH, SOUTH, WEST, EAST。
    static constexpr Direction kAllDirections[] = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
    for (Direction direction : kAllDirections) {
        if (direction == Direction::Down) {
            continue;
        }
        const BlockPos neighbour = origin.offset(direction);
        if (isAcceptableNeighbour(region, neighbour, direction)) {
            const BlockState* vineDefault = &VanillaBlocks::VINE->defaultState();
            const BooleanProperty* face = getPropertyForFace(direction);
            const BlockState* placed = (face != nullptr) ? &vineDefault->with(*face, true) : vineDefault;
            region.setBlockState(origin, placed, 2);
            return true;
        }
    }
    return false;
}

} // namespace mc::world::gen::feature
