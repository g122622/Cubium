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
 */

#include "BlockPredicate.hpp"
#include "common/util/Direction.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"

namespace mc::world::gen::feature::predicate {

bool OnlyInAirPredicate::test(const IWorld& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    return state == nullptr || state->isAir();
}

bool SolidBlockPredicate::test(const IWorld& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    return state != nullptr && state->isSolid();
}

bool HasSturdyFacePredicate::test(const IWorld& world, const BlockPos& pos) const
{
    BlockPos checkPos = pos + m_offset;
    const BlockState* state = world.getBlockState(checkPos);
    if (state == nullptr) {
        return false;
    }
    // hasSturdyFace 检查方块自身某面是否坚固
    // isSolidSide 检查的是从 side 方向看该面是否坚固，因此需要取反方向
    return state->isSolidSide(const_cast<IWorld&>(world), checkPos, Directions::opposite(m_direction));
}

bool MatchingBlockPredicate::test(const IWorld& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }
    return &state->getBlock() == m_block;
}

bool TagMatchPredicate::test(const IWorld& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }
    auto* tag = mc::BlockTags::getTag(mc::ResourceLocation(m_tagName));
    return tag != nullptr && tag->contains(*state);
}

bool EnvironmentScanPredicate::scan(const IWorld& world, BlockPos& startPos) const
{
    BlockPos current = startPos;
    for (i32 i = 0; i < m_maxSteps; ++i) {
        // 先检查终止条件
        if (m_abortCondition && m_abortCondition->test(world, current)) {
            return false;
        }
        // 检查目标条件
        if (m_targetCondition->test(world, current)) {
            startPos = current;
            return true;
        }
        // 沿方向前进一步
        current = current.offset(m_direction);
    }
    return false;
}

// ============================================================================
// AllOfPredicate
// ============================================================================

bool AllOfPredicate::test(const IWorld& world, const BlockPos& pos) const
{
    for (const auto& pred : m_predicates) {
        if (!pred->test(world, pos)) {
            return false;
        }
    }
    return true;
}

std::unique_ptr<BlockPredicate> AllOfPredicate::clone() const
{
    std::vector<std::unique_ptr<BlockPredicate>> cloned;
    cloned.reserve(m_predicates.size());
    for (const auto& pred : m_predicates) {
        cloned.push_back(pred->clone());
    }
    return std::make_unique<AllOfPredicate>(std::move(cloned));
}

// ============================================================================
// AnyOfPredicate
// ============================================================================

bool AnyOfPredicate::test(const IWorld& world, const BlockPos& pos) const
{
    for (const auto& pred : m_predicates) {
        if (pred->test(world, pos)) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<BlockPredicate> AnyOfPredicate::clone() const
{
    std::vector<std::unique_ptr<BlockPredicate>> cloned;
    cloned.reserve(m_predicates.size());
    for (const auto& pred : m_predicates) {
        cloned.push_back(pred->clone());
    }
    return std::make_unique<AnyOfPredicate>(std::move(cloned));
}

// ============================================================================
// NotPredicate
// ============================================================================

bool NotPredicate::test(const IWorld& world, const BlockPos& pos) const
{
    return !m_predicate->test(world, pos);
}

std::unique_ptr<BlockPredicate> NotPredicate::clone() const
{
    return std::make_unique<NotPredicate>(m_predicate->clone());
}

// ============================================================================
// ReplaceablePredicate
// ============================================================================

bool ReplaceablePredicate::test(const IWorld& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return true;
    }
    // TODO: BlockState 缺少 canBeReplaced / canBeReplacedByFluid 方法，
    // 暂时使用 isAir() || getMaterial().isReplaceable() 作为近似实现
    return state->isAir() || state->getMaterial().isReplaceable();
}

// ============================================================================
// WouldSurvivePredicate
// ============================================================================

bool WouldSurvivePredicate::test(const IWorld& world, const BlockPos& pos) const
{
    if (m_state == nullptr) {
        return false;
    }
    BlockPos checkPos = pos + m_offset;
    // isValidPosition 接收 IBlockReader&（继承自 IWorld），需要 const_cast + static_cast
    auto& blockReader = static_cast<IBlockReader&>(const_cast<IWorld&>(world));
    return m_state->getBlock().isValidPosition(*m_state, blockReader, checkPos);
}

// ============================================================================
// InsideWorldBoundsPredicate
// ============================================================================

bool InsideWorldBoundsPredicate::test(const IWorld& /*world*/, const BlockPos& pos) const
{
    return pos.y >= mc::world::MIN_BUILD_HEIGHT && pos.y < mc::world::MAX_BUILD_HEIGHT;
}

// ============================================================================
// OnlyInAirOrWaterPredicate
// ============================================================================

bool OnlyInAirOrWaterPredicate::test(const IWorld& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    return state == nullptr || state->isAir() || state->isLiquid();
}

} // namespace mc::world::gen::feature::predicate
