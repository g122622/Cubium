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
 * bounding_box_encap.ts — `BoundingBox` 封装族的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `ctor|5|5|5|0|0|0`：**构造器在 min/max 反了时会归一化**（与 `AABB` 的无条件归一化不同）。
 * - `encapPos|-`：**空集合返回 `Optional.empty()`**，输出 `<empty>`，不是全零盒。
 * - `encapPos|0,0,0;3,2,4`：**起始盒是第一个点折叠成的点盒**，不是 `infinite()`。
 */

/** `BoundingBox` 封装能力的一个单元。 */
export interface BoundingBoxEncapUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 空集合输出 `"<empty>"`；整数用 `minint` / `maxint` 表示极值。
 */
export function boundingBoxEncapInput(unit: string): BoundingBoxEncapUnit {
    return { unit };
}