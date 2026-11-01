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
 * text_color.ts — `TextColor` 解析与序列化的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `parse|#f00`：**不是 RGB 缩写**，而是十六进制 `0x000F00`（暗绿）——暗藏陷阱。
 * - `parse|#1000000`：超出 `0xFFFFFF` → 报错；`#ffffff` 是上界。
 * - `parse|red` 与 `match|red|#ff5555`：**具名色与十六进制值相等**，但 `serialize` 输出不同。
 */

/** `TextColor` 能力的一个单元。 */
export interface TextColorUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 输出 `"<value 6 位大写十六进制>|<serialize>"`；解析失败输出 `"<error>"`。
 */
export function textColorInput(unit: string): TextColorUnit {
    return { unit };
}