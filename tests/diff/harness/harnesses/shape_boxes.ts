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
 * shape_boxes.ts — `VoxelShape.forAllBoxes` 的输入构造。
 *
 * 把一个形状拆成若干轴对齐盒逐个回调。本能力把回调结果按顺序拼接后比对，
 * 因此**既是数值比对，也是遍历顺序比对**。
 *
 * 单元是一个盒子 `x1,y1,z1,x2,y2,z2`。
 */

/** 该能力的一个单元。 */
export interface ShapeBoxesUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function shapeBoxesInput(unit: string): ShapeBoxesUnit {
    return { unit };
}