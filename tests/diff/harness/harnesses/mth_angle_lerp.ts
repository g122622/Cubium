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
 * mth_angle_lerp.ts — 角度插值与样条能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `wrap|180`：`wrapDegrees` 输出区间是 `(-180, 180]`，180 被绕成 -180。
 * - `rotLerp|0.5|170|-170`：**走短路**，结果是 `180` 而非 `0`。这是角度插值的核心语义。
 * - `catmull|0.5|0|0|1|1`：**过冲**配置，样条值超出 `[p1, p2] = [0, 1]`。
 */

/** 角度插值能力的一个单元。 */
export interface MthAngleLerpUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 浮点按 `%.9g` 格式化后去尾零；`NaN` / `Infinity` / `-Infinity` 以字符串形式给出。
 * 本能力全部是 `float` 运算，必须用 `%.9g`——`%.6g` 会掩盖 `float` 末位差异。
 */
export function mthAngleLerpInput(unit: string): MthAngleLerpUnit {
    return { unit };
}