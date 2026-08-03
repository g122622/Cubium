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

#include "SlimeChunkChecker.hpp"
#include "common/core/Types.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc::world::spawn {

u64 SlimeChunkChecker::javaSetSeed(u64 seed)
{
    // Java: this.seed.set(seed ^ 0x5DEECE66DL) & ((1L << 48) - 1)
    return (seed ^ JAVA_LCG_MULTIPLIER) & JAVA_LCG_MASK;
}

i32 SlimeChunkChecker::javaNext(u64& state, i32 bits)
{
    // Java: state = (state * multiplier + increment) & mask
    state = (state * JAVA_LCG_MULTIPLIER + JAVA_LCG_INCREMENT) & JAVA_LCG_MASK;
    // Java: return (int)(state >>> (48 - bits))
    return static_cast<i32>(state >> (48 - bits));
}

i32 SlimeChunkChecker::javaNextInt(u64& state, i32 bound)
{
    MC_ASSERT_RELEASE(bound > 0);

    // 2 的幂次：直接用高位乘法
    if ((bound & (bound - 1)) == 0) {
        return static_cast<i32>((static_cast<u64>(bound) * static_cast<u32>(javaNext(state, 31))) >> 31);
    }

    // 非幂次：拒绝采样法保证均匀分布
    i32 val;
    i32 remainder;
    do {
        val = javaNext(state, 31);
        remainder = val % bound;
    } while (val - remainder + (bound - 1) < 0);

    return remainder;
}

u64 SlimeChunkChecker::computeSlimeChunkSeed(u64 worldSeed, i32 chunkX, i32 chunkZ)
{
    // MC Java 版 WorldgenRandom.seedSlimeChunk 公式:
    // seed = worldSeed + chunkX^2 * 4987142 + chunkX * 5947611
    //      + chunkZ^2 * 4392871L + chunkZ * 389711
    // 注意: Java 运算符优先级中 + 优先于 ^，所以先做所有加法再异或
    const i64 cx = static_cast<i64>(chunkX);
    const i64 cz = static_cast<i64>(chunkZ);

    const u64 seed = static_cast<u64>(static_cast<i64>(worldSeed) + cx * cx * CHUNK_X_SQUARED_COEFF +
        cx * CHUNK_X_COEFF + cz * cz * CHUNK_Z_SQUARED_COEFF + cz * CHUNK_Z_COEFF);

    return seed ^ SLIME_CHUNK_SALT;
}

bool SlimeChunkChecker::isSlimeChunk(u64 worldSeed, i32 chunkX, i32 chunkZ)
{
    const u64 chunkSeed = computeSlimeChunkSeed(worldSeed, chunkX, chunkZ);
    u64 state = javaSetSeed(chunkSeed);
    return javaNextInt(state, 10) == 0;
}

f32 SlimeChunkChecker::getSurfaceSlimeSpawnChance(i32 moonPhase)
{
    const i32 index = moonPhase % 8;
    return MOON_BRIGHTNESS_PER_PHASE[index] * 0.5f;
}

} // namespace mc::world::spawn
