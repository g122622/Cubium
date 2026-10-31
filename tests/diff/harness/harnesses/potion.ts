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

/*
 * potion.ts — 药水定义能力的输入构造与比较口径。
 *
 * 单元的枚举由 oracle 的 `list` 决定（原版有哪些药水就是全集），探针侧只负责按同一个 ID
 * 输出自己的定义。效果列表在两侧都先按效果 ID 排序：原版 `Potion` 存的是 `List`，
 * 顺序没有玩法语义，集合语义下才可比。
 */

/** `effects` 是语义无序数组：两侧都在输出前按效果 ID 排序，这里再声明一次以防漏排。 */
export const POTION_UNORDERED: readonly string[] = ["effects"];

/** 药水 ID → 探针与 oracle 共用的一行输入。 */
export function potionInput(unit: string): { potion: string } {
    return { potion: unit };
}