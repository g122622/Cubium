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
 * fluid_spread_dir.ts — 流体扩散方向选择能力的输入构造。
 *
 * 单元形如 `<流体>|n,e,s,w`，四个方向的坡度距离，`-1` 表示不可通行。
 */

/** 方向选择能力的一个单元。 */
export interface FluidSpreadDirUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 所有分量都是整数，无浮点表示问题；整行原样传递能让两侧共用同一段解析逻辑。
 */
export function fluidSpreadDirInput(unit: string): FluidSpreadDirUnit {
    return { unit };
}