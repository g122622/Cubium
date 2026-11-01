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
 * identifier_path.ts — 标识符字符串派生能力的输入构造。
 *
 * 单元形如 `<模式>|<namespace>|<path>[|<extra>]`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `lang|minecraft|block/stone`：`toLanguageKey()` 只把 `:` 换成 `.`，**路径里的 `/` 保留**。
 * - `debugFile|foo|bar/baz.txt`：`/` 与 `:` 换成 `_`，但 **`.` 保留**。
 * - `withPrefix|minecraft|stone|Block/`：**构造后重新校验失败**，输出 `<invalid>`。
 */

/** 标识符派生能力的一个单元。 */
export interface IdentifierPathUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 构造失败（命名空间或路径非法）统一输出字符串 `"<invalid>"`——两侧一致，避免 oracle 抛异常。
 */
export function identifierPathInput(unit: string): IdentifierPathUnit {
    return { unit };
}