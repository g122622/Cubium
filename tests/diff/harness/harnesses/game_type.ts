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
 * game_type.ts — `GameType` 能力的输入构造。
 *
 * **探针调用 Cubium 的真实实现**：`mc::GameMode` 枚举与名字转换。
 *
 * 三组关键用例：
 * - `byId|4`：**`ZERO` 越界策略返回 `SURVIVAL`**（`CLAMP` 会给 `SPECTATOR`）。
 * - `flags|ADVENTURE`：**`isSurvival()` 对冒险模式也返回 `true`**（`110`）。
 * - `shortName|SURVIVAL`：**`selectWorld.gameMode.survival`**，长名是 `gameMode.survival`。
 */

/** `GameType` 能力的一个单元。 */
export interface GameTypeUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `flags` 输出三位 `0/1`（creative / survival / blockPlacingRestricted）。
 */
export function gameTypeInput(unit: string): GameTypeUnit {
    return { unit };
}