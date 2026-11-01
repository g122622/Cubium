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
 * style_tristate.ts — `Style` 三态格式标志序列化的输入构造。
 *
 * **探针调用 Cubium 的真实实现**：`mc::text::Style` 的 `setBold` / `setItalic` 等
 * 与 `toJson()`。
 *
 * 三组关键用例：
 * - `set|bold|false`：**显式 false 必须写出 `{"bold":false}`**。
 *   若 Cubium 用 `if (*value)` 判断再写，会静默丢弃——这正是本能力要抓的缺陷。
 * - `set|bold|unset`：**不输出键**（`{}`）。
 * - `asBool|bold|unset`：`isXxx()` 返回 `false`，与原版 `isBold()` 一致
 *   （原版三态字段不带公开访问器）。
 */

/** `Style` 三态能力的一个单元。 */
export interface StyleTristateUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `toJson` 类单元输出紧凑 JSON 文本；`asBool` 输出 `"true"` / `"false"`。
 */
export function styleTristateInput(unit: string): StyleTristateUnit {
    return { unit };
}