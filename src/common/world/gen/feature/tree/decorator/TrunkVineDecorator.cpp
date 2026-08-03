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

#include "TrunkVineDecorator.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/tree/decorator/TreeDecorator.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace tree {
namespace decorator {

void TrunkVineDecorator::place(const TreeDecoratorContext& context) const
{
    // MC TrunkVineDecorator.place：对每根原木，west/east/north/south 各以
    // nextInt(3) > 0（2/3 概率）放置藤蔓。藤蔓朝向属性 = 该方向相反面
    // （west 邻居放 EAST=true 的藤蔓，表示藤蔓贴在原木西面/邻居东面）。
    math::Random& random = context.random();
    for (const BlockPos& log : context.logs()) {
        if (random.nextInt(3) > 0) {
            const BlockPos west = log.west();
            if (context.isAir(west)) {
                context.placeVine(west, BlockStateProperties::EAST());
            }
        }
        if (random.nextInt(3) > 0) {
            const BlockPos east = log.east();
            if (context.isAir(east)) {
                context.placeVine(east, BlockStateProperties::WEST());
            }
        }
        if (random.nextInt(3) > 0) {
            const BlockPos north = log.north();
            if (context.isAir(north)) {
                context.placeVine(north, BlockStateProperties::SOUTH());
            }
        }
        if (random.nextInt(3) > 0) {
            const BlockPos south = log.south();
            if (context.isAir(south)) {
                context.placeVine(south, BlockStateProperties::NORTH());
            }
        }
    }
}

} // namespace decorator
} // namespace tree
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
