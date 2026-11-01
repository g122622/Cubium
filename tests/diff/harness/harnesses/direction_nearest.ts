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
 * direction_nearest.ts — 由向量反推方向能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `approxF|0|0|0`：**全零向量返回 `north`**——原版初值是 `Float.MIN_VALUE`（最小正数）且用
 *   严格 `>`，所有点积为 0 时循环一次都不更新。
 * - `approxF|1|1|0`：`up` 与 `east` 点积都是 1，**枚举顺序决定 `up` 胜出**。
 * - `nearest|1|1|1|north`：三轴平局，**回退到 `fallback`** 而不是猜测。
 */

/** 方向反推能力的一个单元。 */
export interface DirectionNearestUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 方向输出为小写枚举名（`down`/`up`/`north`/`south`/`west`/`east`），
 * `getNearest` 的 `null` 回退输出字符串 `"null"`。
 */
export function directionNearestInput(unit: string): DirectionNearestUnit {
    return { unit };
}