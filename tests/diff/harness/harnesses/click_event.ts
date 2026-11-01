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
 * click_event.ts — `ClickEvent.Action` 能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `allowed|OPEN_FILE`：**`OPEN_FILE` 是唯一 `allowFromServer = false` 的动作**。
 * - `filter|OPEN_FILE`：被 `CODEC` 拒绝（但 `UNSAFE_CODEC` 接受）。
 * - `page|0`：**页码从 1 开始**，`0` 非法。
 */

/** `ClickEvent` 能力的一个单元。 */
export interface ClickEventUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `action` 用枚举名（如 `OPEN_URL`）；`names` 输出全部序列化名（逗号分隔）。
 */
export function clickEventInput(unit: string): ClickEventUnit {
    return { unit };
}