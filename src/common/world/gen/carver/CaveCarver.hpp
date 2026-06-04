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

#include "../../../core/Constants.hpp"
#include "WorldCarver.hpp"

namespace mc {

/**
 * @brief 洞穴世界雕刻器
 *
 * 生成洞穴系统，继承 WorldCarver 基类。
 * 使用雕刻掩码防止重复雕刻。
 *
 * 使用方法：
 * @code
 * CaveCarver carver;
 * CarvingMask mask(chunkX, chunkZ);
 * ProbabilityConfig config(0.14285715f);
 * carver.carve(chunk, biomeSource, seaLevel, chunkX, chunkZ, mask, config);
 * @endcode
 *
 * @note 洞穴雕刻应在 NOISE 阶段之后、SURFACE 阶段之前进行
 */
class CaveCarver : public WorldCarver<ProbabilityConfig> {
public:
    /**
     * @brief 构造洞穴雕刻器
     * @param maxHeight 最大雕刻高度
     */
    explicit CaveCarver(i32 maxHeight = world::MAX_BUILD_HEIGHT);

    ~CaveCarver() override = default;

    /**
     * @brief 在区块中雕刻洞穴
     *
     * @param chunk 要雕刻的区块
     * @param biomeSource 生物群系源（用于获取地表方块）
     * @param seaLevel 海平面高度
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param carvingMask 雕刻掩码
     * @param config 配置
     * @return 是否雕刻了任何方块
     */
    bool carve(ChunkPrimer& chunk,
        const world::biome::BiomeSource& biomeSource,
        i32 seaLevel,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        CarvingMask& carvingMask,
        const ProbabilityConfig& config) override;

    /**
     * @brief 检查是否应该在这个区块生成洞穴
     */
    [[nodiscard]] bool shouldCarve(math::IRandom& rng,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        const ProbabilityConfig& config) const noexcept override;

protected:
    /**
     * @brief 检查是否应该跳过椭球内的这个位置
     * @note 洞穴雕刻器使用标准椭球检测
     */
    [[nodiscard]] bool shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const noexcept override;

    /**
     * @brief 获取最大洞穴生成尝试次数
     */
    [[nodiscard]] virtual i32 getMaxCaveCount() const noexcept { return 15; }

    /**
     * @brief 获取洞穴起始Y坐标
     * @param rng 随机数生成器
     * @return Y坐标
     */
    [[nodiscard]] virtual i32 getCaveStartY(math::IRandom& rng) const;

    /**
     * @brief 获取洞穴半径
     * @param rng 随机数生成器
     * @return 半径
     */
    [[nodiscard]] virtual f32 getCaveRadius(math::IRandom& rng) const;

    /**
     * @brief 获取垂直缩放因子
     * @return 垂直缩放
     */
    [[nodiscard]] virtual f32 getVerticalScale() const noexcept { return 1.0f; }

private:
    /**
     * @brief 生成单个洞穴隧道
     *
     * @param chunk 区块数据
     * @param biomeSource 生物群系源
     * @param seaLevel 海平面高度
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param seed 随机种子
     * @param startX 起始X坐标
     * @param startY 起始Y坐标
     * @param startZ 起始Z坐标
     * @param radius 洞穴半径
     * @param yaw 偏航角（水平方向）
     * @param pitch 俯仰角（垂直方向）
     * @param startIndex 起始索引
     * @param endIndex 结束索引
     * @param verticalScale 垂直缩放
     * @param carvingMask 雕刻掩码
     */
    void _carveTunnel(ChunkPrimer& chunk,
        const world::biome::BiomeSource& biomeSource,
        i32 seaLevel,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        i64 seed,
        f32 startX,
        f32 startY,
        f32 startZ,
        f32 radius,
        f32 yaw,
        f32 pitch,
        i32 startIndex,
        i32 endIndex,
        f32 verticalScale,
        CarvingMask& carvingMask);

    /**
     * @brief 生成圆形洞穴房间
     *
     * @param chunk 区块数据
     * @param biomeSource 生物群系源
     * @param seaLevel 海平面高度
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param seed 随机种子
     * @param centerX 中心X坐标
     * @param centerY 中心Y坐标
     * @param centerZ 中心Z坐标
     * @param radius 房间半径
     * @param verticalScale 垂直缩放
     * @param carvingMask 雕刻掩码
     */
    void _carveRoom(ChunkPrimer& chunk,
        const world::biome::BiomeSource& biomeSource,
        i32 seaLevel,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        i64 seed,
        f32 centerX,
        f32 centerY,
        f32 centerZ,
        f32 radius,
        f32 verticalScale,
        CarvingMask& carvingMask);
};

} // namespace mc
