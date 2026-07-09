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
#include "common/util/Direction.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"

namespace mc {
namespace blocks {

const std::vector<MultifaceSpreadType>& defaultSpreadOrder()
{
    static const std::vector<MultifaceSpreadType> order = {
        MultifaceSpreadType::SamePosition, MultifaceSpreadType::SamePlane, MultifaceSpreadType::WrapAround};
    return order;
}

// ============================================================================
// MultifaceSpreadConfig
// ============================================================================

bool MultifaceSpreadConfig::placeBlock(
    IWorld& world, const MultifaceSpreadPos& spreadPos, const BlockState* current, bool worldGen) const
{
    // MC SpreadConfig.placeBlock: getStateForPlacement → setBlock(pos, state, 2)；
    //   worldGen 时 markPosForPostprocessing（项目暂未实现该标记，此处仅放置）。
    const BlockState* placed = getStateForPlacement(current, world, spreadPos.pos, spreadPos.face);
    if (placed == nullptr) {
        return false;
    }
    MC_UNUSED(worldGen);
    return world.setBlockState(spreadPos.pos, placed, 2);
}

// ============================================================================
// MultifaceSpreader
// ============================================================================

MultifaceSpreadPos MultifaceSpreader::getSpreadPos(
    const BlockPos& pos, Direction spreadDir, Direction fromFace, MultifaceSpreadType type)
{
    // MC SpreadType.getSpreadPos(原位置, 目标方向 spreadDir, 源面 fromFace)：
    //   SAME_POSITION: (pos, spreadDir)
    //   SAME_PLANE:    (pos.relative(spreadDir), fromFace)
    //   WRAP_AROUND:   (pos.relative(spreadDir).relative(fromFace), fromFace.getOpposite())
    switch (type) {
        case MultifaceSpreadType::SamePosition:
            return {pos, spreadDir};
        case MultifaceSpreadType::SamePlane:
            return {pos.offset(spreadDir), fromFace};
        case MultifaceSpreadType::WrapAround:
            return {pos.offset(spreadDir).offset(fromFace), Directions::opposite(fromFace)};
    }
    return {pos, spreadDir};
}

std::optional<MultifaceSpreadPos> MultifaceSpreader::getSpreadFromFaceTowardDirection(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction fromFace, Direction spreadDir) const
{
    // MC getSpreadFromFaceTowardDirection:
    //   1) 同轴不扩散
    //   2) isOtherBlockValidAsSource || (hasFace(fromFace) && !hasFace(spreadDir))
    //   3) 按 getSpreadTypes() 顺序找第一个 canSpreadInto 的 SpreadPos
    if (Directions::getAxis(spreadDir) == Directions::getAxis(fromFace)) {
        return std::nullopt;
    }
    if (!m_config->isOtherBlockValidAsSource(state) &&
        !(m_config->hasFace(state, fromFace) && !m_config->hasFace(state, spreadDir))) {
        return std::nullopt;
    }

    for (MultifaceSpreadType type : m_config->getSpreadTypes()) {
        MultifaceSpreadPos spreadPos = getSpreadPos(pos, spreadDir, fromFace, type);
        if (m_config->canSpreadInto(world, pos, spreadPos)) {
            return spreadPos;
        }
    }
    return std::nullopt;
}

std::optional<MultifaceSpreadPos> MultifaceSpreader::spreadToFace(
    IWorld& world, const MultifaceSpreadPos& spreadPos, bool worldGen) const
{
    // MC spreadToFace: 取目标格当前 state（nullptr=空气），placeBlock 成功则返回 SpreadPos。
    const BlockState* current = world.getBlockState(spreadPos.pos);
    if (m_config->placeBlock(world, spreadPos, current, worldGen)) {
        return spreadPos;
    }
    return std::nullopt;
}

std::optional<MultifaceSpreadPos> MultifaceSpreader::spreadFromFaceTowardRandomDirection(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction fromFace,
    math::IRandom& random,
    bool worldGen) const
{
    // MC: Direction.allShuffled(random) 逐个尝试 spreadFromFaceTowardDirection，首个成功即返回。
    std::vector<Direction> dirs = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
    random.shuffle(dirs);
    for (Direction spreadDir : dirs) {
        auto spreadPos = getSpreadFromFaceTowardDirection(state, world, pos, fromFace, spreadDir);
        if (spreadPos.has_value()) {
            auto placed = spreadToFace(world, spreadPos.value(), worldGen);
            if (placed.has_value()) {
                return placed;
            }
        }
    }
    return std::nullopt;
}

bool MultifaceSpreader::spreadFromFaceTowardRandomDirection(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction fromFace, math::IRandom& random) const
{
    // 5 参便捷版：worldGen=true，仅返回是否成功。
    return spreadFromFaceTowardRandomDirection(state, world, pos, fromFace, random, true).has_value();
}

i64 MultifaceSpreader::spreadFromFaceTowardAllDirections(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction fromFace, bool worldGen) const
{
    // MC spreadFromFaceTowardAllDirections: 对所有方向尝试 spreadFromFaceTowardDirection，计数成功。
    i64 count = 0;
    for (Direction spreadDir : Directions::all()) {
        auto spreadPos = getSpreadFromFaceTowardDirection(state, world, pos, fromFace, spreadDir);
        if (spreadPos.has_value() && spreadToFace(world, spreadPos.value(), worldGen).has_value()) {
            ++count;
        }
    }
    return count;
}

i64 MultifaceSpreader::spreadAll(const BlockState& state, IWorld& world, const BlockPos& pos, bool worldGen) const
{
    // MC spreadAll: Direction.stream().filter(canSpreadFrom).map(spreadFromFaceTowardAllDirections).sum
    i64 total = 0;
    for (Direction fromFace : Directions::all()) {
        if (m_config->canSpreadFrom(state, fromFace)) {
            total += spreadFromFaceTowardAllDirections(state, world, pos, fromFace, worldGen);
        }
    }
    return total;
}

bool MultifaceSpreader::canSpreadInAnyDirection(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction fromFace) const
{
    // MC canSpreadInAnyDirection: 任一方向 getSpreadFromFaceTowardDirection 命中。
    for (Direction spreadDir : Directions::all()) {
        if (getSpreadFromFaceTowardDirection(state, world, pos, fromFace, spreadDir).has_value()) {
            return true;
        }
    }
    return false;
}

} // namespace blocks
} // namespace mc
