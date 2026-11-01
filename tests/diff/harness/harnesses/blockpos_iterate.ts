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
 * blockpos_iterate.ts — 坐标区间遍历顺序的输入构造。
 *
 * 单元形如 `<模式>|<ax>|<ay>|<az>|<bx>|<by>|<bz>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `iter|0|0|0|2|1|1`：**迭代顺序是 x 最快、z 最慢**（`(0,0,0) (1,0,0) (2,0,0) (0,1,0) ...`）。
 * - `iter|5|5|5|0|0|0`：**六参版本不归一化**，`min > max` 时跨度为负 → **一个元素都不产生**。
 * - `pair|2|1|1|0|0|0`：两参版本**自动归一化**端点顺序，结果与 `pair|0|0|0|2|1|1` 相同。
 */

/** 坐标遍历能力的一个单元。 */
export interface BlockPosIterateUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 输出格式：`x,y,z` 用 `;` 连接；空结果输出空串。
 */
export function blockPosIterateInput(unit: string): BlockPosIterateUnit {
    return { unit };
}