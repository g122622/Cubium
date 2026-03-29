/**
 * @file blend.hpp
 * @brief 生物群系颜色混合模块
 *
 * 提供生物群系边界处颜色平滑过渡的功能。
 *
 * ## 架构概述
 *
 * ```
 * BiomeColorBlender (颜色混合器)
 *     │
 *     ├── IBiomeAccessor (生物群系访问接口)
 *     │       │
 *     │       └── ChunkBiomeAccessor (区块生物群系访问器)
 *     │               └── 从 ChunkData 和邻居区块获取生物群系
 *     │
 *     ├── BiomeColorCache (颜色缓存)
 *     │       └── 按区块缓存计算结果，避免重复计算
 *     │
 *     └── ColorResolver (颜色解析器)
 *             ├── GrassColorResolver
 *             ├── FoliageColorResolver
 *             └── WaterColorResolver
 *
 * ```
 *
 * ## 使用示例
 *
 * @code
 * // 创建混合器
 * BiomeColorBlender blender;
 * blender.setBlendRadius(2);  // 5x5 混合区域
 *
 * // 创建访问器
 * std::array<const ChunkData*, 4> neighbors = {west, east, north, south};
 * ChunkBiomeAccessor accessor(chunk, neighbors, chunkX, chunkZ);
 *
 * // 获取混合后的草颜色
 * u32 grassColor = blender.getBlendedColor(
 *     accessor, x, y, z,
 *     BiomeColors::grassColorResolver(),
 *     BiomeColorBlender::ResolverId::Grass
 * );
 * @endcode
 *
 * ## 参考
 *
 * MC 1.16.5 ClientWorld.getBlockColorRaw
 * MC 1.16.5 ColorCache
 */

#pragma once

#include "BiomeColorCache.hpp"
#include "BiomeColorBlender.hpp"
#include "ChunkBiomeAccessor.hpp"
