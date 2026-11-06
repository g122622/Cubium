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
 * mth_map_lerp.ts — `Mth.lerpInt` / `Mth.lerpDiscrete` / `Mth.binarySearch` 的输入构造。
 *
 * 三个方法都做「按比例取整数」，但边界处理各不相同，正是最容易写错的地方：
 *
 * - `lerpInt(t, from, to)` = `from + floor(t * (to - from))`，`t = 1` 落在 `to`；
 * - `lerpDiscrete(t, from, to)` = `from + floor(t * (to - from - 1)) + (t > 0 ? 1 : 0)`，
 *   **乘的是 `to - from - 1`**，再对正值补一格；`t = 0` 不补；
 * - `binarySearch(from, to, predicate)` 在区间上找第一个满足单调谓词的整数。
 *
 * 单元格式：`<mode>|<参数…>`，参数以十进制字符串进入。
 */

/** 该能力的一个单元。 */
export interface MthMapLerpUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 所有数值以十进制字符串进入，避免 JSON 解析阶段引入浮点表示差异。
 */
export function mthMapLerpInput(unit: string): MthMapLerpUnit {
    return { unit };
}
