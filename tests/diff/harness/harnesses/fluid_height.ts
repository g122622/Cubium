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
 * fluid_height.ts — 流体高度能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 只有 `amount` 与 `ownHeight` 进入单元表：`height` 模式的 `hasSameAbove` 需要真实
 * `BlockGetter`，属 world 型，无法用原版实现比对，因此不在单元表内（探针仍实现该分支，
 * 但仅为保持与 `FlowingFluid.getHeight` 的结构对应，不参与差分）。
 */

/** 流体高度能力的一个单元。 */
export interface FluidHeightUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 浮点按 `%.9g` 格式化后去掉尾随零。这里必须用 `%.9g` 而非 `%.6g`：`n/9.0F` 在 `float`
 * 下有约 7 位有效数字，`%.6g` 会丢失末位精度，导致两侧因格式化截断而「假一致」。
 */
export function fluidHeightInput(unit: string): FluidHeightUnit {
    return { unit };
}