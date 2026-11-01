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
 * aabb_ops.ts — `AABB` 几何运算能力的输入构造。
 *
 * 单元形如：
 * - `<box>|inflate|dx|dy|dz>`
 * - `<box>|<box2>|intersect`（或 `minmax` / `intersects`）
 * 其中 `<box>` 是 `minX,minY,minZ,maxX,maxY,maxZ`，由探针与 oracle 的 `list` 各自枚举。
 */

/** AABB 几何运算的一个单元。 */
export interface AabbOpsUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 整行作为一个字符串字段传递：单元既含 `|` 又含 `,` 分隔，原样传能让两侧用同一段解析
 * 逻辑；浮点坐标也以十进制字符串形式进入，避免 JSON 解析阶段引入差异。
 */
export function aabbOpsInput(unit: string): AabbOpsUnit {
    return { unit };
}