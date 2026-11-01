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
 * anvil_durability.ts — 铁砧合并耐久度能力的输入构造。
 *
 * 单元形如 `primaryMax|primaryDamage|secondaryMax|secondaryDamage`，由探针与 oracle
 * 的 `list` 各自枚举同一张表。
 */

/** 铁砧耐久合并能力的一个单元。 */
export interface AnvilDurabilityUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 整行作为一个字符串字段传递：单元本身带 `|` 分隔，原样传能让两侧用同一段解析逻辑。
 * 四个分量都是整数，不存在浮点表示差异。
 */
export function anvilDurabilityInput(unit: string): AnvilDurabilityUnit {
    return { unit };
}