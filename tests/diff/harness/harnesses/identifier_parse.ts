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
 * identifier_parse.ts — 标识符解析能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `ns|foo/bar`：**命名空间不允许 `/`**，而路径允许——两者白名单只差这一个字符。
 * - `parse|foo:`：**空路径是合法的**，得到 `minecraft` 前缀下的空路径。
 * - `parse|foo:bar:baz`：`bySeparator` 只看**第一个** `:`，路径变成 `bar:baz`（含 `:` 非法）。
 */

/** 标识符解析能力的一个单元。 */
export interface IdentifierParseUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 非法输入（`parse` 抛异常、`tryParse` 返回 `null`）统一输出字符串 `"<invalid>"`——两侧一致，
 * 避免 oracle 因异常退出。
 */
export function identifierParseInput(unit: string): IdentifierParseUnit {
    return { unit };
}