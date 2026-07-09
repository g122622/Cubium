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

#include "AttachToLogsDecorator.hpp"
#include "common/world/gen/feature/parser/BlockStateProviderParser.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace tree {
namespace decorator {

AttachToLogsDecorator::AttachToLogsDecorator(
    f32 probability, std::unique_ptr<parser::BlockStateProviderHandle> blockProvider, std::vector<Direction> directions)
    : m_probability(probability)
    , m_blockProvider(std::move(blockProvider))
    , m_directions(std::move(directions))
{}

void AttachToLogsDecorator::place(const TreeDecoratorContext& context) const
{
    // MC AttachedToLogsDecorator.place:
    //   for (BlockPos blockpos : Util.shuffledCopy(logs(), random)) {
    //       Direction direction = Util.getRandom(directions, random);
    //       BlockPos blockpos1 = blockpos.relative(direction);
    //       if (random.nextFloat() <= probability && isAir(blockpos1))
    //           setBlock(blockpos1, blockProvider.getState(random, blockpos1));
    //   }
    math::Random& random = context.random();
    std::vector<BlockPos> shuffled = context.logs();
    random.shuffle(shuffled);

    for (const BlockPos& log : shuffled) {
        const Direction direction =
            m_directions[static_cast<size_t>(random.nextInt(static_cast<i32>(m_directions.size())))];
        const BlockPos target = log.offset(direction);
        if (random.nextFloat() <= m_probability && context.isAir(target)) {
            const BlockState* state =
                parser::BlockStateProviderParser::sampleState(*m_blockProvider, context.region(), random, target);
            if (state != nullptr) {
                context.setBlock(target, state);
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
