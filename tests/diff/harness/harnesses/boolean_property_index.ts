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
 * boolean_property_index.ts — `BooleanProperty` 内部值顺序的输入构造。
 *
 * **探针调用 Cubium 的真实实现**：`BooleanProperty::allowedValues()` / `indexOf`。
 *
 * 最关键的三个单元（每个属性各一组）：
 * - `values|<prop>`：原版是 **`true,false`**，Cubium 是 `false,true`。
 * - `index|<prop>|true`：原版是 **`0`**，Cubium 是 `1`。
 * - `at|<prop>|0`：原版是 **`true`**，Cubium 是 `false`。
 *
 * 这三者共同决定方块状态调色板里布尔属性的偏移量。
 */

/** 布尔属性索引能力的一个单元。 */
export interface BooleanPropertyIndexUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function booleanPropertyIndexInput(unit: string): BooleanPropertyIndexUnit {
    return { unit };
}