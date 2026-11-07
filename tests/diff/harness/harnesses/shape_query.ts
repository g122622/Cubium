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
 * shape_query.ts — `VoxelShape` 的查询类接口。
 *
 * 覆盖四个方法：
 *
 * | 方法 | 语义 |
 * |---|---|
 * | `getCoords(axis)` | 该轴上的坐标切分表 |
 * | `findIndex(axis, coord)` | 坐标落在第几个区间（二分查找） |
 * | `isCubeLike()` / `isCubeLikeAlong(axis)` | 该轴是否恰好是 `[0, 1]` 两段 |
 * | `toAabbs()` | 拆成轴对齐盒列表 |
 *
 * 单元格式：`<mode>|<box>|<arg>`，盒子用 `x1,y1,z1,x2,y2,z2` 表示。
 */

/** 该能力的一个单元。 */
export interface ShapeQueryUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function shapeQueryInput(unit: string): ShapeQueryUnit {
    return { unit };
}