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

#include "RuleTest.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/math/MathUtils.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

// ============================================================================
// LinearPosRuleTest
// ============================================================================

LinearPosRuleTest::LinearPosRuleTest(i32 minDistance, i32 maxDistance, f32 minProbability, f32 maxProbability)
    : m_minDistance(minDistance)
    , m_maxDistance(maxDistance)
    , m_minProbability(minProbability)
    , m_maxProbability(maxProbability)
{
    // 当 minDistance == maxDistance 时，概率固定为 minProbability
    // 仅当 minDistance > maxDistance 时才是无效参数
    MC_ASSERT_RELEASE(minDistance <= maxDistance);
}

bool LinearPosRuleTest::test(
    const BlockPos& /*originalPos*/, const BlockPos& worldPos, const BlockPos& seedPos, math::Random& rng) const
{
    // 使用曼哈顿距离（Manhattan distance）
    i32 distance = worldPos.manhattanDistance(seedPos);
    f32 randomValue = rng.nextFloat();

    // 使用 mappedLerp 进行线性插值
    f32 probability = math::mappedLerp(m_minProbability,
        m_maxProbability,
        static_cast<f32>(m_minDistance),
        static_cast<f32>(m_maxDistance),
        static_cast<f32>(distance));

    return randomValue <= probability;
}

// ============================================================================
// AxisAlignedLinearPosTest
// ============================================================================

AxisAlignedLinearPosTest::AxisAlignedLinearPosTest(
    f32 minProbability, f32 maxProbability, i32 minDistance, i32 maxDistance, Axis axis)
    : m_minProbability(minProbability)
    , m_maxProbability(maxProbability)
    , m_minDistance(minDistance)
    , m_maxDistance(maxDistance)
    , m_axis(axis)
{
    // 当 minDistance == maxDistance 时，概率固定为 minProbability
    // 仅当 minDistance > maxDistance 时才是无效参数
    MC_ASSERT_RELEASE(minDistance <= maxDistance);
}

bool AxisAlignedLinearPosTest::test(
    const BlockPos& /*originalPos*/, const BlockPos& worldPos, const BlockPos& seedPos, math::Random& rng) const
{
    // 计算指定轴方向上的距离
    i32 distance = 0;
    switch (m_axis) {
        case Axis::X:
            distance = std::abs(worldPos.x - seedPos.x);
            break;
        case Axis::Y:
            distance = std::abs(worldPos.y - seedPos.y);
            break;
        case Axis::Z:
            distance = std::abs(worldPos.z - seedPos.z);
            break;
        default:
            distance = 0;
            break;
    }

    f32 randomValue = rng.nextFloat();

    // 使用 mappedLerp 进行线性插值
    f32 probability = math::mappedLerp(m_minProbability,
        m_maxProbability,
        static_cast<f32>(m_minDistance),
        static_cast<f32>(m_maxDistance),
        static_cast<f32>(distance));

    return randomValue <= probability;
}

// ============================================================================
// RuleEntry
// ============================================================================

RuleEntry::RuleEntry(std::unique_ptr<RuleTest> inputPredicate,
    std::unique_ptr<RuleTest> locationPredicate,
    std::unique_ptr<PosRuleTest> posPredicate,
    u32 outputStateId,
    std::optional<nbt::tags::compound_tag> outputNbt)
    : m_inputPredicate(std::move(inputPredicate))
    , m_locationPredicate(std::move(locationPredicate))
    , m_posPredicate(std::move(posPredicate))
    , m_outputStateId(outputStateId)
    , m_outputNbt(std::move(outputNbt))
{}

RuleEntry::RuleEntry(
    std::unique_ptr<RuleTest> inputPredicate, std::unique_ptr<RuleTest> locationPredicate, u32 outputStateId)
    : RuleEntry(std::move(inputPredicate),
          std::move(locationPredicate),
          std::make_unique<AlwaysTruePosRuleTest>(),
          outputStateId,
          std::nullopt)
{}

bool RuleEntry::matches(const BlockState* inputState,
    const BlockState* locationState,
    const BlockPos& originalPos,
    const BlockPos& worldPos,
    const BlockPos& seedPos,
    math::Random& rng) const
{
    // 三个条件必须全部满足。
    // mc::RuleTest 为引用风格，方块状态为空时该谓词视为不匹配（结构处理器生产路径
    // 方块状态均来自已注册方块，不会为空）。
    if (m_inputPredicate && (inputState == nullptr || !m_inputPredicate->test(*inputState, rng))) {
        return false;
    }
    if (m_locationPredicate && (locationState == nullptr || !m_locationPredicate->test(*locationState, rng))) {
        return false;
    }
    if (m_posPredicate && !m_posPredicate->test(originalPos, worldPos, seedPos, rng)) {
        return false;
    }
    return true;
}

std::unique_ptr<RuleEntry> RuleEntry::clone() const
{
    auto clonedInput = m_inputPredicate ? m_inputPredicate->clone() : nullptr;
    auto clonedLocation = m_locationPredicate ? m_locationPredicate->clone() : nullptr;
    auto clonedPos = m_posPredicate ? m_posPredicate->clone() : nullptr;
    return std::make_unique<RuleEntry>(
        std::move(clonedInput), std::move(clonedLocation), std::move(clonedPos), m_outputStateId, m_outputNbt);
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
