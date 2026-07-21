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

#include "../ColorResolver.hpp"
#include "BiomeColorCache.hpp"
#include "common/world/biome/Biome.hpp"
#include <array>
#include <atomic>
#include <functional>

namespace mc {
namespace world::chunk {
class ChunkData;
}
using world::chunk::ChunkData;
namespace world::biome {
class BiomeRegistry;
} // namespace world::biome

namespace client {

/**
 * @brief 生物群系颜色混合器
 *
 * 实现生物群系边界处的颜色平滑过渡。
 *
 * 当混合半径 > 0 时，在 (2r+1) x (2r+1) 区域内采样周围生物群系颜色，
 * 对 RGB 分量分别求平均，实现平滑过渡效果。
 *
 * 使用示例：
 * @code
 * BiomeColorBlender blender;
 * blender.setBlendRadius(2);  // 5x5 混合区域
 *
 * // 获取混合后的草颜色
 * u32 grassColor = blender.getBlendedColor(
 *     world, x, y, z,
 *     BiomeColors::grassColorResolver()
 * );
 * @endcode
 */
class BiomeColorBlender {
public:
    /**
     * @brief 最大混合半径
     *
     * MC 1.16.5 默认为 7，但实际使用 2（5x5 混合区域）。
     * 值越大性能消耗越高。
     */
    static constexpr i32 MAX_BLEND_RADIUS = 7;

    /**
     * @brief 预定义的 ColorResolver ID
     *
     * 用于缓存管理，避免频繁的类型识别
     */
    enum class ResolverId : size_t { Grass = 0, Foliage = 1, Water = 2, DryFoliage = 3, Count = 4 };

    /**
     * @brief 生物群系访问接口
     *
     * 抽象的生物群系访问接口，支持从不同数据源获取生物群系。
     */
    class IBiomeAccessor {
    public:
        virtual ~IBiomeAccessor() = default;

        /**
         * @brief 获取指定位置的生物群系
         * @param x 方块X坐标（世界坐标）
         * @param y 方块Y坐标（世界坐标）
         * @param z 方块Z坐标（世界坐标）
         * @return 生物群系指针，如果未加载返回 nullptr
         */
        [[nodiscard]] virtual const Biome* getBiome(i32 x, i32 y, i32 z) const = 0;

        /**
         * @brief 检查指定区块是否已加载
         */
        [[nodiscard]] virtual bool isChunkLoaded(ChunkCoord x, ChunkCoord z) const = 0;
    };

    BiomeColorBlender() = default;

    // ========================================================================
    // 配置
    // ========================================================================

    /**
     * @brief 设置混合半径
     *
     * 混合区域大小为 (2r+1) x (2r+1)。
     * - r = 0: 无混合，直接使用当前生物群系颜色
     * - r = 2: 5x5 混合区域（MC 默认）
     * - r = 7: 15x15 混合区域（最大）
     *
     * @param radius 混合半径 (0-7)
     */
    void setBlendRadius(i32 radius);

    /**
     * @brief 获取当前混合半径
     */
    [[nodiscard]] i32 blendRadius() const noexcept { return m_blendRadius; }

    /**
     * @brief 启用/禁用缓存
     *
     * 默认启用。禁用后每次都重新计算。
     */
    void setCacheEnabled(bool enabled) { m_cacheEnabled = enabled; }

    /**
     * @brief 缓存是否启用
     */
    [[nodiscard]] bool isCacheEnabled() const noexcept { return m_cacheEnabled; }

    // ========================================================================
    // Colormap 设置
    // ========================================================================

    /**
     * @brief 设置草颜色映射表
     * @param colorMap 256x256 颜色映射表指针（调用方保证生命周期）
     */
    void setGrassColorMap(const std::array<u32, 65536>* colorMap) { m_grassColorMap = colorMap; }

    /**
     * @brief 设置树叶颜色映射表
     * @param colorMap 256x256 颜色映射表指针（调用方保证生命周期）
     */
    void setFoliageColorMap(const std::array<u32, 65536>* colorMap) { m_foliageColorMap = colorMap; }

