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
 * mth_sqrt.ts — 平方根相关能力的输入构造。
 *
 * 单元形如 `<模式>|<参数>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `fast|4`：**`fastInvSqrt` 是近似算法**，一次牛顿迭代后相对误差约 `1e-4`，不精确等于 `0.5`。
 * - `fast|0`：**没有特殊值保护**，返回约 `2.5e153` 而不是 `Infinity`。
 * - `sqrtF|-1`：`(float) Math.sqrt(-1)` 得 `NaN`。
 */

/** 平方根能力的一个单元。 */
export interface MthSqrtUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 全部按 `%.9g` 格式化后去尾零，**尾数与指数分开处理**——否则 `1e+100` 这类值会因为一侧保留
 * 尾零而报假差异（见 [`mth_frac`](./mth_frac.md) 的踩坑记录）。
 */
export function mthSqrtInput(unit: string): MthSqrtUnit {
    return { unit };
}