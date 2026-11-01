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
 * aabb_factory.ts — AABB 工厂方法与基础实例方法的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `encap|0|0|0|0|0|0`：**`+1` 只加在 max 一侧**，单方块得到 `(0,0,0)..(1,1,1)`。
 * - `intersect|0|0|0|1|1|1|5|5|5|6|6|6`：**不相交时返回负尺寸盒**，原版不返回 `null`。
 * - `hasNaN|nan|0|0|1|1|1`：`Double.isNaN` 检查。
 */

/** AABB 工厂能力的一个单元。 */
export interface AabbFactoryUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 浮点用 `%.9g` 格式化，`nan` / `inf` 统一小写。
 */
export function aabbFactoryInput(unit: string): AabbFactoryUnit {
    return { unit };
}