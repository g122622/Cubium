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
};

} // namespace mc::world::spawn
