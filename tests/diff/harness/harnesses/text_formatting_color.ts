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
 * text_formatting_color.ts — `ChatFormatting` 颜色/码位/分类查询的输入构造。
 *
 * **本能力是第一个真正对接 Cubium 实现的探针**：`probes/text_formatting_color.cpp`
 * 直接调用 `mc::text::getFormattingColor` / `isColor` / `isStyle` / `fromCode` / `toCode`，
 * 而非手抄原版逻辑。
 *
 * 三组关键用例：
 * - `color|BOLD`：**Cubium 返回 `FFFFFFFF`（白色哨兵），原版返回 `null`**——预期差异点。
 * - `isStyle|RESET`：**`false`**（`isStyle` 上界是 `Italic`，不含 `Reset`）。
 * - `fromCode|q`：**Cubium 返回 `None`，原版返回 `null`**——同为哨兵值差异。
 */

/** `ChatFormatting` 能力的一个单元。 */
export interface TextFormattingColorUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `color` 输出 8 位大写 ARGB 或字面量 `null`；`fromCode` 输出枚举名或 `NONE`。
 */
export function textFormattingColorInput(unit: string): TextFormattingColorUnit {
    return { unit };
}