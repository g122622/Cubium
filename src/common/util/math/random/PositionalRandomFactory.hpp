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

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/Xoroshiro128ppRandom.hpp"
#include <memory>
#include <string>

namespace mc::math {

/**
 * @brief MC 1.18+ 位置随机工厂
 *
 * 对应 MC Java 版的 PositionalRandomFactory（XoroshiroPositionalRandomFactory）。
 * 通过确定性方式从位置或字符串哈希派生子随机数生成器。
 *
 * 核心用途：
 * - PerlinNoise 使用 fromHashOf("octave_N") 为每个倍频创建独立随机源
 * - 区域化随机种子生成（如装饰物放置）
 *
 * 种子派生逻辑与 MC 1.21 XoroshiroPositionalRandomFactory 完全一致：
 * - fromHashOf: MD5(string) → 128-bit seed XOR factory seed
 * - fromSeed: seed XOR factory seedLo/seedHi
 * - at(x, y, z): Mth.getSeed(x, y, z) XOR factory seedLo
 */
class PositionalRandomFactory {
public:
    /**
     * @brief 构造位置随机工厂
     * @param seedLo 工厂种子低 64 位
     * @param seedHi 工厂种子高 64 位
     */
    PositionalRandomFactory(u64 seedLo, u64 seedHi);

    /**
     * @brief 从字符串哈希创建随机数生成器
     *
     * 使用 MD5 哈希字符串，将 128 位哈希结果与工厂种子 XOR，
     * 然后经过 mixStafford13 混合后作为新 RNG 的种子。
     *
     * 与 MC 1.21 XoroshiroPositionalRandomFactory.fromHashOf() 完全一致。
     *
     * @param key 字符串键（如 "octave_0", "octave_1" 等）
     * @return 新的随机数生成器
     */
    [[nodiscard]] std::unique_ptr<Xoroshiro128ppRandom> fromHashOf(const std::string& key) const;

    /**
     * @brief 从种子创建随机数生成器
     *
     * 将种子与工厂 seedLo/seedHi 分别 XOR，作为新 RNG 的种子。
     *
     * @param seed 输入种子
     * @return 新的随机数生成器
     */
    [[nodiscard]] std::unique_ptr<Xoroshiro128ppRandom> fromSeed(u64 seed) const;

    /**
     * @brief 从 3D 坐标创建随机数生成器
     *
     * 使用 MC 的 Mth.getSeed(x, y, z) 将坐标转换为单个 long，
     * 然后与工厂 seedLo XOR 作为新 RNG 的种子。
     *
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 新的随机数生成器
     */
    [[nodiscard]] std::unique_ptr<Xoroshiro128ppRandom> at(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取工厂种子低 64 位
     */
    [[nodiscard]] u64 seedLo() const { return m_seedLo; }

    /**
     * @brief 获取工厂种子高 64 位
     */
    [[nodiscard]] u64 seedHi() const { return m_seedHi; }

private:
    u64 m_seedLo;
    u64 m_seedHi;
};

} // namespace mc::math
