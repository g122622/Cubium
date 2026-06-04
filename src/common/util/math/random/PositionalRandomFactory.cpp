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

#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/util/crypto/Md5.hpp"
#include <cstring>

namespace mc::math {

// ============================================================================
// Stafford13 混合函数（与 MC RandomSupport.mixStafford13 一致）
// ============================================================================

static u64 mixStafford13(u64 z)
{
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

// ============================================================================
// MC Mth.getSeed — 坐标到种子的确定性转换
// ============================================================================

static i64 getSeed(i32 x, i32 y, i32 z)
{
    i64 i = static_cast<i64>(x) * 3129871L ^ static_cast<i64>(z) * 116129781L ^ static_cast<i64>(y);
    i = i * i * 42317861L + i * 11L;
    return i >> 16;
}

// ============================================================================
// PositionalRandomFactory 实现
// ============================================================================

PositionalRandomFactory::PositionalRandomFactory(u64 seedLo, u64 seedHi)
    : m_seedLo(seedLo)
    , m_seedHi(seedHi)
{}

std::unique_ptr<Xoroshiro128ppRandom> PositionalRandomFactory::fromHashOf(const std::string& key) const
{
    // MC 1.21: 使用 MD5 哈希字符串，将 128 位哈希与工厂种子 XOR
    // 然后经过 mixStafford13 混合
    const util::crypto::Md5::Digest digest = util::crypto::Md5::hash(key);

    // 从 MD5 结果中提取两个 64 位值（小端序）
    u64 hashLo = 0;
    u64 hashHi = 0;
    std::memcpy(&hashLo, digest.data(), 8);
    std::memcpy(&hashHi, digest.data() + 8, 8);

    // XOR 工厂种子并混合
    const u64 finalLo = mixStafford13(hashLo ^ m_seedLo);
    const u64 finalHi = mixStafford13(hashHi ^ m_seedHi);

    return std::make_unique<Xoroshiro128ppRandom>(finalLo, finalHi);
}

std::unique_ptr<Xoroshiro128ppRandom> PositionalRandomFactory::fromSeed(u64 seed) const
{
    // MC 1.21: seed XOR factory seedLo/seedHi
    return std::make_unique<Xoroshiro128ppRandom>(seed ^ m_seedLo, seed ^ m_seedHi);
}

std::unique_ptr<Xoroshiro128ppRandom> PositionalRandomFactory::at(i32 x, i32 y, i32 z) const
{
    // MC 1.21: Mth.getSeed(x, y, z) XOR factory seedLo
    const i64 posSeed = getSeed(x, y, z);
    return std::make_unique<Xoroshiro128ppRandom>(static_cast<u64>(posSeed) ^ m_seedLo, m_seedHi);
}

} // namespace mc::math
