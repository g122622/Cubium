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

#include "BiomeColorBlender.hpp"
#include "../BiomeColors.hpp"
#include "client/world/color/ColorResolver.hpp"
#include "client/world/color/blend/BiomeColorCache.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include <algorithm>
#include <array>
#include <cstddef>

namespace mc::client {

// ============================================================================
// BiomeColorBlender 实现
// ============================================================================

void BiomeColorBlender::setBlendRadius(i32 radius)
{
    m_blendRadius = std::clamp(radius, 0, MAX_BLEND_RADIUS);

    // 预分配工作缓冲区
    const size_t maxSize = static_cast<size_t>(MAX_BLEND_RADIUS * 2 + 1);
    m_colorBuffer.reserve(maxSize * maxSize);
}

void BiomeColorBlender::invalidateChunk(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    m_cache.invalidateChunk(chunkX, chunkZ);
}

void BiomeColorBlender::clearCache()
{
    m_cache.clear();
}

BiomeColorCache::Stats BiomeColorBlender::getCacheStats() const
{
    return m_cache.getStats();
}

u32 BiomeColorBlender::getBlendedColor(
    const IBiomeAccessor& accessor, i32 x, i32 y, i32 z, const ColorResolver& resolver, ResolverId resolverId)
{
    if (m_cacheEnabled) {
        return getBlendedColorCached(accessor, x, y, z, resolver, resolverId);
    }

    if (m_blendRadius == 0) {
        return _getColorDirect(accessor, x, y, z, resolver, resolverId);
    }

    return _getColorBlended(accessor, x, y, z, resolver, resolverId);
}

u32 BiomeColorBlender::getBlendedColorCached(
    const IBiomeAccessor& accessor, i32 x, i32 y, i32 z, const ColorResolver& resolver, ResolverId resolverId)
{
    const ChunkCoord chunkX = x >> world::CHUNK_SHIFT;
    const ChunkCoord chunkZ = z >> world::CHUNK_SHIFT;
    const i32 localX = x & world::CHUNK_MASK;
    const i32 localZ = z & world::CHUNK_MASK;

    return m_cache.getOrCompute(chunkX, chunkZ, localX, localZ, static_cast<size_t>(resolverId), [&]() {
        if (m_blendRadius == 0) {
            return _getColorDirect(accessor, x, y, z, resolver, resolverId);
        }
        return _getColorBlended(accessor, x, y, z, resolver, resolverId);
    });
}

u32 BiomeColorBlender::_getColorDirect(
    const IBiomeAccessor& accessor, i32 x, i32 y, i32 z, const ColorResolver& resolver, ResolverId resolverId)
{
    const Biome* biome = accessor.getBiome(x, y, z);
    if (!biome) {
        // 默认颜色（白色，不进行着色）
        return 0xFFFFFFFF;
    }

    // 首先尝试获取覆盖颜色
    const u32 color = resolver.getColor(*biome, static_cast<f64>(x), static_cast<f64>(z));
    if (color != 0xFFFFFFFF) {
        return color;
    }

    // 需要从 colormap 获取
    const std::array<u32, 65536>* colorMap = _getColorMap(resolverId);
    if (colorMap) {
        const f32 temperature = std::clamp(biome->climate().temperature, 0.0f, 1.0f);
        const f32 humidity = std::clamp(biome->climate().humidity, 0.0f, 1.0f) * temperature;
        const i32 tempIndex = static_cast<i32>((1.0f - temperature) * 255.0f);
        const i32 humidityIndex = static_cast<i32>((1.0f - humidity) * 255.0f);
        const i32 colorIndex = (humidityIndex << 8) | tempIndex;
        return (*colorMap)[static_cast<size_t>(colorIndex)];
    }

    // 返回默认颜色
    return _getDefaultColor(resolverId);
}

u32 BiomeColorBlender::_getColorBlended(
    const IBiomeAccessor& accessor, i32 x, i32 y, i32 z, const ColorResolver& resolver, ResolverId resolverId)
{
    const i32 radius = m_blendRadius;
    const i32 startX = x - radius;
    const i32 startZ = z - radius;
    const i32 endX = x + radius;
    const i32 endZ = z + radius;

    // 清空工作缓冲区
    m_colorBuffer.clear();

    // 获取 colormap 和默认颜色
    const std::array<u32, 65536>* colorMap = _getColorMap(resolverId);
    const u32 defaultColor = _getDefaultColor(resolverId);

    // 采样区域内的颜色
    for (i32 sampleX = startX; sampleX <= endX; ++sampleX) {
        for (i32 sampleZ = startZ; sampleZ <= endZ; ++sampleZ) {
            const Biome* biome = accessor.getBiome(sampleX, y, sampleZ);
            if (biome) {
                // 使用带 colormap 支持的颜色获取
                const u32 color = resolver.getColorWithColorMap(
                    *biome, static_cast<f64>(sampleX), static_cast<f64>(sampleZ), colorMap, defaultColor);
                m_colorBuffer.push_back(color);
            }
        }
    }

    if (m_colorBuffer.empty()) {
        return defaultColor;
    }

    // 计算平均颜色
    return averageColors(m_colorBuffer.data(), m_colorBuffer.size());
}

u32 BiomeColorBlender::averageColors(const u32* colors, size_t count) noexcept
{
    if (count == 0) {
        return 0xFFFFFFFF;
    }

    if (count == 1) {
        return colors[0];
    }

    // 分别累加 RGB 分量
    u32 r = 0, g = 0, b = 0;

    for (size_t i = 0; i < count; ++i) {
        const u32 color = colors[i];
        r += (color >> 16) & 0xFF;
        g += (color >> 8) & 0xFF;
        b += color & 0xFF;
    }

    // 计算平均值
    r /= static_cast<u32>(count);
    g /= static_cast<u32>(count);
    b /= static_cast<u32>(count);

    // 组合成 RGB 颜色
    return (r << 16) | (g << 8) | b;
}

u32 BiomeColorBlender::_getDefaultColor(ResolverId resolverId) noexcept
{
    switch (resolverId) {
        case ResolverId::Grass:
            return 0x91BD59; // 默认平原草色
        case ResolverId::Foliage:
            return 0x48B518; // FoliageColors.getDefault()
        case ResolverId::Water:
            return 0x3F76E4; // 默认水颜色
        case ResolverId::DryFoliage:
            return 0x5C3C32; // DryFoliageColor.FOLIAGE_DRY_DEFAULT
        default:
            return 0xFFFFFF;
    }
}

const std::array<u32, 65536>* BiomeColorBlender::_getColorMap(ResolverId resolverId) const noexcept
{
    switch (resolverId) {
        case ResolverId::Grass:
            return m_grassColorMap;
        case ResolverId::Foliage:
            return m_foliageColorMap;
        case ResolverId::Water:
            return nullptr; // 水不需要 colormap
        case ResolverId::DryFoliage:
            return m_dryFoliageColorMap;
        default:
            return nullptr;
    }
}

BiomeColorBlender::ResolverId BiomeColorBlender::getResolverId(const ColorResolver& resolver)
{
    // 通过类型识别解析器
    // 注意：这里使用动态类型识别，实际使用时应该直接传递 ResolverId
    if (dynamic_cast<const GrassColorResolver*>(&resolver)) {
        return ResolverId::Grass;
    }
    if (dynamic_cast<const FoliageColorResolver*>(&resolver)) {
        return ResolverId::Foliage;
    }
    if (dynamic_cast<const WaterColorResolver*>(&resolver)) {
        return ResolverId::Water;
    }
    if (dynamic_cast<const DryFoliageColorResolver*>(&resolver)) {
        return ResolverId::DryFoliage;
    }

    // 默认返回草颜色
    return ResolverId::Grass;
}

} // namespace mc::client
