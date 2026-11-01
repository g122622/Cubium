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
 * heightmap_types.ts — 高度图类型判定能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 */

/** 高度图类型能力的一个单元。 */
export interface HeightmapTypesUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 方块属性用四位 `0`/`1` 表达（air、blocksMotion、hasFluid、isLeaves），
 * 对应原版真实方块状态。
 *
 * `MOTION_BLOCKING|0|1|0|1` 与 `MOTION_BLOCKING|0|0|1|1` 两组在原版里没有对应方块
 * （没有「会阻挡的树叶」），oracle 会输出 `noSuchBlock`，已登记为已知缺口。
 */
export function heightmapTypesInput(unit: string): HeightmapTypesUnit {
    return { unit };
}