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
 * mth_out_from_origin.ts — `Mth.outFromOrigin` 的输入构造。
 *
 * 从原点向外交替扩展的整数序列：先上、再下、再上……直到两侧都越界。
 *
 * **一个反直觉之处**：`hasNext` 只拦「两个方向都越界」，**不检查 `next` 本身是否合法**，
 * 因此序列末尾可能出现一个越界的值（例如 `(0, -2, 2, 1)` 得 `0 1 -1 2 -2 3`）。
 * 这不是 bug，是原版 `IntStream.iterate(seed, hasNext, next)` 的求值顺序决定的：
 * 判断的是「上一个值还有没有后继」，而不是「下一个值在不在界内」。
 *
 * 单元格式：`<origin>|<lower>|<upper>|<step>`。
 */

/** 该能力的一个单元。 */
export interface MthOutFromOriginUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function mthOutFromOriginInput(unit: string): MthOutFromOriginUnit {
    return { unit };
}
