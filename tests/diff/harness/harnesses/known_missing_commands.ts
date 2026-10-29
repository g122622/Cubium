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
 * 清单来自 command_tree harness 的运行结果（原版 89 条 vs Cubium 80 条）：
 * 下面 9 条尚未实现，其余 80 条已逐字一致。
 * 每实现一条就从这里删一条 —— 这些是**实现缺口**，不是参照数据缺失；留在这里的
 * 条目会在每次运行摘要里单独点名，不会静默消失。
 *
 * 别名（`w`/`tm`/`tell` → `msg`，`xp` → `experience`）在原版里是独立注册的节点；
 * 这些均已在 Cubium 注册。
 */
export const KNOWN_MISSING_COMMANDS = [
    "fetchprofile",
    "fillbiome",
    "item",
    "place",
    "random",
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
/*
 * 本项目独有、原版服务端没有的根命令。
 *
 * 这些不是「未对齐」：它们是 Cubium 自己的调试/运维命令，原版没有对应实现。列在这里
 * 是为了让 command_tree 的比对把这些命令从探针一侧摘掉，而不是让报告长期挂着一处假差异。
 * 新增自研命令时同步登记；命令被删除时也要从这里删掉，避免清单腐化。
 */
export const PROJECT_ONLY_COMMANDS = ["panel"];

/**
 * 命令树对象级清理：把指定命令从 `tree` 之外的派生字段里摘掉。
 *
 * `tree` 每项是「根命令 + 子节点签名」，用前缀匹配指出哪些项属于该命令；`arguments` /
 * `components` 的键是完整节点路径（以根命令名开头），同样按前缀摘；`wire` 是整棵树编码
 * 后的字节，一旦两侧命令集合不同就必然不同，只能整体丢弃——它的作用（节点顺序、重定向下标
 * 等结构级偏差）由 `tree` 逐项比对承担，摘掉不会造成盲区。
 */
export function dropProjectOnlyCommands(
    record: Record<string, unknown>,
    matches: (name: string) => boolean,
): Record<string, unknown> {
    const withoutKey = (value: unknown): unknown => {
        if (value === null || typeof value !== "object" || Array.isArray(value)) {
            return value;
        }
        const result: Record<string, unknown> = {};
        for (const [key, field] of Object.entries(value as Record<string, unknown>)) {
            const root = key.trimStart().split(" ")[0];
            if (!matches(root)) {
                result[key] = field;
            }
        }
        return result;
    };
    const result: Record<string, unknown> = {};
    for (const [key, value] of Object.entries(record)) {
        if (key === "wire") {
            // 整棵树编码后的字节：两侧命令集合不同就必然不同，没有「摘掉一部分」可言。
            continue;
        }
        result[key] = key === "tree" || key === "unit" ? value : withoutKey(value);
    }
    return result;
}
