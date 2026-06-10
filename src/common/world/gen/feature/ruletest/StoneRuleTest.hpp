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

#pragma once

#include "RuleTest.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace ruletest {

/**
 * @brief 匹配石头类方块的规则测试
 *
 * 匹配石头、花岗岩、闪长岩、安山岩。
 * 用于自然矿石生成。
 */
class StoneRuleTest : public RuleTest {
public:
    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "stone"; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override;
};

} // namespace ruletest
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
