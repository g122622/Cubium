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
 * portal_relative.ts — 下界传送门相对位置的输入构造。
 *
 * 单元形如 `<cornerX>|<cornerY>|<cornerZ>|<axis1Size>|<axis2Size>|<axis>|<width>|<height>|<posX>|<posY>|<posZ>`。
 *
 * 三组关键用例：
 * - `0|0|0|1|3|x|2.0|1.8|0|0|0`：**宽度不足时 `x` 分量取 `0.5`**（水平居中）。
 * - `0|0|0|2|1|x|0.6|2.0|0|0|0`：**高度不足时 `y` 分量取 `0.0`**（贴底）——与上一条不对称。
 * - `0|0|0|2|3|y|0.6|1.8|0|1|2`：**`axis = Y` 时 `other` 得 `X`**，不是 `Z`。
 */

/** 传送门相对位置能力的一个单元。 */
export interface PortalRelativeUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 输出 `x,y,z` 用 `%.9g` 格式化；`x` / `y` 是归一化插值，`z` 是带符号的绝对偏移。
 */
export function portalRelativeInput(unit: string): PortalRelativeUnit {
    return { unit };
}