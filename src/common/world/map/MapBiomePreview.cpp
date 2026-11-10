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

#include "common/world/map/MapBiomePreview.hpp"

#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/map/MapData.hpp"
#include "common/world/map/MaterialColor.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace mc::world::map {

namespace {

constexpr i32 PREVIEW_SIZE = MapData::MAP_SIZE;

// 阴影档位：0 = 最暗，1 = 偏暗，2 = 正常，3 = 偏亮（与 MaterialColor 的 shade 顺序一致）
constexpr u32 SHADE_LOWEST = 0;
constexpr u32 SHADE_LOW = 1;
constexpr u32 SHADE_NORMAL = 2;
constexpr u32 SHADE_HIGH = 3;

} // namespace

void renderBiomePreviewMap(IWorld& world, MapData& mapData)
{
    // 每个像素代表 scale × scale 格；采样点坐标要乘回 scale，再相对地图中心偏移 64 像素。
    const i32 scale = 1 << mapData.scale();
    const i32 originX = mapData.xCenter() / scale - 64;
    const i32 originZ = mapData.zCenter() / scale - 64;

    // 先采样一遍"哪些像素落在水体生物群系上"。九宫格判定要读到相邻像素，
    // 因此这里存满 128×128 而不是只存内部 126×126。
    std::array<bool, static_cast<size_t>(PREVIEW_SIZE) * static_cast<size_t>(PREVIEW_SIZE)> watery{};
    for (i32 row = 0; row < PREVIEW_SIZE; ++row) {
        for (i32 column = 0; column < PREVIEW_SIZE; ++column) {
            const BiomeId biome = world.getBiome((originX + column) * scale, 0, (originZ + row) * scale);
            watery[static_cast<size_t>(row * PREVIEW_SIZE + column)] =
                BiomeTags::WATER_ON_MAP_OUTLINES().contains(biome);
        }
    }

    const auto isWatery = [&watery](i32 row, i32 column) {
        return watery[static_cast<size_t>(row * PREVIEW_SIZE + column)];
    };

    // 逐像素上色：最外圈留空，内部按九宫格里"是水的邻居数量"决定颜色与明暗——
    // 水体用橙色并在深处画正弦错位的横条纹（水面反光），岸边陆地按离水远近在棕色系里分档。
    for (i32 row = 1; row < PREVIEW_SIZE - 1; ++row) {
        for (i32 column = 1; column < PREVIEW_SIZE - 1; ++column) {
            i32 neighborWaterCount = 0;
            for (i32 rowOffset = -1; rowOffset < 2; ++rowOffset) {
                for (i32 columnOffset = -1; columnOffset < 2; ++columnOffset) {
                    if ((rowOffset != 0 || columnOffset != 0) && isWatery(row + rowOffset, column + columnOffset)) {
                        ++neighborWaterCount;
                    }
                }
            }

            u32 shade = SHADE_LOWEST;
            MaterialColorId color = MaterialColorId::AIR;
            if (isWatery(row, column)) {
                color = MaterialColorId::ADOBE;
                if (neighborWaterCount > 7 && column % 2 == 0) {
                    const i32 band = (row + static_cast<i32>(std::sin(static_cast<f32>(column)) * 7.0f)) / 8 % 5;
                    if (band == 0 || band == 4) {
                        shade = SHADE_LOW;
                    } else if (band == 1 || band == 3) {
                        shade = SHADE_NORMAL;
                    } else {
                        shade = SHADE_HIGH;
                    }
                } else if (neighborWaterCount > 7) {
                    // 深处的水不画条纹，保持无色（与原版一致：颜色为 NONE 时不写像素）。
                    color = MaterialColorId::AIR;
                } else if (neighborWaterCount > 5) {
                    shade = SHADE_NORMAL;
                } else if (neighborWaterCount > 3) {
                    shade = SHADE_LOW;
                } else if (neighborWaterCount > 1) {
                    shade = SHADE_LOW;
                }
            } else if (neighborWaterCount > 0) {
                color = MaterialColorId::BROWN;
                shade = neighborWaterCount > 3 ? SHADE_NORMAL : SHADE_LOWEST;
            }

            if (color != MaterialColorId::AIR) {
                mapData.setColor(column, row, static_cast<u8>(static_cast<u8>(color) * 4 + shade));
            }
        }
    }
}

} // namespace mc::world::map
