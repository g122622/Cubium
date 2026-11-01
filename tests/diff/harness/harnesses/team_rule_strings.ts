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
 * team_rule_strings.ts — 队伍可见性 / 碰撞规则字符串映射的输入构造。
 *
 * **探针调用 Cubium 的真实实现**：`mc::scoreboard::teamVisibilityToString` 等。
 *
 * 三组关键用例：
 * - `visibilityToString|HIDE_FOR_OTHER_TEAMS`：**`hideForOtherTeams`（驼峰）**，
 *   不是 `hide_for_other_teams`。
 * - `visibilityFromString|dunno`：**原版返回 `null`，Cubium 回退到 `Always`**——预期差异点。
 * - `visibilityFromString|hide_for_other_teams`：**未命中**（下划线不是合法形式）。
 */

/** 队伍规则字符串能力的一个单元。 */
export interface TeamRuleStringsUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `toString` 输出驼峰序列化名；`fromString` 输出枚举名或字面量 `null`。
 */
export function teamRuleStringsInput(unit: string): TeamRuleStringsUnit {
    return { unit };
}