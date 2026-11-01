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
 * blockpos_spiral.ts — 螺旋遍历能力的输入构造。
 *
 * 单元形如 `<radius>|<centerX>|<centerY>|<centerZ>|<first>|<second>`。
 *
 * 三组关键用例：
 * - `0|0|0|0|east|south`：**`radius = 0` 只产出中心 1 个点**。
 * - `1|0|0|0|east|south` 对比 `1|0|0|0|south|east`：**交换 first/second 会镜像螺旋**。
 * - 段长度序列是 `leg/2 + 1`（1、1、2、2、3…），**不是 1、2、3、4**。
 */

/** 螺旋遍历能力的一个单元。 */
export interface BlockPosSpiralUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 方向取 `down`/`up`/`north`/`south`/`west`/`east`；输出为 `x,y,z` 用 `;` 连接。
 */
export function blockPosSpiralInput(unit: string): BlockPosSpiralUnit {
    return { unit };
}