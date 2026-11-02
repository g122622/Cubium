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
 * world_border.ts — `WorldBorder` 边界判定能力的输入构造。
 *
 * 单元形如 `<模式>|<centerX>,<centerZ>|<size>|<…>`，由探针与 oracle 的 `list`
 * 各自枚举同一张表。
 */

/** 世界边界判定能力的一个单元。 */
export interface WorldBorderUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 整行作为一个字符串字段传递：单元带 `|` 分隔且有多种形态，原样传能让两侧用同一段
 * 解析逻辑。
 *
 * **浮点参数必须以字符串形式保留原始十进制文本**：边界判定对最后一位敏感
 * （`isWithinBounds(AABB)` 里 `1.0E-5F` 与 `1.0E-5` 的差别足以翻转结果），
 * 若走 JSON 数字，两侧解析器的精度处理可能不同而引入假差异。
 *
 * 当前单元表里所有浮点都是十进制字面量，原样透传即可。
 */
export function worldBorderInput(unit: string): WorldBorderUnit {
    return { unit };
}