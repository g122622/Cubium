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
 * mth_rounding.ts — `Mth.floor` / `Mth.ceil` 取整能力的输入构造。
 *
 * 单元就是待取整的十进制数值字符串，由探针与 oracle 的 `list` 各自枚举同一张表。
 */

/** 取整能力的一个单元。 */
export interface MthRoundingUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 数值以十进制字符串传递而非 JSON number：`3000000000` 超出 int 范围，按字符串传递能让
 * 两侧都用各自的字符串解析路径处理，避免 JSON 解析阶段就引入差异。
 */
export function mthRoundingInput(unit: string): MthRoundingUnit {
    return { unit };
}