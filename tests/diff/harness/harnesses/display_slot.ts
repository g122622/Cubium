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
 * display_slot.ts — `DisplaySlot` 能力的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `serial|TEAM_DARK_BLUE`：**团队槽用 `.` 分隔**（`sidebar.team.dark_blue`），
 *   与枚举名的下划线风格不同。
 * - `byId|19`：**越界返回 `LIST`**（`OutOfBoundsStrategy.ZERO`），不是 `null`。
 * - `teamColor|RESET`：**`RESET` 是格式码不是颜色**，返回 `null`。
 */

/** `DisplaySlot` 能力的一个单元。 */
export interface DisplaySlotUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `byId` 与 `teamColor` 输出枚举名（如 `TEAM_RED`）或字面量 `null`。
 */
export function displaySlotInput(unit: string): DisplaySlotUnit {
    return { unit };
}