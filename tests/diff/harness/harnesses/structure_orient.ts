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
 * structure_orient.ts — `BoundingBox.orientBox` 的输入构造。
 *
 * 单元形如 `<x>|<y>|<z>|<offsetX>|<offsetY>|<offsetZ>|<sizeX>|<sizeY>|<sizeZ>|<direction>`。
 *
 * 三组关键用例：
 * - `0|0|0|0|0|0|3|2|4|up`：**`UP` / `DOWN` 走 `default` 分支**，结果与 `SOUTH` 相同。
 * - `0|0|0|0|0|0|3|2|4|west`：**`WEST`/`EAST` 交换 X 与 Z 的角色**（`minX` 用 `sizeZ`）。
 * - `0|0|0|1|1|1|2|2|2|west`：**正方体尺寸不掩盖 X/Z 交换**——用非立方尺寸才能暴露错误。
 */

/** 结构朝向能力的一个单元。 */
export interface StructureOrientUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 方向取 `down`/`up`/`north`/`south`/`west`/`east`；
 * 输出为 `minX,minY,minZ,maxX,maxY,maxZ`。
 */
export function structureOrientInput(unit: string): StructureOrientUnit {
    return { unit };
}