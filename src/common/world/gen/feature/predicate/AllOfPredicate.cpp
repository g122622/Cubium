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

#include "AllOfPredicate.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc::world::gen::feature::predicate {

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

} // namespace mc::world::gen::feature::predicate
