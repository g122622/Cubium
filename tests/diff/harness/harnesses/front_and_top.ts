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
 * front_and_top.ts — `FrontAndTop` 查找表的输入构造。
 *
 * 单元形如 `<模式>|<front>|<top>`。
 *
 * 三组关键用例：
 * - `serial|down|north`：**`DOWN_NORTH` 的含义是 `front=DOWN, top=NORTH`**（参数顺序 `(name, front, top)`）。
 * - `serial|north|south`：**同轴组合返回 `null`**，输出 `<null>`。
 * - `top|up|east`：`UP_EAST` 的 `top()` 是 `east`——注意不是 `up`。
 */

/** `FrontAndTop` 能力的一个单元。 */
export interface FrontAndTopUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * 非法组合（`fromFrontAndTop` 返回 `null`）统一输出字符串 `"<null>"`。
 */
export function frontAndTopInput(unit: string): FrontAndTopUnit {
    return { unit };
}