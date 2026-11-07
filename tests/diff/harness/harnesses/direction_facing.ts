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
 * direction_facing.ts — 方向的两个查询接口。
 *
 * | 方法 | 语义 |
 * |---|---|
 * | `getAxisDirection()` | 该方向在所属轴上是正还是负 |
 * | `isFacingAngle(yaw)` | 给定偏航角下，该方向是否朝向观察者前方 |
 *
 * `isFacingAngle` 内部走 `Mth.sin` / `Mth.cos` 的查表实现，
 * 因此单元要覆盖查表边界的角度（±45°、±90°、±135° 一类）。
 *
 * 单元格式：`<mode>|<dir>|<arg>`。
 */

/** 该能力的一个单元。 */
export interface DirectionFacingUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function directionFacingInput(unit: string): DirectionFacingUnit {
    return { unit };
}