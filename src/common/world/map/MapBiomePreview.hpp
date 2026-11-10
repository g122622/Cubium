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

namespace mc {

class IWorld;

namespace world::map {

class MapData;

/**
 * @brief 按生物群系填涂地图的预览底图
 *
 * 探险地图（林地府邸/海底神殿/埋藏的宝藏等）在拿到手时就带一张"地形预览"：按
 * 128×128 采样生物群系，水体画成橙色、陆地画成棕色系，并在水陆交界处画明暗过渡，
 * 让玩家一眼看出目标周围是海还是陆。
 *
 * 采样只依赖 `BiomeTags::WATER_ON_MAP_OUTLINES` 标签，与地形高度、方块种类无关，
 * 因此不需要读方块或高度图——地图还没被"画"过也能先有预览。
 *
 * 这是地图取色链路的纯函数部分，从 `FilledMapItem` 中抽出以便单元测试直接覆盖
 * （测试只需构造一个 `MapData` 和一个 `getBiome` 可控的世界桩）。
 *
 * @param world 世界（只用于取样生物群系）
 * @param mapData 目标地图数据（中心与缩放必须已初始化）
 */
void renderBiomePreviewMap(IWorld& world, MapData& mapData);

} // namespace world::map
} // namespace mc
