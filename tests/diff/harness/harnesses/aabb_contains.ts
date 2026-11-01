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
 * aabb_contains.ts — 包围盒包含判定能力的输入构造。
 *
 * 单元形如 `<box>|<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * `0,0,0,1,1,1|contains|0|0|0` 与 `0,0,0,1,1,1|contains|1|1|1` 是本能力的核心对比：
 * 原版 `contains` 是**半开区间** `[min, max)`，`min` 边界算在盒内，`max` 边界不算。
 */

/** 包围盒包含判定能力的一个单元。 */
export interface AabbContainsUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 浮点按 `%.6g` 格式化后去掉尾随零，与其它浮点能力的契约一致。
 */
export function aabbContainsInput(unit: string): AabbContainsUnit {
    return { unit };
}