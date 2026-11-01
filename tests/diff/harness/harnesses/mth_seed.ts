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
 * mth_seed.ts — 方块坐标哈希种子能力的输入构造。
 *
 * 单元形如 `<模式>|<x>|<y>|<z>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `seed|2147483647|0|0`：`x * 3129871` 是 **`int` 乘法**，在 `Integer.MAX_VALUE` 处会溢出回绕；
 *   而 `z * 116129781L` 是 `long` 乘法。两者的位宽不对称。
 * - `seed|5|3|7` 对比 `seed|7|3|5`：交换 `x` 与 `z` 结果不同（系数不同）。
 * - `vec|5|3|7` 与 `seed|5|3|7`：`Vec3i` 重载应与三参重载**完全一致**。
 */

/** 坐标种子能力的一个单元。 */
export interface MthSeedUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 输出是**有符号 64 位整数**，按**字符串**给出——JSON 数字在 JavaScript 中会因超过
 * `Number.MAX_SAFE_INTEGER` 而丢失精度，因此必须用字符串。
 */
export function mthSeedInput(unit: string): MthSeedUnit {
    return { unit };
}