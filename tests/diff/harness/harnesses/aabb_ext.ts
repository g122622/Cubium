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
 * aabb_ext.ts — `AABB` 的中心点、角点与向上包含。
 *
 * | 方法 | 语义 |
 * |---|---|
 * | `getCenter()` | 三轴中点 |
 * | `getBottomCenter()` | X/Z 中点，Y 取 `minY` |
 * | `getMinPosition()` / `getMaxPosition()` | 最小 / 最大角点 |
 * | `include(point)` | 原地扩展盒子以包含该点 |
 *
 * `getBottomCenter` 与 `getCenter` 的差别只在 Y：
 * 前者给 `minY`，后者给中点。单元要用 Y 方向有厚度的盒子才能区分两者。
 *
 * 单元格式：`<mode>|<box>|<arg>`，盒子用 `x1,y1,z1,x2,y2,z2` 表示。
 */

/** 该能力的一个单元。 */
export interface AabbExtUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function aabbExtInput(unit: string): AabbExtUnit {
    return { unit };
}