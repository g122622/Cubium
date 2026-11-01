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
 * mth_degrees.ts — 角度差与朝向平滑能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `diff|170|-170`：`wrapDegrees(-340) = 20`——从 170° 到 -170° 只需转 20°。
 * - `rotate|170|-170|5`：**原版从 `target` 反推**，结果是 `-175` 而不是直觉的 `175`。
 *   两者角度等价（差 360）但数值不同，会写进实体数据。
 * - `absMaxI|-2147483648|0`：`Math.abs(Integer.MIN_VALUE)` 仍是负数，**溢出**。
 */

/** 角度能力的一个单元。 */
export interface MthDegreesUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 角度类结果是 `float`，按 `%.9g` 格式化后去尾零；`absMaxI` 与 `sign` 输出整数。
 */
export function mthDegreesInput(unit: string): MthDegreesUnit {
    return { unit };
}