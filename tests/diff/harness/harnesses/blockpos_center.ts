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
 * blockpos_center.ts — BlockPos 基础变换的输入构造。
 *
 * 覆盖三个方法：
 *   - `bottomCenter`：底面中心，Y 不偏移、X/Z 各加 0.5；
 *   - `multiply`：各分量按标量缩放，含 0 与负数；
 *   - `subtract`：逐分量相减，含负坐标。
 *
 * 浮点输出保留 6 位小数比对，不比对位模式——浮点末位差异不在本项目的对齐范围内。
 *
 * 单元格式：`<mode>|<参数…>`。
 */

/** 该能力的一个单元。 */
export interface BlockposCenterUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function blockposCenterInput(unit: string): BlockposCenterUnit {
    return { unit };
}