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
 * shapes_boolean.ts — `VoxelShape` 布尔组合能力的输入构造。
 *
 * 单元形如 `<运算>|<形状A>|<形状B>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 */

/** 布尔组合能力的一个单元。 */
export interface ShapesBooleanUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 形状用名字（`lower` / `upper` / `center` 等）引用，两侧各自展开成同一个 8³ 网格，
 * 因此比较的是运算结果而非形状构造方式。
 */
export function shapesBooleanInput(unit: string): ShapesBooleanUnit {
    return { unit };
}