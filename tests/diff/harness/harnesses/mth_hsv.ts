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
 * mth_hsv.ts — HSV 颜色转换能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 两组关键用例：
 * - `rgb|1|1|1` 与 `rgb|0|1|1`：**`hue = 1.0` 与 `hue = 0.0` 完全等价**（`(int)(6.0) % 6 = 0`）。
 * - `argb|0|1|1|255`：alpha 为 `255` 时最高位为 1，结果是**负的** 32 位整数。
 */

/** HSV 转换能力的一个单元。 */
export interface MthHsvUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 输出是**有符号 32 位整数**（与 Java `int` 一致），因此 `hsvToArgb` 在 alpha >= 128 时
 * 会给出负数。探针与 oracle 都按有符号十进制输出。
 */
export function mthHsvInput(unit: string): MthHsvUnit {
    return { unit };
}