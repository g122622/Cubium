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

#include "Xoroshiro128ppRandom.hpp"
#include "PositionalRandomFactory.hpp"

namespace mc::math {

Xoroshiro128ppRandom::Xoroshiro128ppRandom(u64 seed)
{
    setSeed(seed);
}

Xoroshiro128ppRandom::Xoroshiro128ppRandom(u64 seedLo, u64 seedHi)
{
    m_state[0] = seedLo;
    m_state[1] = seedHi;

    // 与 MC Xoroshiro128PlusPlus 一致：全零状态时使用默认值
    if ((m_state[0] | m_state[1]) == 0ULL) {
        m_state[0] = 0x9e3779b97f4a7c15ULL; // GOLDEN_RATIO_64
        m_state[1] = 0x6a09e667f3bcc909ULL;
    }

    m_hasGaussian = false;
}

void Xoroshiro128ppRandom::setSeed(u64 seed)
{
    // 使用 SplitMix64 扩展种子
    u64 state = seed;
    m_state[0] = splitMix64(state);
    m_state[1] = splitMix64(state);

    // 确保状态不全为零
    if (m_state[0] == 0 && m_state[1] == 0) {
        m_state[0] = 1;
    }

    m_hasGaussian = false;
}

u64 Xoroshiro128ppRandom::nextU64()
{
    const u64 s0 = m_state[0];
    u64 s1 = m_state[1];

    // xoroshiro128++ 核心算法
    const u64 result = rotl(s0 + s1, 17) + s0;

    // 更新状态
    s1 ^= s0;
    m_state[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21);
    m_state[1] = rotl(s1, 28);

    return result;
}

void Xoroshiro128ppRandom::skip(u64 /* count */)
{
    // xoroshiro128++ 的快速跳转
    // 使用预计算的跳转多项式
    // 参考 http://xoroshiro.di.unimi.it/xoroshiro128plusplus.c
    // 注意：参数 count 被忽略，因为快速跳转每次跳过 2^64 个状态
    // 这是 xoroshiro/xoshiro 系列算法的标准实现方式

    static const u64 JUMP[] = {0x2bd7a6a6e99c2ddcULL, 0x0992ccaf6a6fca05ULL};

    u64 s0 = 0;
    u64 s1 = 0;

    for (size_t i = 0; i < sizeof(JUMP) / sizeof(JUMP[0]); ++i) {
        for (int b = 0; b < 64; ++b) {
            if (JUMP[i] & (1ULL << b)) {
                s0 ^= m_state[0];
                s1 ^= m_state[1];
            }
            (void)nextU64(); // 故意丢弃返回值，用于状态更新
        }
    }

    m_state[0] = s0;
    m_state[1] = s1;
}

u64 Xoroshiro128ppRandom::splitMix64(u64& state)
{
    state += 0x9e3779b97f4a7c15ULL;
    u64 z = state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

PositionalRandomFactory Xoroshiro128ppRandom::forkPositional()
{
    // MC 1.21: 消耗两个 nextLong() 获取 128 位种子创建工厂
    const u64 seedLo = static_cast<u64>(nextLong());
    const u64 seedHi = static_cast<u64>(nextLong());
    return PositionalRandomFactory(seedLo, seedHi);
}

} // namespace mc::math
