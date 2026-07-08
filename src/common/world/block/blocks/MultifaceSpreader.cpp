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

#include "MultifaceSpreader.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"

namespace mc {
namespace blocks {

bool MultifaceSpreader::spreadFromFaceTowardRandomDirection(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction fromFace, math::IRandom& random) const
{
    // MC: Direction.allShuffled(random)，逐个尝试，命中即止。
    std::vector<Direction> dirs = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
    random.shuffle(dirs);
    for (Direction spreadDir : dirs) {
        if (spreadFromFaceTowardDirection(state, world, pos, fromFace, spreadDir)) {
            return true;
        }
    }
    return false;
}

bool MultifaceSpreader::spreadFromFaceTowardDirection(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction fromFace, Direction spreadDir) const
{
    // MC getSpreadFromFaceTowardDirection:
    //   1) 同轴不扩散
    //   2) hasFace(state, fromFace) && !hasFace(state, spreadDir) 才继续
    //   3) 逐 SpreadType 取 spreadPos，predicate 命中即放置
    if (Directions::getAxis(spreadDir) == Directions::getAxis(fromFace)) {
        return false;
    }
    if (!MultifaceBlock::hasFace(state, fromFace) || MultifaceBlock::hasFace(state, spreadDir)) {
        return false;
    }

    for (const SpreadPos& spread : candidateSpreadPos(pos, spreadDir, fromFace)) {
        if (!canSpreadInto(world, spread.pos, spread.face)) {
            continue;
        }
        // MC DefaultSpreaderConfig.canSpreadInto 额外要求 isValidStateForPlacement。
        const BlockState* targetState = world.getBlockState(spread.pos);
        if (targetState == nullptr) {
            continue;
        }
        if (!m_block.isValidStateForPlacement(world, *targetState, spread.pos, spread.face)) {
            continue;
        }
        spreadToFace(world, spread.pos, spread.face);
        return true;
    }
    return false;
}

std::vector<MultifaceSpreader::SpreadPos> MultifaceSpreader::candidateSpreadPos(
    const BlockPos& pos, Direction spreadDir, Direction fromFace) const
{
    // MC DEFAULT_SPREAD_ORDER = {SAME_POSITION, SAME_PLANE, WRAP_AROUND}
    //   SAME_POSITION: (pos, spreadDir)
    //   SAME_PLANE:    (pos.relative(spreadDir), fromFace)
    //   WRAP_AROUND:   (pos.relative(spreadDir).relative(fromFace), fromFace.getOpposite())
    std::vector<SpreadPos> result;
    result.reserve(3);
    result.push_back({pos, spreadDir});
    result.push_back({pos.offset(spreadDir), fromFace});
    result.push_back({pos.offset(spreadDir).offset(fromFace), Directions::opposite(fromFace)});
    return result;
}

bool MultifaceSpreader::canSpreadInto(IWorld& world, const BlockPos& targetPos, Direction /*face*/) const
{
    // MC DefaultSpreaderConfig.stateCanBeReplaced:
    //   state.isAir() || state.is(this.block) || (state.is(WATER) && state.getFluidState().isSource())
    const BlockState* state = world.getBlockState(targetPos);
    if (state == nullptr) {
        return true; // nullptr 视为空气
    }
    if (state->isAir()) {
        return true;
    }
    if (state->is(&m_block)) {
        return true;
    }
    const fluid::FluidState* fluid = state->getFluidState();
    if (fluid != nullptr && fluid->isSource() && &fluid->getFluid() == fluid::Fluids::WATER()) {
        return true;
    }
    return false;
}

void MultifaceSpreader::spreadToFace(IWorld& world, const BlockPos& targetPos, Direction face) const
{
    // MC spreadToFace: getStateForPlacement(targetState, reader, pos, face) → setBlock(pos, state, 2)。
    const BlockState* current = world.getBlockState(targetPos);
    const BlockState* placed = m_block.getStateForPlacement(current, world, targetPos, face);
    if (placed != nullptr) {
        world.setBlockState(targetPos, placed, 2);
    }
}

} // namespace blocks
} // namespace mc
