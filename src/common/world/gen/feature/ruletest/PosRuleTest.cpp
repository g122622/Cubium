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
 * copies of substantial portions of the Software.
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

#include "PosRuleTest.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace ruletest {

// ============================================================================
// LinearPosRuleTest
// ============================================================================

LinearPosRuleTest::LinearPosRuleTest(i32 minDistance, i32 maxDistance, f32 minProbability, f32 maxProbability)
    : m_minDistance(minDistance)
    , m_maxDistance(maxDistance)
    , m_minProbability(minProbability)
    , m_maxProbability(maxProbability)
{
    MC_ASSERT_RELEASE(minDistance <= maxDistance);
}

bool LinearPosRuleTest::test(
    const BlockPos& /*originalPos*/, const BlockPos& worldPos, const BlockPos& seedPos, math::Random& rng) const
{
    i32 distance = worldPos.manhattanDistance(seedPos);
    f32 randomValue = rng.nextFloat();

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
    MC_ASSERT_RELEASE(minDistance <= maxDistance);
}

bool AxisAlignedLinearPosTest::test(
    const BlockPos& /*originalPos*/, const BlockPos& worldPos, const BlockPos& seedPos, math::Random& rng) const
{
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

    f32 probability = math::mappedLerp(m_minProbability,
        m_maxProbability,
        static_cast<f32>(m_minDistance),
        static_cast<f32>(m_maxDistance),
        static_cast<f32>(distance));

    return randomValue <= probability;
}

} // namespace ruletest
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
