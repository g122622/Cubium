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
 * explosion_seen_grid.ts — 爆炸可见度采样网格能力的输入构造。
 *
 * 单元是实体碰撞箱的六个边界，形如 `minX|minY|minZ|maxX|maxY|maxZ`。
 * 探针与 oracle 的 `list` 各自枚举同一张尺寸表。
 */

/** 爆炸可见度采样网格能力的一个单元。 */
export interface ExplosionSeenGridUnit {
    readonly minX: string;
    readonly minY: string;
    readonly minZ: string;
    readonly maxX: string;
    readonly maxY: string;
    readonly maxZ: string;
}

/**
 * 单元字符串 → 探针与 oracle 共用的一行输入。
 *
 * 六个分量全部按字符串传递，避免十进制表示在两侧解析出不同的 double。
 */
export function explosionSeenGridInput(unit: string): ExplosionSeenGridUnit {
    const parts = unit.split("|");
    if (parts.length !== 6) {
        throw new Error(`explosion_seen_grid: 单元应为 6 段 'minX|minY|minZ|maxX|maxY|maxZ'：${unit}`);
    }
    return {
        minX: parts[0],
        minY: parts[1],
        minZ: parts[2],
        maxX: parts[3],
        maxY: parts[4],
        maxZ: parts[5],
    };
}