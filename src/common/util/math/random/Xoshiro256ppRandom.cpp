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

#include "Xoshiro256ppRandom.hpp"
#include "common/core/Types.hpp"
#include <cstddef>

namespace mc::math {

Xoshiro256ppRandom::Xoshiro256ppRandom(u64 seed)
{
    setSeed(seed);
}

void Xoshiro256ppRandom::setSeed(u64 seed)
{
    // 使用 SplitMix64 扩展种子
    u64 state = seed;
    m_state[0] = splitMix64(state);
    m_state[1] = splitMix64(state);
    m_state[2] = splitMix64(state);
    m_state[3] = splitMix64(state);

    // 确保状态不全为零
    if (m_state[0] == 0 && m_state[1] == 0 && m_state[2] == 0 && m_state[3] == 0) {
        m_state[0] = 1;
    }

    m_hasGaussian = false;
}

u64 Xoshiro256ppRandom::nextU64()
{
    // xoshiro256++ 核心算法
    const u64 result = rotl(m_state[0] + m_state[3], 23) + m_state[0];

    const u64 t = m_state[1] << 17;

    m_state[2] ^= m_state[0];
    m_state[3] ^= m_state[1];
    m_state[1] ^= m_state[2];
    m_state[0] ^= m_state[3];

    m_state[2] ^= t;
    m_state[3] = rotl(m_state[3], 45);

    return result;
}

void Xoshiro256ppRandom::skip(u64 /* count */)
{
    // xoshiro256++ 的快速跳转
    // 使用预计算的跳转多项式
    // 参考 http://xoroshiro.di.unimi.it/xoshiro256plusplus.c
    // 注意：参数 count 被忽略，因为快速跳转每次跳过 2^128 个状态
    // 这是 xoroshiro/xoshiro 系列算法的标准实现方式

    static const u64 JUMP[] = {
        0x180ec6d33cfd0abaULL, 0xd5a61266f0c9392cULL, 0xa9582618e03fc9aaULL, 0x39abdc4529b1661cULL};

    u64 s0 = 0;
    u64 s1 = 0;
    u64 s2 = 0;
    u64 s3 = 0;

    for (size_t i = 0; i < sizeof(JUMP) / sizeof(JUMP[0]); ++i) {
        for (int b = 0; b < 64; ++b) {
            if (JUMP[i] & (1ULL << b)) {
                s0 ^= m_state[0];
                s1 ^= m_state[1];
                s2 ^= m_state[2];
                s3 ^= m_state[3];
            }
            (void)nextU64(); // 故意丢弃返回值，用于状态更新
        }
    }

    m_state[0] = s0;
    m_state[1] = s1;
    m_state[2] = s2;
    m_state[3] = s3;
}

u64 Xoshiro256ppRandom::splitMix64(u64& state)
{
    state += 0x9e3779b97f4a7c15ULL;
    u64 z = state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

} // namespace mc::math
