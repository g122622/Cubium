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

#include "BiomeColorBlender.hpp"
#include "BiomeColorCache.hpp"
#include "ChunkBiomeAccessor.hpp"
