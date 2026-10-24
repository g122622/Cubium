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
 * known_missing_commands.ts — 原版服务端存在、Cubium 尚未注册的根命令。
 *
 * 清单来自 command_tree harness 第一次运行的结果（原版 89 条 vs Cubium 74 条）。
 * 每实现一条就从这里删一条 —— 这些是**实现缺口**，不是参照数据缺失；留在这里的
 * 条目会在每次运行摘要里单独点名，不会静默消失。
 *
 * 别名（`w`/`tm`/`tell` → `msg`，`xp` → `experience`）在原版里是独立注册的节点；
 * Cubium 已有 `w`/`tell`/`xp`，缺的只有 `tm`（列在下面）。
 */
export const KNOWN_MISSING_COMMANDS = [
    "damage",
    "debug",
    "fetchprofile",
    "fillbiome",
    "item",
    "place",
    "random",
    "return",
    "ride",
    "rotate",
    "stopwatch",
    "test",
    "tick",
    "transfer",
    "waypoint",
] as const;

/**
 * 原版已移除、Cubium 仍在注册的「多余命令」。它们不在原版命令树里，属于兼容层残留：
 *   - `debugconfig`：原版 1.21 起只有 `/debug`；
 *   - `replaceitem`：1.17 起被 `/item replace` 取代；
 *   - `publish`：原版只存在于集成服务器（单人）。
 * 这些不属于「原版有、这里没有」的缺口，但同样要求集合一致，故显式列出。
 */
export const EXTRA_COMMANDS = [
    "debugconfig",
    "publish",
    "replaceitem",
    "warden_spawn_tracker",
] as const;