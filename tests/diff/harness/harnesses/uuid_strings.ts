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
 * uuid_strings.ts — UUID 字符串解析与格式化的输入构造。
 *
 * **探针调用 Cubium 的真实实现**：`mc::util::uuidFromString` / `uuidToString`。
 *
 * 三组关键用例：
 * - `parse|069a79f444e94726a5befca90e38aaf5`：**32 位无连字符**——
 *   JDK 的 `UUID.fromString` **拒绝**，Cubium **接受**。预期差异点。
 * - `parse|dunno`：JDK 抛异常（`null`），Cubium 返回全零。
 * - `toString|...`：Cubium 输出 32 位，原版 `toString()` 是 36 位。
 */

/** UUID 字符串能力的一个单元。 */
export interface UuidStringsUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `parse` / `toStringDashes` 输出 36 位小写；`toString` 输出 32 位无连字符。
 */
export function uuidStringsInput(unit: string): UuidStringsUnit {
    return { unit };
}