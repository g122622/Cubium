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
 * mth_inverse_lerp.ts — 反向插值与三次插值能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 两组关键用例：
 * - `invD|5|0|0` 与 `invD|0|0|0`：退化区间 `from == to`，原版不做保护，产生
 *   `Infinity` / `NaN`。
 * - `lerp3|0.5|0.5|0|0|0|0|10|10|10|10|0` 与 `...|999`：**第 11 个参数 `d11` 在原版实现里
 *   完全未被使用**，两者结果必须相同。
 */

/** 反向插值能力的一个单元。 */
export interface MthInverseLerpUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 浮点按 `%.9g` 格式化后去尾零；`NaN` / `Infinity` / `-Infinity` 以字符串形式给出。
 */
export function mthInverseLerpInput(unit: string): MthInverseLerpUnit {
    return { unit };
}