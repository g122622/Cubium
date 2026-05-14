#pragma once

#include "../../MeshTypes.hpp"
#include "common/core/Types.hpp"
#include <array>

namespace mc {

// 前向声明
class ChunkData;
class BlockState;

namespace client {
namespace renderer {

/**
 * @brief 环境光遮蔽(AO)计算器
 *
 * 为方块面的每个顶点计算平滑光照值。
 * 算法基于MC 1.16.5的BlockModelRenderer.AmbientOcclusionFace实现。
 *
 * 核心概念:
 * 1. 对于每个面，采样4个角落位置的光照和AO亮度
 * 2. 检查角落外侧方块的透明度，决定是否需要采样对角线位置
 * 3. 根据透明度条件，计算每个顶点的最终亮度和颜色乘数
 *
 * 参考: net.minecraft.client.renderer.BlockModelRenderer.AmbientOcclusionFace
 */
class AmbientOcclusionCalculator {
public:
    /**
     * @brief AO计算结果
     *
     * 包含4个顶点的光照和颜色乘数。
     */
    struct Result {
        std::array<float, 4> vertexColorMultiplier{}; ///< 顶点颜色乘数 (0.0-1.0)
        std::array<u8, 4> vertexSkyLight{};           ///< 顶点天空光 (0-15)
        std::array<u8, 4> vertexBlockLight{};         ///< 顶点方块光 (0-15)
    };

    /**
     * @brief 计算面的AO值
     *
     * @param chunk 当前区块数据
     * @param blockX 方块X坐标（区块内）
     * @param blockY 方块Y坐标（世界坐标）
     * @param blockZ 方块Z坐标（区块内）
     * @param face 面朝向
     * @param neighborChunks 周围区块指针数组 (顺序: -X, +X, -Z, +Z, -Y, +Y)
     * @param nonCubicWeights 非立方体权重数组 (nullptr表示使用立方体模式)
     * @return AO计算结果
     */
    [[nodiscard]] Result calculate(const ChunkData& chunk,
        i32 blockX,
        i32 blockY,
        i32 blockZ,
        Face face,
        const ChunkData* neighborChunks[6],
        const float* nonCubicWeights = nullptr);

private:
    /**
     * @brief 角落采样数据
     */
    struct CornerSample {
        u8 skyLight = 0;           ///< 天空光照 (0-15)
        u8 blockLight = 0;         ///< 方块光照 (0-15)
        float aoBrightness = 1.0f; ///< AO亮度 (0.2 或 1.0)
    };

    /**
     * @brief 采样指定位置的光照和AO信息
     *
     * @param chunk 当前区块
     * @param x X坐标（可以是区块外坐标）
     * @param y Y坐标
     * @param z Z坐标（可以是区块外坐标）
     * @param neighborChunks 周围区块
     * @return 采样结果
     */
    [[nodiscard]] CornerSample samplePosition(
        const ChunkData& chunk, i32 x, i32 y, i32 z, const ChunkData* neighborChunks[6]) const;

    /**
     * @brief 检查指定位置是否透明（不透明方块返回false）
     *
     * @param chunk 当前区块
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param neighborChunks 周围区块
     * @return 如果位置是透明的（可以透光）返回true
     */
    [[nodiscard]] bool isTransparent(
        const ChunkData& chunk, i32 x, i32 y, i32 z, const ChunkData* neighborChunks[6]) const;

    /**
     * @brief 计算AO亮度
     *
     * 实心方块返回0.2，透明方块返回1.0
     *
     * @param state 方块状态（可为nullptr表示空气）
     * @return AO亮度值
     */
    [[nodiscard]] static float getAoBrightness(const BlockState* state);

    /**
     * @brief 检查方块是否不透明（用于AO计算）
     *
     * @param state 方块状态（可为nullptr表示空气）
     * @return 如果有不透明碰撞形状返回true
     */
    [[nodiscard]] static bool hasOpaqueCollisionShape(const BlockState* state);

    /**
     * @brief 计算AO亮度（四个值的平均值，特殊处理0值）
     *
     * 参考: net.minecraft.client.renderer.BlockModelRenderer.AmbientOcclusionFace#getAoBrightness
     *
     * @param br1 角落1亮度 (packed: skyLight << 20 | blockLight << 4)
     * @param br2 角落2亮度
     * @param br3 角落3亮度
     * @param br4 中心亮度
     * @return 打包的亮度值
     */
    [[nodiscard]] static u32 getAoBrightness(u32 br1, u32 br2, u32 br3, u32 br4);

    /**
     * @brief 将天空光和方块光打包为单一值
     *
     * 格式: skyLight << 20 | blockLight << 4
     * 与MC原版打包格式一致 (0xF000F0 掩码)
     *
     * @param skyLight 天空光 (0-15)
     * @param blockLight 方块光 (0-15)
     * @return 打包的亮度值
     */
    [[nodiscard]] static u32 packLight(u8 skyLight, u8 blockLight);

    /**
     * @brief 解包天空光
     */
    [[nodiscard]] static u8 unpackSkyLight(u32 packed);

    /**
     * @brief 解包方块光
     */
    [[nodiscard]] static u8 unpackBlockLight(u32 packed);

    /**
     * @brief 根据权重计算顶点亮度
     *
     * @param b1-b4 四个亮度值
     * @param w1-w4 四个权重
     * @return 插值后的打包亮度
     */
    [[nodiscard]] static u32 getVertexBrightness(
        u32 b1, u32 b2, u32 b3, u32 b4, float w1, float w2, float w3, float w4);

    /**
     * @brief 获取指定位置的光照值（天空光和方块光的打包值）
     *
     * 参考: net.minecraft.world.WorldRenderer#getPackedLightmapCoords
     *
     * @param chunk 区块数据
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param neighborChunks 周围区块
     * @return 打包的光照值
     */
    [[nodiscard]] static u32 getPackedLight(
        const ChunkData& chunk, i32 x, i32 y, i32 z, const ChunkData* neighborChunks[6]);
};

} // namespace renderer
} // namespace client
} // namespace mc
