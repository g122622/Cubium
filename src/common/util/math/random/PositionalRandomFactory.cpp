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
#include "common/core/Types.hpp"
#include "common/util/crypto/Md5.hpp"
#include "common/util/math/random/Xoroshiro128ppRandom.hpp"
#include <memory>
#include <string>

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
    // MC 1.21: XoroshiroPositionalRandomFactory.fromHashOf(String)
    // 使用 MD5 哈希字符串，将 128 位哈希与工厂种子 XOR
    // 然后经过 mixStafford13 混合后作为新 RNG 的种子
    //
    // Java 的 Longs.fromBytes() 使用大端序解释字节：
    //   long lo = Longs.fromBytes(bytes[0], bytes[1], ..., bytes[7])
    //   long hi = Longs.fromBytes(bytes[8], bytes[9], ..., bytes[15])
    // 其中 Longs.fromBytes(b0..b7) = b0<<56 | b1<<48 | ... | b7
    // 即 bytes[0] 是最高有效字节（大端序）
    const util::crypto::Md5::Digest digest = util::crypto::Md5::hash(key);

    // 大端序解释：第一个字节是最高有效字节
    u64 hashLo = (static_cast<u64>(digest[0]) << 56) | (static_cast<u64>(digest[1]) << 48) |
        (static_cast<u64>(digest[2]) << 40) | (static_cast<u64>(digest[3]) << 32) |
        (static_cast<u64>(digest[4]) << 24) | (static_cast<u64>(digest[5]) << 16) | (static_cast<u64>(digest[6]) << 8) |
        static_cast<u64>(digest[7]);
    u64 hashHi = (static_cast<u64>(digest[8]) << 56) | (static_cast<u64>(digest[9]) << 48) |
        (static_cast<u64>(digest[10]) << 40) | (static_cast<u64>(digest[11]) << 32) |
        (static_cast<u64>(digest[12]) << 24) | (static_cast<u64>(digest[13]) << 16) |
        (static_cast<u64>(digest[14]) << 8) | static_cast<u64>(digest[15]);

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
