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

#include "RuleEntry.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace ruletest {

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

bool RuleEntry::matches(const BlockState& inputState,
    const BlockState& locationState,
    const BlockPos& originalPos,
    const BlockPos& worldPos,
    const BlockPos& seedPos,
    math::Random& rng) const
{
    if (m_inputPredicate && !m_inputPredicate->test(inputState, rng)) {
        return false;
    }
    if (m_locationPredicate && !m_locationPredicate->test(locationState, rng)) {
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

} // namespace ruletest
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
