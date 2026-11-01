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
 * string_util.ts — 字符串工具能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `trunc|abcdefgh|3|true`：**`maxLength > 3` 才加省略号**，`maxLength = 3` 时直接截断。
 * - `lines|a\nb`：`LINE_PATTERN` 只匹配 `\r\n` 与 `\v`，**单独的 `\n` 不换行**，因此得 1。
 * - `strip|\u00A7zhello`：`\u00A7z` 不在 `[0-9A-FK-OR]` 里，**不会被剥离**。
 */

/** 字符串工具能力的一个单元。 */
export interface StringUtilUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 文本中的控制字符用 `\n` / `\r` / `\v` / `\t` / `\uXXXX` 两字符转义表示，探针与 oracle 各自解码。
 * 输出文本由 Gson 与 nlohmann 各自的 JSON 转义产生，两侧一致。
 */
export function stringUtilInput(unit: string): StringUtilUnit {
    return { unit };
}