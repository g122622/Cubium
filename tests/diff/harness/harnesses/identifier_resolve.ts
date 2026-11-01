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
 * identifier_resolve.ts — `Identifier` 三种构造入口的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `parse|:stone` 与 `default|:stone`：**首字符冒号时 `parse` 补默认命名空间，
 *   但 `withDefaultNamespace` 抛异常**（`:` 不是合法路径字符）。
 * - `default|foo:bar`：**`withDefaultNamespace` 不解析 `:`**，整个串当路径 → 抛异常。
 * - `parse|a:b:c`：**按第一个冒号拆分**，路径 `b:c` 含冒号 → 抛异常。
 */

/** `Identifier` 能力的一个单元。 */
export interface IdentifierResolveUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 成功输出 `namespace:path`；`parse` / `default` 失败输出 `!IdentifierException`；
 * `tryParse` 失败输出 `null`。
 */
export function identifierResolveInput(unit: string): IdentifierResolveUnit {
    return { unit };
}