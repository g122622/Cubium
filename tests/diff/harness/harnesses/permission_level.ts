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
 * permission_level.ts — `PermissionLevel` 能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `byId|5`：**`CLAMP` 钳到 `OWNERS`**（`WRAP` 会给 `MODERATORS`、`ZERO` 会给 `ALL`）。
 * - `byId|-1`：**钳到 `ALL`**（`WRAP` 会给 `OWNERS`）。
 * - `higher|ALL|ALL`：**相等时返回 `true`**（`>=` 不是 `>`）。
 */

/** `PermissionLevel` 能力的一个单元。 */
export interface PermissionLevelUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `byId` 输出枚举名；`higher` 输出 `"true"` / `"false"`。
 */
export function permissionLevelInput(unit: string): PermissionLevelUnit {
    return { unit };
}