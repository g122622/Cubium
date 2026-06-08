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

#pragma once

#include "CarverConfiguration.hpp"
#include "WorldCarver.hpp"
#include <vector>

namespace mc {

/**
 * @brief 峡谷雕刻器
 *
 * 生成峡谷地形特征。峡谷是长而蜿蜒的地下峡谷，具有变化的宽度和深度。
 *
 * 参考 MC 1.21.11: net.minecraft.world.level.levelgen.carver.CanyonWorldCarver
 *
 * 使用 CanyonCarverConfiguration 进行配置，包括：
 * - probability: 生成概率
 * - y: 起始高度提供器
 * - yScale: Y轴缩放
 * - verticalRotation: 垂直旋转角度
 * - shape: 峡谷形状配置（距离因子、厚度、宽度平滑度等）
 */
class CanyonCarver : public WorldCarver<CanyonCarverConfiguration> {
public:
    /**
     * @brief 构造峡谷雕刻器
     * @param maxHeight 最大雕刻高度
     */
    explicit CanyonCarver(i32 maxHeight = world::MAX_BUILD_HEIGHT);

    ~CanyonCarver() override = default;

    /**
     * @brief 在区块中雕刻峡谷
     *
     * @param chunk 要雕刻的区块
     * @param context 雕刻上下文
     * @param biomeSource 生物群系源
     * @param targetChunkX 目标区块 X 坐标（雕刻写入的区块）
     * @param targetChunkZ 目标区块 Z 坐标
     * @param originChunkX 起始区块 X 坐标（雕刻起源的区块）
     * @param originChunkZ 起始区块 Z 坐标
     * @param carvingMask 雕刻掩码
     * @param rng 已初始化的随机数生成器
     * @param config 峡谷配置
     * @return 是否雕刻了任何方块
     */
    bool carve(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::BiomeSource& biomeSource,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        ChunkCoord originChunkX,
        ChunkCoord originChunkZ,
        CarvingMask& carvingMask,
        math::IRandom& rng,
        const CanyonCarverConfiguration& config) override;

    /**
     * @brief 检查是否应该在这个起始区块执行雕刻
     *
     * @param rng 随机数生成器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param config 峡谷配置
     * @return 是否应该雕刻
     */
    [[nodiscard]] bool shouldCarve(math::IRandom& rng,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        const CanyonCarverConfiguration& config) const override;

private:
    /**
     * @brief 初始化宽度因子数组
     *
     * MC: initWidthFactors — 为每个高度预计算半径变化因子
     *
     * @param context 雕刻上下文
     * @param config 峡谷配置
     * @param rng 随机数生成器
     * @return 宽度因子数组（大小为 genDepth）
     */
    [[nodiscard]] std::vector<f32> _initWidthFactors(
        CarvingContext& context, const CanyonCarverConfiguration& config, math::IRandom& rng) const;

    /**
     * @brief 更新垂直半径
     *
     * MC: updateVerticalRadius — 基于进度计算垂直半径
     * f = 1.0 - abs(0.5 - progress) * 2.0
     * f1 = verticalRadiusDefaultFactor + verticalRadiusCenterFactor * f
     * result = f1 * baseRadius * randomBetween(0.75, 1.0)
     *
     * @param config 峡谷配置
     * @param rng 随机数生成器
     * @param baseRadius 基础垂直半径
     * @param totalSteps 总步数
     * @param currentStep 当前步数
     * @return 更新后的垂直半径
     */
    [[nodiscard]] f32 _updateVerticalRadius(const CanyonCarverConfiguration& config,
        math::IRandom& rng,
        f32 baseRadius,
        f32 totalSteps,
        f32 currentStep) const;

    /**
     * @brief 生成蜿蜒峡谷
     *
     * @param chunk 区块数据
     * @param context 雕刻上下文
     * @param biomeSource 生物群系源
     * @param targetChunkX 目标区块 X 坐标
     * @param targetChunkZ 目标区块 Z 坐标
     * @param seed 随机种子
     * @param startX 起始 X 坐标
     * @param startY 起始 Y 坐标
     * @param startZ 起始 Z 坐标
     * @param thickness 厚度
     * @param yaw 偏航角（水平方向）
     * @param pitch 俯仰角（垂直方向）
     * @param startIndex 起始索引
     * @param endIndex 结束索引
     * @param yScale Y轴缩放
     * @param carvingMask 雕刻掩码
     * @param config 峡谷配置
     */
    void _generateCanyon(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::BiomeSource& biomeSource,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        i64 seed,
        f32 startX,
        f32 startY,
        f32 startZ,
        f32 thickness,
        f32 yaw,
        f32 pitch,
        i32 startIndex,
        i32 endIndex,
        f32 yScale,
        CarvingMask& carvingMask,
        const CanyonCarverConfiguration& config);

    /**
     * @brief 创建椭球跳过检查器
     *
     * MC: shouldSkip — 使用高度阈值表进行厚度检测
     * 公式: (dx*dx + dz*dz) * threshold + dy*dy/6.0 >= 1.0
     *
     * @param context 雕刻上下文
     * @param heightThresholds 高度阈值数组
     * @return 跳过检查回调
     */
    [[nodiscard]] CarveSkipChecker _createSkipChecker(
        CarvingContext& context, const std::vector<f32>& heightThresholds) const;
};

} // namespace mc
