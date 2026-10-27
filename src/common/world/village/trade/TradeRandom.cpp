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

#include "common/world/village/trade/TradeRandom.hpp"

namespace mc {
namespace world {
namespace village {
namespace trade {

TradeRandom::TradeRandom(u64 seed)
    : m_engine(seed)
{}

TradeRandom::TradeRandom(const TradeRandom& other)
    : m_engine(other.m_engine)
{}

TradeRandom& TradeRandom::operator=(const TradeRandom& other)
{
    if (this != &other) {
        m_engine = other.m_engine;
    }
    return *this;
}

void TradeRandom::setSeed(u64 seed)
{
    m_engine.setSeed(seed);
}

u64 TradeRandom::nextU64()
{
    return m_engine.nextU64();
}

math::PositionalRandomFactory TradeRandom::forkPositional()
{
    return math::PositionalRandomFactory(nextU64());
}

TradeRandom TradeRandom::nextChild()
{
    // 父流只推进一次，子流种子取推进后的值；同一父流连续派生互不干扰可复现
    TradeRandom child(*this);
    child.setSeed(m_engine.nextU64());
    return child;
}

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
