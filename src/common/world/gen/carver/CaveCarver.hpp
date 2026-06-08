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
#include "CarvingContext.hpp"
#include "WorldCarver.hpp"
#include "common/core/Types.hpp"

namespace mc {

/**
 * @brief 洞穴世界雕刻器
 *
 * 生成洞穴系统，继承 WorldCarver 基类。
 * 使用 CaveCarverConfiguration 配置，支持灵活的高度范围、半径乘数和地板高度设置。
 *
 * 参考 MC 1.21.11: net.minecraft.world.level.levelgen.carver.CaveWorldCarver
 *
 * 使用方法：
 * @code
 * CaveCarver carver;
 * CarvingMask mask(chunkX, chunkZ);
 * CaveCarverConfiguration config = ConfiguredCarvers::createOverworldCaveConfig(replaceableTag);
 * carver.carve(chunk, context, biomeSource, targetChunkX, targetChunkZ, originChunkX, originChunkZ, mask, rng, config);
 * @endcode
 *
 * @note 洞穴雕刻应在 NOISE 阶段之后、SURFACE 阶段之前进行
 */
class CaveCarver : public WorldCarver<CaveCarverConfiguration> {
public:
    /**
     * @brief 构造洞穴雕刻器
     */
    CaveCarver();

    ~CaveCarver() override = default;

    /**
     * @brief 在区块中雕刻洞穴
     *
     * @param chunk 要雕刻的目标区块
     * @param context 雕刻上下文
     * @param biomeSource 生物群系源（用于获取地表方块）
     * @param targetChunkX 目标区块 X 坐标（雕刻写入的区块）
     * @param targetChunkZ 目标区块 Z 坐标
     * @param originChunkX 起始区块 X 坐标（雕刻起源的区块）
     * @param originChunkZ 起始区块 Z 坐标
     * @param carvingMask 雕刻掩码
     * @param rng 已初始化的随机数生成器
     * @param config 洞穴配置
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
        const CaveCarverConfiguration& config) override;

    /**
     * @brief 检查是否应该在这个区块生成洞穴
     * 使用配置中的概率值
     */
    [[nodiscard]] bool shouldCarve(math::IRandom& rng,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        const CaveCarverConfiguration& config) const noexcept override;

protected:
    /**
     * @brief 获取洞穴最大数量上限
     * MC 1.21.11: getCaveBound() - 嵌套随机生成洞穴数量的最大值
     * @return 默认返回 15，下界版本重写为 10
     */
    [[nodiscard]] virtual i32 getCaveBound() const noexcept { return 15; }

    /**
     * @brief 获取洞穴厚度（半径基础值）
     * MC 1.21.11: getThickness()
     * @param rng 随机数生成器
     * @return 厚度值
     */
    [[nodiscard]] virtual f32 getThickness(math::IRandom& rng) const;

    /**
     * @brief 获取 Y 缩放因子
     * MC 1.21.11: getYScale()
     * @return 默认返回 1.0，下界版本重写为其他值
     */
    [[nodiscard]] virtual f64 getYScale() const noexcept { return 1.0; }

private:
    /**
     * @brief 生成单个洞穴隧道
     *
     * @param chunk 区块数据
     * @param context 雕刻上下文
     * @param biomeSource 生物群系源
     * @param seaLevel 海平面高度
     * @param targetChunkX 目标区块X坐标（用于 isInCarvingRange 和 carveEllipsoid）
     * @param targetChunkZ 目标区块Z坐标
     * @param seed 随机种子
     * @param startX 起始X坐标
     * @param startY 起始Y坐标
     * @param startZ 起始Z坐标
     * @param horizontalRadiusMultiplier 水平半径乘数
     * @param verticalRadiusMultiplier 垂直半径乘数
     * @param thickness 洞穴厚度
     * @param yaw 偏航角（水平方向）
     * @param pitch 俯仰角（垂直方向）
     * @param startIndex 起始索引
     * @param endIndex 结束索引
     * @param yScale Y缩放因子
     * @param carvingMask 雕刻掩码
     * @param skipChecker 椭球跳过检查回调
     * @param config 配置
     */
    void _createTunnel(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::BiomeSource& biomeSource,
        i32 seaLevel,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        i64 seed,
        f64 startX,
        f64 startY,
        f64 startZ,
        f64 horizontalRadiusMultiplier,
        f64 verticalRadiusMultiplier,
        f32 thickness,
        f32 yaw,
        f32 pitch,
        i32 startIndex,
        i32 endIndex,
        f64 yScale,
        CarvingMask& carvingMask,
        const CarveSkipChecker& skipChecker,
        const CaveCarverConfiguration& config);

    /**
     * @brief 生成圆形洞穴房间
     *
     * @param chunk 区块数据
     * @param context 雕刻上下文
     * @param biomeSource 生物群系源
     * @param seaLevel 海平面高度
     * @param targetChunkX 目标区块X坐标
     * @param targetChunkZ 目标区块Z坐标
     * @param centerX 中心X坐标
     * @param centerY 中心Y坐标
     * @param centerZ 中心Z坐标
     * @param radius 半径
     * @param yScale Y缩放因子
     * @param carvingMask 雕刻掩码
     * @param skipChecker 椭球跳过检查回调
     * @param config 配置
     */
    void _createRoom(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::BiomeSource& biomeSource,
        i32 seaLevel,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        f64 centerX,
        f64 centerY,
        f64 centerZ,
        f32 radius,
        f64 yScale,
        CarvingMask& carvingMask,
        const CarveSkipChecker& skipChecker,
        const CaveCarverConfiguration& config);
};

} // namespace mc
