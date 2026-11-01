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
 * hover_event.ts — `HoverEvent.Action` 能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `allowed|SHOW_TEXT`：**三个动作全部 `allowFromServer = true`**（与 `click_event` 对照）。
 * - `filter|SHOW_TEXT`：因此 `filterForSerialization` **从不失败**。
 * - `tostring|SHOW_TEXT`：输出 `<action show_text>`，**不是枚举名**。
 */

/** `HoverEvent` 能力的一个单元。 */
export interface HoverEventUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `action` 用枚举名（如 `SHOW_TEXT`）；`names` 输出全部序列化名。
 */
export function hoverEventInput(unit: string): HoverEventUnit {
    return { unit };
}