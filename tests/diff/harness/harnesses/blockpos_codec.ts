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
 * blockpos_codec.ts — `BlockPos` 位打包与坐标运算能力的输入构造。
 *
 * 单元形如：
 * - `pack|x|y|z`
 * - `offset|x|y|z|dx|dy|dz`
 *
 * 由探针与 oracle 的 `list` 各自枚举同一张表。
 */

/** BlockPos 编解码能力的一个单元。 */
export interface BlockPosCodecUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 坐标是整数，但打包结果会超出 JavaScript 的安全整数范围（2^53），因此输出侧用字符串
 * 承载 packed 值；输入侧仍是普通整数，无精度问题。
 */
export function blockposCodecInput(unit: string): BlockPosCodecUnit {
    return { unit };
}