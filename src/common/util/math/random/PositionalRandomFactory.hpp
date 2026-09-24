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
#include <memory>
#include <string>

namespace mc::math {

// 前向声明：IRandom.hpp 需要本类的完整定义（forkPositional 按值返回），
// 故此处不能反向 include IRandom.hpp，否则循环包含。unique_ptr 的析构在 .cpp 中完成。
class IRandom;

/**
 * @brief 位置随机工厂（对应 MC Java 版的两种 PositionalRandomFactory 实现）
 *
 * 原版有两个 flavor，派生算法完全不同，故本类用枚举区分而不是统一处理：
 *
 * - **Xoroshiro**（XoroshiroPositionalRandomFactory，128 位种子）：
 *     fromHashOf(name) → MD5(name) 的 128 位摘要 XOR (seedLo, seedHi)
 *     fromSeed(seed)   → (seed ^ seedLo, seed ^ seedHi)
 *     at(x,y,z)        → (Mth.getSeed(x,y,z) ^ seedLo, seedHi)
 *   由 XoroshiroRandomSource.forkPositional() 产出。
 *
 * - **Legacy**（LegacyPositionalRandomFactory，64 位种子）：
 *     fromHashOf(name) → new LegacyRandomSource(name.hashCode() ^ seed)
 *     fromSeed(seed)   → new LegacyRandomSource(seed)      【注意：不与种子异或】
 *     at(x,y,z)        → new LegacyRandomSource(Mth.getSeed(x,y,z) ^ seed)
 *   由 LegacyRandomSource.forkPositional() 产出。WorldgenRandom 的 forkPositional 转发
 *   内层，故 `new WorldgenRandom(new LegacyRandomSource(seed)).forkPositional()` 得到的是它。
 *
 * 【为什么必须区分】GeodeFeature / DualNoiseProvider / NoiseBasedStateProvider 都走
 * Legacy flavor，其 fromHashOf 用的是 Java String.hashCode()（32 位有符号），
 * 而 Xoroshiro flavor 用的是 MD5——两者派生出的 RNG 序列毫无关系。若不区分，
 * 晶洞的噪声图案会整体偏离原版。
 */
class PositionalRandomFactory {
public:
    /// 派生算法 flavor
    enum class Flavor : u8 {
        Xoroshiro, ///< 128 位种子，MD5 哈希
        Legacy,    ///< 64 位种子，Java String.hashCode()
    };

    /**
     * @brief 构造 Xoroshiro flavor 的位置随机工厂
     * @param seedLo 工厂种子低 64 位
     * @param seedHi 工厂种子高 64 位
     */
    PositionalRandomFactory(u64 seedLo, u64 seedHi);

    /**
     * @brief 构造 Legacy flavor 的位置随机工厂
     * @param seed 64 位工厂种子（对应 LegacyPositionalRandomFactory 的 seed 字段）
     */
    explicit PositionalRandomFactory(u64 seed);

    /**
     * @brief 从字符串哈希创建随机数生成器
     *
     * Xoroshiro flavor：MD5(key) 的 128 位摘要与工厂种子逐位异或。
     * Legacy flavor：Java `key.hashCode() ^ seed`，注意 hashCode 是 32 位有符号 int，
     *                与 64 位 seed 异或时按符号扩展（负数高位全 1）。
     *
     * @param key 字符串键（如 "octave_0"、"minecraft:terrain" 等）
     * @return 新的随机数生成器（flavor 对应 Xoroshiro128ppRandom 或 JavaLegacyRandom）
     */
    [[nodiscard]] std::unique_ptr<IRandom> fromHashOf(const std::string& key) const;

    /**
     * @brief 从种子创建随机数生成器
     *
     * Xoroshiro flavor：种子与 seedLo/seedHi 分别 XOR。
     * Legacy flavor：直接用种子构造 LegacyRandomSource（**不与工厂种子异或**）。
     *
     * @param seed 输入种子
     * @return 新的随机数生成器
     */
    [[nodiscard]] std::unique_ptr<IRandom> fromSeed(u64 seed) const;

    /**
     * @brief 从 3D 坐标创建随机数生成器
     *
     * 两种 flavor 都用 Mth.getSeed(x, y, z) 与工厂种子异或，区别只在异或结果
     * 如何喂给各自的 RNG（Xoroshiro 作 seedLo、保留 seedHi；Legacy 作 64 位种子）。
     *
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 新的随机数生成器
     */
    [[nodiscard]] std::unique_ptr<IRandom> at(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取 flavor
     */
    [[nodiscard]] Flavor flavor() const { return m_flavor; }

    /**
     * @brief 获取工厂种子低 64 位（Legacy flavor 下即唯一的工厂种子）
     */
    [[nodiscard]] u64 seedLo() const { return m_seedLo; }

    /**
     * @brief 获取工厂种子高 64 位（Legacy flavor 下恒为 0）
     */
    [[nodiscard]] u64 seedHi() const { return m_seedHi; }

private:
    Flavor m_flavor;
    u64 m_seedLo;
    u64 m_seedHi;
};

} // namespace mc::math
