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
 * argb_ops.ts — ARGB 通道与混合能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 颜色参数支持 `0x` 前缀的十六进制，便于书写带 alpha 的值。三组关键用例：
 * - `ch|-1`：`alpha` 用 `>>>`（无符号右移），`-1` 应得 `255` 而不是 `-1`。
 * - `pack3|255|0|0`：三参 `color` 的 alpha 是 **255**，与 `mth_hsv` 的 alpha=0 相反。
 * - `scaleF|0x00FF8040|0.5` 对比 `scaleI|0x00FF8040|127`：**两种重载结果可能差 1**。
 */

/** ARGB 能力的一个单元。 */
export interface ArgbOpsUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 输出统一为**有符号十进制**整数（与 Java `int` 一致），`ch` 模式输出 `a,r,g,b` 四段。
 */
export function argbOpsInput(unit: string): ArgbOpsUnit {
    return { unit };
}