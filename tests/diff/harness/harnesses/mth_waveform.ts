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
 * mth_waveform.ts — 波形与缓动函数能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 两组关键用例：
 * - `tri|-3|10`：`triangleWave` 内部用 Java 取模（向零截断），负输入会产生**超出 [-1,1]**
 *   的值（`2.2`）。
 * - `smooth|-0.5` 与 `smooth|2`：`smoothstep` **不 clamp**，区间外继续外推。
 */

/** 波形能力的一个单元。 */
export interface MthWaveformUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `triangleWave` / `square` / `cube` 是 `float` 运算，`smoothstep` 系列是 `double`。
 * 统一按 `%.9g` 格式化后去尾零。
 */
export function mthWaveformInput(unit: string): MthWaveformUnit {
    return { unit };
}