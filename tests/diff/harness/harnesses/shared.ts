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

/*
 * shared.ts — 各 harness 共用的小工具。
 */

/** 把数值舍入到 f32（Java 的 float 与 C++ 的 float 都是 IEEE-754 单精度）。 */
export function f32(value: number): number {
    const buffer = new DataView(new ArrayBuffer(4));
    buffer.setFloat32(0, value, false);
    return buffer.getFloat32(0, false);
}

/** 与 C++ 的 floatBits 一致的十六进制表示：`0x` + 8 位小写十六进制。 */
export function f32Bits(value: number): string {
    const buffer = new DataView(new ArrayBuffer(4));
    buffer.setFloat32(0, value, false);
    return `0x${buffer.getUint32(0, false).toString(16).padStart(8, "0")}`;
}

/** 按字典序比较字符串（与 C++ 的 std::string 比较一致，按 UTF-16 码元）。 */
export function compareStrings(lhs: string, rhs: string): number {
    return lhs < rhs ? -1 : lhs > rhs ? 1 : 0;
}