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
 * mth_round_ext.ts — `Mth.ceilLong` 与 `Mth.map` 的输入构造。
 *
 * | 方法 | 语义 |
 * |---|---|
 * | `ceilLong(double)` | 截断后比较再决定是否 +1 |
 * | `map(v, inMin, inMax, outMin, outMax)` | 线性映射，等价于 `lerp(inverseLerp(...))` |
 *
 * `ceilLong` 的重点是**恰好整数**与**负数**：整数不加 1，负数靠截断方向决定结果。
 *
 * 单元格式：`<mode>|<args...>`。
 */

/** 该能力的一个单元。 */
export interface MthRoundExtUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function mthRoundExtInput(unit: string): MthRoundExtUnit {
    return { unit };
}