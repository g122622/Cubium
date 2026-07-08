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

#include "Column.hpp"

#include "common/util/Direction.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

std::optional<i32> Column::scanDirection(IWorld& world,
    i32 range,
    const StatePredicate& isEmptyPredicate,
    const StatePredicate& isStopPredicate,
    BlockPosMutable& cursor,
    i32 originY,
    Direction direction)
{
    cursor.setY(originY);

    for (i32 i = 1; i < range && isEmptyPredicate(world.getBlockState(cursor)); ++i) {
        cursor.move(direction);
    }

    return isStopPredicate(world.getBlockState(cursor)) ? std::optional<i32>(cursor.y) : std::nullopt;
}

std::optional<std::unique_ptr<Column>> Column::scan(
    IWorld& world, const BlockPos& origin, i32 range, StatePredicate isEmptyPredicate, StatePredicate isStopPredicate)
{
    BlockPosMutable cursor(origin);
    if (!isEmptyPredicate(world.getBlockState(origin))) {
        return std::nullopt;
    }

    const i32 originY = origin.y;
    const std::optional<i32> ceiling =
        scanDirection(world, range, isEmptyPredicate, isStopPredicate, cursor, originY, Direction::Up);
    const std::optional<i32> floor =
        scanDirection(world, range, isEmptyPredicate, isStopPredicate, cursor, originY, Direction::Down);
    return create(floor, ceiling);
}

std::unique_ptr<Column> Column::create(std::optional<i32> floor, std::optional<i32> ceiling)
{
    if (floor.has_value() && ceiling.has_value()) {
        return std::make_unique<Range>(*floor, *ceiling);
    }
    if (floor.has_value()) {
        // above(floor) = Ray(edge=floor, pointingUp=true)
        return std::make_unique<Ray>(*floor, true);
    }
    if (ceiling.has_value()) {
        // below(ceiling) = Ray(edge=ceiling, pointingUp=false)
        return std::make_unique<Ray>(*ceiling, false);
    }
    return std::make_unique<Line>();
}

std::unique_ptr<Column> Column::withFloor(std::optional<i32> floor) const
{
    return create(floor, getCeiling());
}

std::unique_ptr<Column> Column::withCeiling(std::optional<i32> ceiling) const
{
    return create(getFloor(), ceiling);
}

} // namespace mc
