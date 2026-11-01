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
 * enchantment_storage.ts — `ItemEnchantments.Mutable` 增删改能力的输入构造。
 *
 * 单元是一串用 `,` 分隔的操作，每个操作为 `<op>:<附魔名>:<等级>`，
 * 由探针与 oracle 的 `list` 各自枚举同一张表。
 */

/** 附魔存储能力的一个单元。 */
export interface EnchantmentStorageUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 附魔名用单字母 `a`~`e` 代表注册表中排序后的前 5 个附魔；等级是整数。整行原样传递
 * 能让两侧共用同一段解析逻辑。
 */
export function enchantmentStorageInput(unit: string): EnchantmentStorageUnit {
    return { unit };
}