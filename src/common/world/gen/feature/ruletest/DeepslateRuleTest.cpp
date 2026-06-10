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

#include "DeepslateRuleTest.hpp"
#include "common/world/block/registry/DeepslateBlocks.hpp"
#include "common/world/block/registry/TuffBlocks.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace ruletest {

bool DeepslateRuleTest::test(const BlockState& state, math::Random& /*random*/) const
{
    if (block_registry::DeepslateBlocks::DEEPSLATE && state.is(block_registry::DeepslateBlocks::DEEPSLATE)) {
        return true;
    }
    if (block_registry::TuffBlocks::TUFF && state.is(block_registry::TuffBlocks::TUFF)) {
        return true;
    }
    return false;
}

std::unique_ptr<RuleTest> DeepslateRuleTest::clone() const
{
    return std::make_unique<DeepslateRuleTest>();
}

} // namespace ruletest
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
