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
 * mth_pack_degrees.ts — 角度单字节压缩能力的输入构造。
 *
 * 单元形如 `<模式>|<参数>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `pack|180`：`180 × 256 / 360 = 128`，强转 `(byte)` 后**回绕成 `-128`**。`packDegrees`
 *   **不做 clamp**。
 * - `pack|-0.5`：`Mth.floor` 与 `(int)` 在负小数上分歧（前者向负无穷，后者向零）。
 * - `round|180` 与 `round|-180`：两者打包后**都是 `-128`**，因此往返后结果相同。
 */

/** 角度压缩能力的一个单元。 */
export interface MthPackDegreesUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `pack` 输出有符号十进制整数（`-128..127`）；`unpack` / `round` 输出按 `%.9g` 格式化并去尾零的
 * `float` 字符串（`unpack` 的中间运算是 `int` 乘法，必须用 `%.9g` 才能暴露 `float` 末位差异）。
 */
export function mthPackDegreesInput(unit: string): MthPackDegreesUnit {
    return { unit };
}