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
 * enchantment_tags.ts — 附魔标签能力的输入构造。
 *
 * 单元的枚举由 oracle 的 `list` 决定（数据包 `data/minecraft/tags/enchantment/*.json`
 * 里有哪些标签就是全集）。每个单元回答「这个标签包含哪些附魔」。
 *
 * 成员是集合语义（`tooltip_order` 的顺序另属提示框排序，不在本能力范围内），
 * 两侧都在输出前排序，再声明一次无序，避免排序遗漏被当成数据差异。
 */

/** `members` 是语义无序数组。 */
export const ENCHANTMENT_TAGS_UNORDERED: readonly string[] = ["members"];

/** 标签 ID → 探针与 oracle 共用的一行输入。 */
export function enchantmentTagsInput(unit: string): { tag: string } {
    return { tag: unit };
}