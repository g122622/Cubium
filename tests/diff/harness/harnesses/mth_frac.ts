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
 * mth_frac.ts — 小数部分能力的输入构造。
 *
 * 单元形如 `<模式>|<参数>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `fracF|1e10`：`floor` 返回 `int`，`1e10` 饱和到 `Integer.MAX_VALUE`，因此结果**约为
 *   `9999999999` 而不是 `0`**——`frac(float)` 在大输入上完全失效。
 * - `fracF|-1.5`：`floor` 向负无穷取整，结果是 **`0.5`（正）**，不是 `-0.5`。
 * - `fracF|16777217`：`float` 只有 24 位有效数字，`2^24` 以上已是整数，`frac` 得 `0`。
 */

/** 小数部分能力的一个单元。 */
export interface MthFracUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `floorF` 输出 `int`，`floorD` 输出 `long`（**按字符串**给出，避免 JSON 精度问题）；
 * `fracF` / `fracD` 按 `%.9g` 格式化后去尾零。
 */
export function mthFracInput(unit: string): MthFracUnit {
    return { unit };
}