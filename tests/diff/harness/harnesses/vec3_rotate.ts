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
 * vec3_rotate.ts — `Vector3d` 的旋转与投影。
 *
 * | 方法 | 语义 |
 * |---|---|
 * | `xRot` / `yRot` / `zRot` | 绕指定轴旋转给定弧度 |
 * | `projectedOn` | 在给定向上的投影 |
 * | `horizontal()` | 只保留 X/Z 分量 |
 *
 * 旋转走 `Mth.sin` / `Mth.cos` 的查表实现，因此在 **90° 的整数倍**上
 * 会得到精确的 0 与 ±1，而不是 libm 的 ±1e-8 残差。单元刻意覆盖这些角度。
 *
 * 单元格式：`<mode>|<vec>|<arg>`。
 */

/** 该能力的一个单元。 */
export interface Vec3RotateUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function vec3RotateInput(unit: string): Vec3RotateUnit {
    return { unit };
}