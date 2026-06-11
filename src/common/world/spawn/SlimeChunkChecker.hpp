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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"

namespace mc::world::spawn {

/**
 * @brief 史莱姆区块判断工具
 *
 * 使用世界种子确定性判断某个区块是否为史莱姆区块。
 * 算法复刻 MC Java 版 WorldgenRandom.seedSlimeChunk() 逻辑，
 * 使用 Java LegacyRandomSource (48位 LCG) 生成确定性随机数。
 *
 * 史莱姆区块判断流程：
 * 1. 用世界种子、区块坐标和固定盐值计算区块种子
 * 2. 用区块种子初始化 Java LCG 随机数生成器
 * 3. 调用 nextInt(10)，若结果为 0 则该区块为史莱姆区块（10% 概率）
 */
class SlimeChunkChecker {
public:
    /**
     * @brief 判断指定区块是否为史莱姆区块
     *
     * @param worldSeed 世界种子
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return true 如果该区块为史莱姆区块
     */
    [[nodiscard]] static bool isSlimeChunk(u64 worldSeed, i32 chunkX, i32 chunkZ);

    /**
     * @brief 计算史莱姆区块种子
     *
     * 使用 MC Java 版 WorldgenRandom.seedSlimeChunk 公式：
     * seed = (worldSeed + chunkX^2 * 4987142 + chunkX * 5947611
     *       + chunkZ^2 * 4392871L + chunkZ * 389711) ^ 987234911L
     *
     * @param worldSeed 世界种子
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 区块种子
     */
    [[nodiscard]] static u64 computeSlimeChunkSeed(u64 worldSeed, i32 chunkX, i32 chunkZ);

    /**
     * @brief 获取地表史莱姆生成概率（基于月相）
     *
     * 满月时概率最高 (0.5)，新月时概率为 0。
     * 月相亮度数组: {1.0, 0.75, 0.5, 0.25, 0.0, 0.25, 0.5, 0.75}
     * 生成概率 = 月亮亮度 * 0.5
     *
     * @param moonPhase 月相索引 (0-7, 0=满月)
     * @return 地表史莱姆生成概率 [0.0, 0.5]
     */
    [[nodiscard]] static f32 getSurfaceSlimeSpawnChance(i32 moonPhase);

private:
    /// Java LCG 乘数常量
    static constexpr u64 JAVA_LCG_MULTIPLIER = 25214903917ULL;
    /// Java LCG 增量常量
    static constexpr u64 JAVA_LCG_INCREMENT = 11ULL;
    /// Java LCG 48位掩码
    static constexpr u64 JAVA_LCG_MASK = (1ULL << 48) - 1;

    /// 史莱姆区块种子公式的 X^2 系数
    static constexpr i64 CHUNK_X_SQUARED_COEFF = 4987142LL;
    /// 史莱姆区块种子公式的 X 系数
    static constexpr i64 CHUNK_X_COEFF = 5947611LL;
    /// 史莱姆区块种子公式的 Z^2 系数
    static constexpr i64 CHUNK_Z_SQUARED_COEFF = 4392871LL;
    /// 史莱姆区块种子公式的 Z 系数
    static constexpr i64 CHUNK_Z_COEFF = 389711LL;
    /// 史莱姆区块种子公式的盐值
    static constexpr u64 SLIME_CHUNK_SALT = 987234911ULL;

    /// 月亮亮度数组 (索引 0=满月, 4=新月)
    static constexpr f32 MOON_BRIGHTNESS_PER_PHASE[8] = {1.0f, 0.75f, 0.5f, 0.25f, 0.0f, 0.25f, 0.5f, 0.75f};

    /**
     * @brief Java LegacyRandomSource 的 setSeed 操作
     *
     * Java版 Random.setSeed(seed):
     * internalSeed = (seed ^ 0x5DEECE66DL) & ((1L << 48) - 1)
     *
     * @param seed 输入种子
     * @return 内部状态值
     */
    [[nodiscard]] static u64 javaSetSeed(u64 seed);

    /**
     * @brief Java LegacyRandomSource 的 next(bits) 操作
     *
     * Java版 Random.next(bits):
     * internalSeed = (internalSeed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1)
     * return (int)(internalSeed >>> (48 - bits))
     *
     * @param state 当前内部状态（引用，会被修改）
     * @param bits 要返回的位数 (1-32)
     * @return 低位 bits 位随机数
     */
    [[nodiscard]] static i32 javaNext(u64& state, i32 bits);

    /**
     * @brief Java LegacyRandomSource 的 nextInt(bound) 操作
     *
     * 当 bound 不是 2 的幂时，使用拒绝采样法保证均匀分布。
     *
     * @param state 当前内部状态（引用，会被修改）
     * @param bound 上界（正整数）
     * @return [0, bound) 范围内的随机整数
     */
    [[nodiscard]] static i32 javaNextInt(u64& state, i32 bound);
};

} // namespace mc::world::spawn
