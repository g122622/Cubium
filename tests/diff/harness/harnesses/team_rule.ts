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
 * team_rule.ts — `Team.CollisionRule` / `Team.Visibility` 的输入构造。
 *
 * 单元形如 `<模式>|<参数…>`，由探针与 oracle 的 `list` 各自枚举同一张表。
 *
 * 三组关键用例：
 * - `collisionName|PUSH_OTHER_TEAMS`：**`pushOtherTeams`（Teams 复数）**，
 *   而 `PUSH_OWN_TEAM` 是 `pushOwnTeam`（Team 单数）。
 * - `collisionDisplay|ALWAYS`：翻译键 `team.collision.always`，
 *   `visibilityDisplay|ALWAYS` 是 `team.visibility.always`——**同名字、不同键**。
 * - `collisionById|4`：**越界返回 `ALWAYS`**（`OutOfBoundsStrategy.ZERO`）。
 */

/** `Team` 规则能力的一个单元。 */
export interface TeamRuleUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `*Display` 输出翻译键；`*ById` 输出枚举名（如 `ALWAYS`）。
 */
export function teamRuleInput(unit: string): TeamRuleUnit {
    return { unit };
}