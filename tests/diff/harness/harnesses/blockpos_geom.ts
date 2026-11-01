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
 * blockpos_geom.ts — 方块坐标几何变换能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * `containing|-0.5,-1.5,-0.1` 用于暴露 `Mth.floor` 的向负无穷取整——若误用 `(int)` 强转
 * （向零截断），会得到 `0,-1,0` 而非原版的 `-1,-2,-1`。
 */

/** 方块坐标几何变换能力的一个单元。 */
export interface BlockPosGeomUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 输出坐标是整数三元组（逗号分隔），以字符串形式给出，避免任何精度问题。
 */
export function blockPosGeomInput(unit: string): BlockPosGeomUnit {
    return { unit };
}