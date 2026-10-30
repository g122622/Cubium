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
 * select.ts — 决定「这次该跑哪些能力」。
 *
 * 五种选择方式，互斥：
 *   - 显式名字（`run item_summary loot_table`）：只跑这些；
 *   - `--changed REF` / `--changed-files FILE`：只跑受这些改动影响、且当前输入没有通过记录的能力；
 *   - `--failed`：只跑上次失败的；
 *   - `--all`：全部，忽略通过记录；
 *   - 缺省：所有「依赖摘要与上次不一致」的能力（即输入变过或从没跑过的）。
 *
 * 「受影响的」判定用依赖图（见 graph.ts）：改动的文件出现在某能力的代码依赖或运行期模块里。
 * 从来没通过过的能力无论是否受影响都要跑——它本来就没有可沿用的结论。反过来，改了依赖图
 * 里没学到的东西（第一次新增的能力）也会因为「没有记录」而自然被选中。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";

import type { DependencyEntry } from "./graph.ts";
import { affectedHarnesses } from "./graph.ts";
import type { HarnessRecord } from "./state.ts";
import { REPO_ROOT } from "./layout.ts";

export interface SelectionRequest {
    /** 显式给出的能力名；非空时只跑这些。 */
    readonly names: readonly string[];
    /** 与 merge base 比较的 git ref（如 origin/main）。 */
    readonly changedRef?: string;
    /** 改动文件清单（每行一个路径），适用于没有 git 历史的副本。 */
    readonly changedFiles?: string;
    readonly failed: boolean;
    readonly all: boolean;
}

export interface SelectionInput {
    readonly available: readonly string[];
    readonly entries: ReadonlyMap<string, DependencyEntry>;
    /** 当前依赖摘要（能力 → 摘要）；依赖图不可用时为空。 */
    readonly digests: ReadonlyMap<string, string>;
    readonly records: Readonly<Record<string, HarnessRecord>>;
    /** 依赖图不可用时为 false：所有能力都视为「必须运行」。 */
    readonly graphAvailable: boolean;
}

export interface SelectionResult {
    readonly selected: string[];
    /** 需要先构建的探针（具体名字对不上时为空，调用方退回全量构建）。 */
    readonly probes: string[];
    /** 选择器的说明信息（打印给用户，解释「为什么是这些」）。 */
    readonly notes: string[];
}

/** 某个能力当前是否能沿用上次的通过结论。 */
export function isFresh(name: string, input: SelectionInput): boolean {
    const record = input.records[name];
    if (record === undefined || record.status !== "pass" || record.graph !== undefined) {
        return false;
    }
    if (!input.graphAvailable) {
        return false;
    }
    const digest = input.digests.get(name);
    return digest !== undefined && digest === record.digest;
}

/** 读取 git 的改动文件清单（相对 merge base，含未提交改动与未跟踪文件）。 */
export function changedFilesFromGit(ref: string): string[] | undefined {
    const git = (...args: string[]): string | undefined => {
        try {
            return execFileSync("git", args, { cwd: REPO_ROOT, encoding: "utf8", stdio: ["ignore", "pipe", "pipe"] });
        } catch {
            return undefined;
        }
    };
    const base = git("merge-base", "HEAD", ref)?.trim();
    if (base === undefined || base.length === 0) {
        return undefined;
    }
    const diff = git("diff", "--name-only", base) ?? "";
    const others = git("ls-files", "--others", "--exclude-standard") ?? "";
    const files = new Set([...diff.split("\n"), ...others.split("\n")].map((line) => line.trim()).filter(Boolean));
    return [...files].sort();
}

/** 按请求计算本次要运行的能力。 */
export function select(request: SelectionRequest, input: SelectionInput): SelectionResult {
    const notes: string[] = [];
    const known = new Set(input.available);
    const unknown = request.names.filter((name) => !known.has(name));
    if (unknown.length > 0) {
        throw new Error(`未知能力：${unknown.join("、")}（可用：${input.available.join("、")}）`);
    }
    let selected: string[];
    if (request.names.length > 0) {
        selected = [...request.names];
    } else if (request.failed) {
        selected = input.available.filter((name) => input.records[name]?.status !== undefined && input.records[name]?.status !== "pass");
        notes.push(`上次失败：${selected.length} 个`);
    } else if (request.all) {
        selected = [...input.available];
        notes.push("--all：忽略通过记录，全部运行");
    } else if (request.changedRef !== undefined || request.changedFiles !== undefined) {
        const changed = request.changedFiles !== undefined
            ? readChangedList(request.changedFiles)
            : changedFilesFromGit(request.changedRef as string);
        if (changed === undefined) {
            throw new Error(`无法得到改动清单：${request.changedRef ?? request.changedFiles}（没有 git 历史或 merge base？改用 --changed-files）`);
        }
        // `--changed` 的语义是「跑受这些改动影响的」，**不**用通过记录过滤：显式给出一批改动时
        // 用户就是在要求验证它们，而「摘要没变」恰恰可能是改动没被依赖图收录（工具本身的缺陷），
        // 用通过记录过滤会把这种漏检掩盖成「无需运行」。
        const affected = new Set(affectedHarnesses(input.entries, changed));
        selected = input.available.filter((name) => affected.has(name));
        notes.push(`${changed.length} 个文件改动：${affected.size} 个能力依赖它们，全部运行`);
        const unlearned = input.available.filter((name) => input.records[name] === undefined);
        if (unlearned.length > 0) {
            // 没跑过的能力没有运行期模块，依赖图看不全：必须一并运行，否则会漏掉它依赖的改动。
            selected = [...new Set([...selected, ...unlearned])].sort();
            notes.push(`另有 ${unlearned.length} 个能力从未运行过，一并运行以建立依赖图`);
        }
    } else {
        selected = input.available.filter((name) => !isFresh(name, input));
    }
    // 需要构建的探针：本次要跑的、且依赖图能从名字推出来的能力。
    const probes = selected.filter((name) => input.entries.has(name));
    return { selected, probes, notes };
}

function readChangedList(file: string): string[] {
    const text = fs.readFileSync(file, "utf8");
    return [...new Set(text.split(/\r?\n/).map((line) => line.trim()).filter((line) => line.length > 0))].sort();
}