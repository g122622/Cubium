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
 * mth_atan2.ts — `Mth.atan2` 查表近似实现的输入构造。
 *
 * 原版 `Mth.atan2` 不是 `Math.atan2`：它用 `fastInvSqrt` 归一化，再用
 * `double` 位模式当表下标查 `asin`/`cos` 表，最后加一个三次修正项。
 * 结果与标准库相差 1e-6 量级，**逐位不同**。
 *
 * 单元格式：`<y>|<x>`，两个分量都以十进制字符串进入，
 * 避免 JSON 解析阶段引入浮点表示差异。
 */

/** 反正切能力的一个单元。 */
export interface MthAtan2Unit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `y` 与 `x` 以十进制字符串进入，避免 JSON 解析阶段引入浮点表示差异。
 */
export function mthAtan2Input(unit: string): MthAtan2Unit {
    return { unit };
}
