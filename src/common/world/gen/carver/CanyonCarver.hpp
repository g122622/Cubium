#pragma once

#include "WorldCarver.hpp"

namespace mc {

/**
 * @brief 峡谷雕刻器
 *
 * 参考 MC CanyonWorldCarver，生成峡谷地形。
 * 改进版本：
 * - 继承 WorldCarver 基类
 * - 使用预计算半径变化表（field_202536_i）
 * - 改进的蜿蜒曲线算法
 * - 使用雕刻掩码防止重复雕刻
 *
 * 使用方法：
 * @code
 * CanyonCarver carver;
 * CarvingMask mask(chunkX, chunkZ);
 * ProbabilityConfig config(0.02f);
 * carver.carve(chunk, biomeProvider, seaLevel, chunkX, chunkZ, mask, config);
 * @endcode
 *
 * @note 峡谷雕刻应在 NOISE 阶段之后、SURFACE 阶段之前进行
 * @note 参考 MC 1.16.5 CanyonWorldCarver
 */
class CanyonCarver : public WorldCarver<ProbabilityConfig> {
public:
    /**
     * @brief 构造峡谷雕刻器
     * @param maxHeight 最大雕刻高度
     */
    explicit CanyonCarver(i32 maxHeight = 256);

    ~CanyonCarver() override = default;

    /**
     * @brief 在区块中雕刻峡谷
     *
     * @param chunk 要雕刻的区块
     * @param biomeProvider 生物群系提供者
     * @param seaLevel 海平面高度
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param carvingMask 雕刻掩码
     * @param config 配置
     * @return 是否雕刻了任何方块
     */
    bool carve(ChunkPrimer& chunk,
               const BiomeProvider& biomeProvider,
               i32 seaLevel,
               ChunkCoord chunkX,
               ChunkCoord chunkZ,
               CarvingMask& carvingMask,
               const ProbabilityConfig& config) override;

    /**
     * @brief 检查是否应该在这个区块生成峡谷
     */
    [[nodiscard]] bool shouldCarve(
        math::IRandom& rng,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        const ProbabilityConfig& config) const override;

protected:
    /**
     * @brief 检查是否应该跳过椭球内的这个位置
     * @note 峡谷雕刻器使用特殊的厚度检测
     */
    [[nodiscard]] bool shouldSkipEllipsoidPosition(
        f32 dx, f32 dy, f32 dz, i32 y) const override;

private:
    /// 预计算的半径变化表（参考 MC field_202536_i，大小 1024）
    std::vector<f32> m_heightThresholds;

    /**
     * @brief 初始化半径变化表
     */
    void initializeHeightThresholds();

    /**
     * @brief 生成蜿蜒峡谷
     *
     * @param chunk 区块数据
     * @param biomeProvider 生物群系提供者
     * @param seaLevel 海平面高度
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param seed 随机种子
     * @param startX 起始 X 坐标
     * @param startY 起始 Y 坐标
     * @param startZ 起始 Z 坐标
     * @param radius 基础半径
     * @param yaw 偏航角（水平方向）
     * @param pitch 俯仰角（垂直方向）
     * @param startIndex 起始索引
     * @param endIndex 结束索引
     * @param horizontalScale 水平缩放
     * @param carvingMask 雕刻掩码
     */
    void generateCanyon(
        ChunkPrimer& chunk,
        const BiomeProvider& biomeProvider,
        i32 seaLevel,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        i64 seed,
        f32 startX, f32 startY, f32 startZ,
        f32 radius,
        f32 yaw, f32 pitch,
        i32 startIndex, i32 endIndex,
        f32 horizontalScale,
        CarvingMask& carvingMask);

    /**
     * @brief 更新半径（峡谷入口宽，深处窄）
     * @param baseRadius 基础半径
     * @param progress 进度 (0.0 - 1.0)
     * @param thresholds 半径变化表
     * @param index 当前索引
     * @return 更新后的半径
     */
    [[nodiscard]] f32 updateRadius(
        f32 baseRadius,
        f32 progress,
        const std::vector<f32>& thresholds,
        i32 index) const;
};

} // namespace mc
