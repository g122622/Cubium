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
 * enum_property_order_all.ts — 全部 40 个枚举属性值顺序的输入构造。
 *
 * **探针调用 Cubium 的真实实现**：`EnumProperty::allowedValues()` +
 * `IProperty::valueToString`。
 *
 * 每个属性 3 个单元：
 * - `values`：按索引顺序的序列化名（原版 `getSerializedName()`）
 * - `count`：值个数
 * - `name`：属性字符串名
 *
 * 最关键的一行是 `VERTICAL_DIRECTION|values`：原版是 `up,down`。
 */

/** 全部枚举属性顺序能力的一个单元。 */
export interface EnumPropertyOrderAllUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function enumPropertyOrderAllInput(unit: string): EnumPropertyOrderAllUnit {
    return { unit };
}