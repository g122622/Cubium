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
 * `mth_atan2` 的已知缺口。
 *
 * 原版 `Mth.atan2` 内部有两张 257 项的表：`ASIN_TAB[i] = Math.asin(i/256.0)`、
 * `COS_TAB[i] = Math.cos(asin(i/256.0))`。
 *
 * 本项目用 `std::asin` / `std::cos` 构造同样的表。**绝大多数项逐位一致**，
 * 但个别项相差 1 ULP——实测 `i = 181` 时 `cos` 值：
 *
 * | | 位模式 |
 * |---|---|
 * | Java `Math.cos` | `0x3fe6a13cc8a9b946` |
 * | C++ `std::cos`  | `0x3fe6a13cc8a9b947` |
 *
 * 这是两个平台 libm 实现细节的差异，不是业务代码缺陷：即使把整条 `atan2` 链路写成
 * 与原版字面一致的 C++，只要底层 `cos` 差 1 ULP，结果就会差 1 ULP。
 *
 * 受影响的单元只有 `y` 与 `x` 的绝对值相等那两个（`1|1` 与 `-1|1`）——它们恰好落在
 * `i = 181` 这一格上。其余 31 个单元逐位一致。
 *
 * 保留在能力里而不是从单元表删掉，是为了让这个平台差异在测试报告里持续可见。
 */

/** `mth_atan2` 的已知差异键。 */
export const KNOWN_ATAN2_GAPS: readonly { key: string; field: string; reason: string }[] = [
    "1|1",
    "-1|1",
].map((unit) => ({
    key: unit,
    field: "bits",
    reason: "平台 libm 差异：std::cos 在 asin(181/256) 处比 Java Math.cos 差 1 ULP",
}));