    /**
     * @brief 设置干枯植被颜色映射表
     * @param colorMap 256x256 颜色映射表指针（调用方保证生命周期）
     */
    void setDryFoliageColorMap(const std::array<u32, 65536>* colorMap) { m_dryFoliageColorMap = colorMap; }

    // ========================================================================
    // 颜色获取
    // ========================================================================

    /**
     * @brief 获取混合后的颜色
     *
     * 如果混合半径为 0，直接返回当前生物群系颜色。
     * 否则在混合区域内采样并平均。
     *
     * @param accessor 生物群系访问器
     * @param x 方块X坐标
     * @param y 方块Y坐标
     * @param z 方块Z坐标
     * @param resolver 颜色解析器
     * @param resolverId 解析器ID（用于缓存和 colormap 选择）
     * @return 混合后的颜色 (RGB)
     */
    [[nodiscard]] u32 getBlendedColor(
        const IBiomeAccessor& accessor, i32 x, i32 y, i32 z, const ColorResolver& resolver, ResolverId resolverId);

    /**
     * @brief 获取混合后的颜色（带缓存）
     *
     * 首先检查缓存，未命中则计算并缓存结果。
     */
    [[nodiscard]] u32 getBlendedColorCached(
        const IBiomeAccessor& accessor, i32 x, i32 y, i32 z, const ColorResolver& resolver, ResolverId resolverId);

    // ========================================================================
    // 缓存管理
    // ========================================================================

    /**
     * @brief 获取颜色缓存
     */
    [[nodiscard]] BiomeColorCache& cache() { return m_cache; }
    [[nodiscard]] const BiomeColorCache& cache() const { return m_cache; }

    /**
     * @brief 使指定区块的缓存失效
     */
    void invalidateChunk(ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 清空所有缓存
     */
    void clearCache();

    /**
     * @brief 获取缓存统计信息
     */
    [[nodiscard]] BiomeColorCache::Stats getCacheStats() const;

    // ========================================================================
    // 工具方法
    // ========================================================================

    /**
     * @brief 颜色混合：对 RGB 分量求平均
     *
     * @param colors 颜色数组
     * @param count 颜色数量
     * @return 平均颜色
     */
    [[nodiscard]] static u32 averageColors(const u32* colors, size_t count) noexcept;

    /**
     * @brief 获取 ResolverId
     */
    [[nodiscard]] static ResolverId getResolverId(const ColorResolver& resolver);

private:
    /**
     * @brief 无混合，直接获取颜色
     */
    [[nodiscard]] u32 _getColorDirect(
        const IBiomeAccessor& accessor, i32 x, i32 y, i32 z, const ColorResolver& resolver, ResolverId resolverId);

    /**
     * @brief 带混合的颜色获取
     */
    [[nodiscard]] u32 _getColorBlended(
        const IBiomeAccessor& accessor, i32 x, i32 y, i32 z, const ColorResolver& resolver, ResolverId resolverId);

    /**
     * @brief 获取默认颜色
     */
    [[nodiscard]] static u32 _getDefaultColor(ResolverId resolverId) noexcept;

    /**
     * @brief 获取 colormap
     */
    [[nodiscard]] const std::array<u32, 65536>* _getColorMap(ResolverId resolverId) const noexcept;

    i32 m_blendRadius = 2; // 默认 5x5 混合区域
    bool m_cacheEnabled = true;
    BiomeColorCache m_cache;

    // Colormap 指针（由 ChunkMesher 管理）
    const std::array<u32, 65536>* m_grassColorMap = nullptr;
    const std::array<u32, 65536>* m_foliageColorMap = nullptr;
    const std::array<u32, 65536>* m_dryFoliageColorMap = nullptr;

    // 工作缓冲区（避免频繁分配）
    std::vector<u32> m_colorBuffer;
};

} // namespace client
} // namespace mc
