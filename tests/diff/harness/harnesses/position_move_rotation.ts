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
 * position_move_rotation.ts — `PositionMoveRotation.calculateAbsolute` 的输入构造。
 *
 * 单元形如 `<p1 的 8 段>|<p2 的 8 段>|<relatives>`，共 17 段。
 *
 * 三组关键用例：
 * - `...|x,y,z` 与 `...|-`：**含 `X` 是相对移动，不含是绝对设置**。
 * - `...70|...40|-`：**`xRot` 被钳到 90**；而 `yRot` 累加到 210 不钳。
 * - `...|rotate_delta`：**速度向量按角度差旋转**，先 `xRot` 再 `yRot`。
 */

/** `PositionMoveRotation` 能力的一个单元。 */
export interface PositionMoveRotationUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 输出 `posX,posY,posZ|deltaX,deltaY,deltaZ|yRot|xRot`，浮点用 `%.9g`。
 */
export function positionMoveRotationInput(unit: string): PositionMoveRotationUnit {
    return { unit };
}