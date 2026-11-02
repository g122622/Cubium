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
 * text_formatting_name.ts — `ChatFormatting` 名字双向映射的输入构造。
 *
 * **探针调用 Cubium 的真实实现**：`mc::text::toName` / `mc::text::fromName`。
 *
 * 三组关键用例：
 * - `name|DARK_BLUE`：**`dark_blue`（snake_case 小写）**，不是 `DARK_BLUE` 也不是 `darkBlue`。
 * - `fromName|DARK_BLUE`：**大小写不敏感**（Cubium 先 `tolower`），与原版 `getNames` 的小写基准一致。
 * - `fromName|darkBlue`：**未命中**（snake_case 才是规范形式）→ `NONE`。
 */

/** `ChatFormatting` 名字能力的一个单元。 */
export interface TextFormattingNameUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `name` / `names` 输出小写 snake_case 名；`fromName` 输出枚举名或 `NONE`。
 */
export function textFormattingNameInput(unit: string): TextFormattingNameUnit {
    return { unit };
}