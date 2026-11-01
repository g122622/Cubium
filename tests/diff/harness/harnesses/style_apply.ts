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
 * style_apply.ts — `Style` 格式应用与合并的输入构造。
 *
 * 单元形如 `<模式>|<style 7 段>|<参数>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `applyFormat|ff0000|1|1|-|-|-|-|green`：**只换色，保留格式标志**。
 * - `applyLegacyFormat|ff0000|1|1|1|1|1|-|green`：**换色同时清空全部格式标志**——与上一条对比。
 * - `applyTo|ff0000|1|-|...|-|0|1|...`：**`this` 的非空字段优先**，`null` 才回落到 `other`。
 */

/** `Style` 能力的一个单元。 */
export interface StyleApplyUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 样式编码为 7 段：`<color|->|<bold|->|<italic|->|<underlined|->|<strikethrough|->|<obfuscated|->|<insertion|->`。
 * 布尔三元值 `-` 表示 `null`（继承），`0` 表示 `false`（显式关闭），`1` 表示 `true`。
 */
export function styleApplyInput(unit: string): StyleApplyUnit {
    return { unit };
}