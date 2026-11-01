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
 * mth_absmax.ts — `Mth.absMax` / `chessboardDistance` / `smallestSquareSide` 的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `absMaxInt|-2147483648|5`：**`Math.abs(Integer.MIN_VALUE)` 溢出为负**，结果取 `5`。
 * - `chess|0|0|2147483647|-1`：**坐标差溢出回绕**（Java `int` 静默 wrap）。
 * - `square|0`：**`0` 返回 `0`**，负数抛异常。
 */

/** `Mth.absMax` 能力的一个单元。 */
export interface MthAbsMaxUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `absMaxInt` / `chess` / `square` 输出整数；`absMaxFloat` / `absMaxDouble` 输出 `%.9g` 浮点；
 * `square` 对负数输出 `!IllegalArgumentException`。
 */
export function mthAbsMaxInput(unit: string): MthAbsMaxUnit {
    return { unit };
}