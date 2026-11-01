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
 * vec3i_arith.ts — 整数向量算术语义的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * `manhattan|0,0,0|16777217,0,0` 与 `manhattan|0,0,0|9000000,9000000,9000000` 两组用于暴露
 * 原版 `distManhattan` 内部用 `float` 累加的精度问题——`16777217` 超出 `float` 的 24 位尾数
 * 范围，会被舍入。
 */

/** 整数向量算术能力的一个单元。 */
export interface Vec3iArithUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 坐标是整数三元组，`compare` 输出的是 `compareTo` 的**原始差值**（不是 -1/0/1），
 * 因此可能超出 `[-1, 1]`，也可能因 `int` 溢出而为负。
 */
export function vec3iArithInput(unit: string): Vec3iArithUnit {
    return { unit };
}