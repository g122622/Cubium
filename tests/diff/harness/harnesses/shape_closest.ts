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
 * shape_closest.ts — `VoxelShape` 的最近点与整平包裹盒的输入构造。
 *
 * 两个方法：
 *   - `closestPointTo(point)`：形状上离给定点最近的位置，点在形状内部时返回点自身；
 *   - `singleEncompassing()`：把整个形状包成一个最小轴对齐盒。
 *
 * 单元格式：`<mode>|<x1>,<y1>,<z1>,<x2>,<y2>,<z2>|<px>,<py>,<pz>`。
 */

/** 该能力的一个单元。 */
export interface ShapeClosestUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function shapeClosestInput(unit: string): ShapeClosestUnit {
    return { unit };
}