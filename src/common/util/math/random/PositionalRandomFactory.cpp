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
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/Xoroshiro128ppRandom.hpp"
#include <memory>
#include <string>

namespace mc::math {

// ============================================================================
// MC Mth.getSeed — 坐标到种子的确定性转换
// ============================================================================

static i64 getSeed(i32 x, i32 y, i32 z)
{
    // Java 源码（Mth.getSeed）：
    //   long l = (long)(x * 3129871) ^ (long) z * 116129781L ^ (long) y;
    //   l = l * l * 42317861L + l * 11L;
    //   return l >> 16;
    // 两处整型回绕都必须逐位复刻，而 C++ 的有符号溢出是 UB，故全程用无符号运算显式回绕：
    // - `x * 3129871` 是 Java 的 **int** 乘法，先在 int32 内回绕，再符号扩展为 long。
    //   若按 64 位乘法计算，|x| >= 687 时结果即与原版发散（正常世界坐标几乎全部越界）。
    // - `l * l * 42317861L + l * 11L` 是 Java 的 long 回绕运算。
    // - 末尾 `>> 16` 是 Java 的算术右移。
    const i32 xTerm = static_cast<i32>(static_cast<u32>(x) * 3129871u);
    u64 i = static_cast<u64>(xTerm) ^ static_cast<u64>(static_cast<i64>(z)) * 116129781ULL ^
        static_cast<u64>(static_cast<i64>(y));
    i = i * i * 42317861ULL + i * 11ULL;
    return static_cast<i64>(i) >> 16;
}

// ============================================================================
// PositionalRandomFactory 实现
// ============================================================================

PositionalRandomFactory::PositionalRandomFactory(u64 seedLo, u64 seedHi)
    : m_flavor(Flavor::Xoroshiro)
    , m_seedLo(seedLo)
    , m_seedHi(seedHi)
{}

PositionalRandomFactory::PositionalRandomFactory(u64 seed)
    : m_flavor(Flavor::Legacy)
    , m_seedLo(seed)
    , m_seedHi(0)
{}

std::unique_ptr<IRandom> PositionalRandomFactory::fromHashOf(const std::string& key) const
{
    // Java String.hashCode()： s[0]*31^(n-1) + s[1]*31^(n-2) + ... + s[n-1]，int 回绕
    // Legacy flavor 需要它，Xoroshiro flavor 不需要（走 MD5）。
    if (m_flavor == Flavor::Legacy) {
        i32 hash = 0;
        for (const char ch : key) {
            hash = static_cast<i32>(static_cast<u32>(hash) * 31u + static_cast<u32>(static_cast<i8>(ch)));
        }
        // Java: new LegacyRandomSource(i ^ this.seed)，i 是 int 并按符号扩展参与 long 异或
        return std::make_unique<JavaLegacyRandom>(
            static_cast<u64>(static_cast<i64>(hash) ^ static_cast<i64>(m_seedLo)));
    }

    // MC 1.21: XoroshiroPositionalRandomFactory.fromHashOf(String)
    //   RandomSupport.Seed128bit s = RandomSupport.seedFromHashOf(s);
    //   return new XoroshiroRandomSource(s.xor(this.seedLo, this.seedHi));
    // seedFromHashOf 返回**未经混合**的 MD5 高/低 64 位，xor 只做逐位异或，
    // 两参构造 Xoroshiro128PlusPlus(lo, hi) 直接写状态——全程没有 mixStafford13。
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

    return std::make_unique<Xoroshiro128ppRandom>(hashLo ^ m_seedLo, hashHi ^ m_seedHi);
}

std::unique_ptr<IRandom> PositionalRandomFactory::fromSeed(u64 seed) const
{
    if (m_flavor == Flavor::Legacy) {
        // Java: LegacyPositionalRandomFactory.fromSeed(long) → new LegacyRandomSource(seed)
        // 【注意】这里**不与工厂种子异或**，与 Xoroshiro flavor 的行为不同。
        return std::make_unique<JavaLegacyRandom>(seed);
    }
    // MC 1.21: seed XOR factory seedLo/seedHi
    return std::make_unique<Xoroshiro128ppRandom>(seed ^ m_seedLo, seed ^ m_seedHi);
}

std::unique_ptr<IRandom> PositionalRandomFactory::at(i32 x, i32 y, i32 z) const
{
    const i64 posSeed = getSeed(x, y, z);
    if (m_flavor == Flavor::Legacy) {
        // Java: LegacyPositionalRandomFactory.at → new LegacyRandomSource(Mth.getSeed(x,y,z) ^ this.seed)
        return std::make_unique<JavaLegacyRandom>(static_cast<u64>(posSeed) ^ m_seedLo);
    }
    // MC 1.21: Mth.getSeed(x, y, z) XOR factory seedLo，seedHi 原样保留
    return std::make_unique<Xoroshiro128ppRandom>(static_cast<u64>(posSeed) ^ m_seedLo, m_seedHi);
}

} // namespace mc::math
