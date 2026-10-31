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
 * enchantment_applicability.ts — 附魔适用性能力的输入构造。
 *
 * 单元的枚举由 oracle 的 `list` 决定（数据包 `enchantment/` 下有哪些附魔就是全集）。
 * 每个单元回答「这个附魔能附到哪些物品上」。
 *
 * 比的是**最终物品集合**，而不是中间表示：原版读 `supported_items` 物品标签，
 * Cubium 用 `EnchantmentType` 枚举归纳，两者表示完全不同，只有集合可比。
 * 成员是集合语义，两侧都在输出前排序。
 */

/** `supportedItems` 是语义无序数组。 */
export const ENCHANTMENT_APPLICABILITY_UNORDERED: readonly string[] = ["supportedItems"];

/** 附魔 ID → 探针与 oracle 共用的一行输入。 */
export function enchantmentApplicabilityInput(unit: string): { enchantment: string } {
    return { enchantment: unit };
}