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
 * mth_length_equal.ts — `Mth.length` / `Mth.lengthSquared` / `Mth.equal` 的输入构造。
 *
 * 三个方法都只有一行算术，但都藏着一个「看起来等价、实际不等价」的陷阱：
 *
 * - `length(float, float)` 全程 f32：平方和用 f32 算完才开方，**不提升到 f64**；
 * - `length(double, double)` 全程 f64，与上一行的结果不同；
 * - `equal(double, double)` 的阈值是 **`1.0E-5F`（f32 常量）** 而不是 `1.0E-5`：
 *   `1.0e-5f` 升为 double 是 `9.999999747378752e-06`，比 `1e-5` 略小，
 *   所以在 `1e-5` 附近的差值上，用哪个常量会给出相反的结论。
 *
 * 单元格式：`<mode>|<参数…>`，参数以十进制字符串进入。
 */

/** 该能力的一个单元。 */
export interface MthLengthEqualUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function mthLengthEqualInput(unit: string): MthLengthEqualUnit {
    return { unit };
}
