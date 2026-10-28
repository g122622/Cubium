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
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include <bit>

namespace mc::world::spawn {

u64 SlimeChunkChecker::computeSlimeChunkSeed(u64 worldSeed, i32 chunkX, i32 chunkZ)
{
    const auto x = static_cast<u32>(chunkX);
    const auto z = static_cast<u32>(chunkZ);
    const auto widen = [](u32 value) {
        return static_cast<u64>(static_cast<i64>(std::bit_cast<i32>(value)));
    };
    // 三项在提升为 long 前先按 int 回绕，z² 同样先以 int 求值。
    return (worldSeed + widen(x * x * 4987142U) + widen(x * 5947611U) + widen(z * z) * 4392871ULL +
               widen(z * 389711U)) ^
        987234911ULL;
}

bool SlimeChunkChecker::isSlimeChunk(u64 worldSeed, i32 chunkX, i32 chunkZ)
{
    math::JavaLegacyRandom random(computeSlimeChunkSeed(worldSeed, chunkX, chunkZ));
    return random.nextInt(10) == 0;
}

} // namespace mc::world::spawn
