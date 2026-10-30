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
 * state.ts — 增量复跑的账本。
 *
 * 每次运行结束把「每个能力读到了什么、结论是什么、依赖内容摘要」写进
 * `<构建目录>/diff/state/state.json`，下次运行据此决定哪些能力可以沿用上次的通过结论。
 *
 * 一条记录的语义：
 *   - `digest`：依赖（代码 + 运行期模块 + 构建产物 + 环境）的内容摘要。当前重新算出的摘要
 *     与它相同，说明「同样的输入重新跑一遍必然得到同样的结论」，通过结论可以沿用；
 *   - `files` / `modules` / `artifacts`：依赖清单本身，`--changed` 选择器靠它判断一次改动
 *     会影响哪些能力；
 *   - `units` / `matched` / `seconds`：调度依据（耗时长的先跑）与摘要展示。
 *
 * 失败记录同样写入：让 `--failed` 与 `--status` 能看到失败的能力，但**从不**被当成可沿用
 * 的结论——只有 pass 且摘要一致才复用。
 *
 * 状态与构建目录绑定：不同构建配置的 probe 不是同一个产物，摘要与产物哈希对不上时会自然
 * 失效，不需要额外区分。
 */

import fs from "node:fs";
import path from "node:path";

import type { HashCache } from "../core/hash.ts";
import { pruneHashCache } from "../core/hash.ts";
import type { DependencyEntry } from "./graph.ts";

/** 状态文件格式版本：结构不兼容的旧文件直接丢弃重建。 */
const STATE_VERSION = 2;

export interface HarnessRecord {
    readonly status: "pass" | "fail" | "error";
    /** 依赖内容摘要（见文件头注释）。 */
    readonly digest: string;
    readonly files: readonly string[];
    readonly modules: readonly string[];
    readonly artifacts: readonly string[];
    /** 原版 oracle 输出的缓存键；没有 oracle 的能力为 undefined。 */
    readonly oracleKey?: string;
    readonly units?: number;
    readonly matched?: number;
    /** 已登记的已知缺口条数（`--status` 里点名，避免闸门悄悄变宽）。 */
    readonly knownDifferences?: number;
    readonly seconds?: number;
    /** 最近一次运行时间（ISO 8601）。 */
    readonly finished?: string;
    /** 失败摘要（差异处数或异常信息）。 */
    readonly failure?: string;
    /** 依赖图不可用时的说明：该记录只是「记录」，不能作为复用依据。 */
    readonly graph?: string;
}

export interface RunnerState {
    readonly version: number;
    readonly harnesses: Record<string, HarnessRecord>;
    readonly hashes: HashCache;
    readonly lastRun?: {
        readonly failures: readonly string[];
        readonly finished: string;
        readonly seconds: number;
    };
}

export function statePath(stateDirectory: string): string {
    return path.join(stateDirectory, "state.json");
}

/** 读状态文件；不存在或格式不兼容时返回空状态。 */
export function loadState(stateDirectory: string): RunnerState {
    const file = statePath(stateDirectory);
    const empty: RunnerState = { version: STATE_VERSION, harnesses: {}, hashes: {} };
    if (!fs.existsSync(file)) {
        return empty;
    }
    try {
        const parsed = JSON.parse(fs.readFileSync(file, "utf8")) as RunnerState;
        if (parsed.version !== STATE_VERSION || typeof parsed.harnesses !== "object" || parsed.harnesses === null) {
            return empty;
        }
        return { version: STATE_VERSION, harnesses: parsed.harnesses, hashes: parsed.hashes ?? {}, lastRun: parsed.lastRun };
    } catch (error) {
        // 状态文件损坏只带来「全部重跑」，不影响正确性；但要让用户知道为什么会慢。
        console.warn(`[diff] 状态文件不可解析（${(error as Error).message}），本次全部重新运行：${file}`);
        return empty;
    }
}

/** 写状态文件（先写临时文件再改名，中途中断不会留下半个文件）。 */
export function saveState(stateDirectory: string, state: RunnerState, live: Set<string>): void {
    fs.mkdirSync(stateDirectory, { recursive: true });
    const pruned: RunnerState = { ...state, hashes: pruneHashCache(state.hashes, live) };
    const file = statePath(stateDirectory);
    const temporary = `${file}.${process.pid}.tmp`;
    fs.writeFileSync(temporary, `${JSON.stringify(pruned, null, 1)}\n`);
    fs.renameSync(temporary, file);
}

/** 由依赖条目与运行结果构造一条记录。 */
export function buildRecord(
    entry: DependencyEntry,
    fields: Pick<HarnessRecord, "status" | "digest"> & Partial<HarnessRecord>,
): HarnessRecord {
    return {
        ...fields,
        files: entry.files,
        modules: entry.modules,
        artifacts: entry.artifacts,
    };
}

/** 一次运行结束时的整体结果摘要（`--status` 与退出码使用）。 */
export interface RunSummary {
    readonly total: number;
    readonly passed: number;
    readonly failed: readonly string[];
    readonly errored: readonly string[];
    readonly skipped: number;
    readonly seconds: number;
}