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
 * block_state_enum_strings_2.ts — 第二组方块状态枚举字符串映射的输入构造。
 *
 * **探针调用 Cubium 的真实实现**：`EnumProperty<T>::Traits::toString` / `fromName`。
 *
 * 三组关键用例：
 * - `PistonType|toString|DEFAULT`：**`normal`**（不是 `default`）。
 * - `SlabType|toString|BOTTOM`：**`bottom`**，且 `SlabType` 顺序与 `Half` 相反。
 * - `PistonType|fromName|default`：**`null`**（`default` 不是合法序列化名）。
 */

/** 第二组方块状态枚举字符串能力的一个单元。 */
export interface BlockStateEnumStrings2Unit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `toString` 输出小写名；`fromName` 输出枚举常量名或字面量 `null`。
 */
export function blockStateEnumStrings2Input(unit: string): BlockStateEnumStrings2Unit {
    return { unit };
}