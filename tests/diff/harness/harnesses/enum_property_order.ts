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
 * enum_property_order.ts — `EnumProperty` 值顺序的输入构造。
 *
 * **探针调用 Cubium 的真实实现**：`EnumProperty::allowedValues()` / `indexOf`。
 *
 * 三个关键单元：
 * - `facing|values`：原版是 **`NORTH,EAST,SOUTH,WEST,UP,DOWN`**，Cubium 是
 *   `DOWN,UP,NORTH,SOUTH,WEST,EAST`——**两者完全不同**。
 * - `facing|cycle|NORTH`：调试棒点一次后的去向（原版 `EAST`，Cubium `SOUTH`）。
 * - `facing|index|NORTH`：原版 `0`，Cubium `2`。
 *
 * 底层 `Direction` 枚举顺序两边一致，差异只在「属性」这一层。
 */

/** 枚举属性顺序能力的一个单元。 */
export interface EnumPropertyOrderUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function enumPropertyOrderInput(unit: string): EnumPropertyOrderUnit {
    return { unit };
}