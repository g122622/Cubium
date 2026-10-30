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
 * runner.ts — 差分测试的编排入口。
 *
 * 编排层要回答的只有三个问题：**这次该跑哪些能力**（select.ts）、**该构建哪些探针**
 * （build.ts）、**怎么并行跑完并记账**（scheduler.ts + state.ts）。每个能力自己怎么跑
 * 在执行进程里（worker.ts → runner/session.ts），主进程从不加载能力声明之外的业务代码。
 *
 * 一个能力的完整流程：
 *   1. 依赖图给出它依赖的仓库文件（C++ 链接闭包 + oracle 源码 + 运行期模块）；
 *   2. 把这些文件的内容摘要与上次的通过记录比较：一致就跳过，不一致才重跑；
 *   3. 重跑时执行进程先问原版 oracle（可命中 oracle/cache.ts 的缓存），再跑探针，比对、写报告；
 *   4. 结果与「这次实际读了哪些模块」一起写回状态文件，供下次与 `--changed` 使用。
 *
 * 用法（详见 README）：
 *   node tests/diff/harness/runner.ts                  # 只跑输入变过的（全绿时几乎立即结束）
 *   node tests/diff/harness/runner.ts run item_summary # 运行指定能力
 *   node tests/diff/harness/runner.ts --list           # 看会跑哪些，不运行
 *   node tests/diff/harness/runner.ts --changed origin/main   # 只跑 PR 改动影响到的
 *   node tests/diff/harness/runner.ts --status         # 每个能力上次的结果
 *   node tests/diff/harness/runner.ts build [probe...] # 只构建探针
 *
 * 退出码：0 全部通过；1 存在差异；2 环境问题（缺 probe、构建失败、缺原版数据）。
 */

import fs from "node:fs";
import path from "node:path";
import { pathToFileURL } from "node:url";

import type { RunOutcome } from "./core/types.ts";
import { digestOf, hashFiles, type HashCache } from "./core/hash.ts";
import { SHARED_INPUTS } from "./core/types.ts";
import { findHarness, HARNESSES } from "./harnesses/index.ts";
import type { DependencyEntry } from "./runner/graph.ts";
import { buildGraph, digestsFromRecords, entryDigest } from "./runner/graph.ts";
import { buildProbes } from "./runner/build.ts";
import type { ProjectLayout } from "./runner/layout.ts";
import { cpuCount, probeArtifact, projectLayout, REPO_ROOT } from "./runner/layout.ts";
import { runAll, defaultJobs, freeMemoryMiB, type ScheduledHarness, type WorkerResult } from "./runner/scheduler.ts";
import { isFresh, select } from "./runner/select.ts";
import type { HarnessRecord, RunSummary, RunnerState } from "./runner/state.ts";
import { buildRecord, loadState, saveState, statePath } from "./runner/state.ts";

/** 命令行选项。 */
interface Options {
    readonly command: "run" | "build" | "graph";
    readonly names: string[];
    readonly list: boolean;
    readonly status: boolean;
    readonly failed: boolean;
    readonly all: boolean;
    readonly fresh: boolean;
    readonly changedRef?: string;
    readonly changedFiles?: string;
    readonly jobs?: number;
    readonly noBuild: boolean;
    readonly timeoutSeconds: number;
    readonly samples: number;
    readonly buildDir?: string;
}

const USAGE = `用法：
  node tests/diff/harness/runner.ts [run] [能力...]       运行（缺省：只跑输入变过的）
  node tests/diff/harness/runner.ts build [probe...]      只构建探针
  node tests/diff/harness/runner.ts graph                 打印能力依赖图摘要

选择方式（互斥）：
  --all                   全部运行，忽略通过记录
  --failed                只跑上次失败的
  --changed <ref>         只跑受 merge base 以来改动影响的（例如 origin/main）
  --changed-files <file>  同上，改动清单来自文件（没有 git 历史的副本）
  --list                  只列出会跑哪些能力，不运行
  --status                打印每个能力上次的结果

其它：
  -j, --jobs <n>          同时运行的能力数（默认 CPU 核心数的三分之一，可用 MC_DIFF_JOBS 覆盖）
  --fresh                 不复用原版 oracle 的缓存结果
  --no-build              跳过构建（假定探针已构建）
  --timeout <sec>         单个能力超时（默认 1800）
  --samples <n>           终端里展示的差异样本条数（默认 5）
  --build-dir <dir>       构建目录（默认 <仓库>/build，可用 MC_DIFF_BUILD_DIR）`;

function parseArguments(argv: readonly string[]): Options {
    let command: Options["command"] = "run";
    const names: string[] = [];
    let list = false;
    let status = false;
    let failed = false;
    let all = false;
    let fresh = false;
    let changedRef: string | undefined;
    let changedFiles: string | undefined;
    let jobs: number | undefined;
    let noBuild = false;
    let timeoutSeconds = 1800;
    let samples = 5;
    let buildDir: string | undefined;
    let index = 0;
    const next = (flag: string): string => {
        const value = argv[++index];
        if (value === undefined) {
            throw new Error(`${flag} 需要一个参数`);
        }
        return value;
    };
    while (index < argv.length) {
        const argument = argv[index];
        if (argument === "run") {
            command = "run";
        } else if (argument === "build") {
            command = "build";
        } else if (argument === "graph") {
            command = "graph";
        } else if (argument === "--list") {
            list = true;
        } else if (argument === "--status") {
            status = true;
        } else if (argument === "--failed") {
            failed = true;
        } else if (argument === "--all") {
            all = true;
        } else if (argument === "--fresh") {
            fresh = true;
        } else if (argument === "--no-build") {
            noBuild = true;
        } else if (argument === "--changed") {
            changedRef = next(argument);
        } else if (argument === "--changed-files") {
            changedFiles = next(argument);
        } else if (argument === "-j" || argument === "--jobs") {
            jobs = Number(next(argument));
        } else if (argument === "--timeout") {
            timeoutSeconds = Number(next(argument));
        } else if (argument === "--samples") {
            samples = Number(next(argument));
        } else if (argument === "--build-dir") {
            buildDir = next(argument);
        } else if (argument === "-h" || argument === "--help") {
            console.log(USAGE);
            process.exit(0);
        } else if (argument.startsWith("-")) {
            throw new Error(`未知选项：${argument}\n\n${USAGE}`);
        } else {
            names.push(argument);
        }
        index++;
    }
    return { command, names, list, status, failed, all, fresh, changedRef, changedFiles, jobs, noBuild, timeoutSeconds, samples, buildDir };
}

/** 环境指纹：依赖摘要的一部分，工具链变化时让所有能力重跑。 */
function environmentDigest(layout: ProjectLayout): string {
    return digestOf(
        new Map([
            ["<config>", layout.config],
            ["<ninja>", layout.ninja],
            ["<node>", process.version],
            ["<platform>", `${process.platform}-${process.arch}`],
        ]),
    );
}

/**
 * 建立依赖图。
 *
 * 失败（找不到 ninja、探针未构建、依赖日志缺失）时返回原因：调用方把全部能力当作「必须运行」，
 * 并在摘要里点名——绝不把「没有依赖」当成「永远通过」。
 */
function buildDependencyEntries(layout: ProjectLayout, state: RunnerState): { entries: Map<string, DependencyEntry>; reason?: string } {
    try {
        const { loadNinjaModel, executableObjects } = requireNinja();
        const model = loadNinjaModel(layout.buildDir, layout.implConfig, layout.ninja);
        const probes = new Map(HARNESSES.map((declaration) => [declaration.name, declaration.probe ?? declaration.name]));
        const oracles = new Map(
            HARNESSES.filter((declaration) => declaration.oracle !== undefined).map((declaration) => [declaration.name, declaration.oracle as string]),
        );
        const objects = new Map<string, { objects: readonly string[]; missing: readonly string[] }>();
        for (const probe of new Set(probes.values())) {
            objects.set(probe, executableObjects(model, probeArtifact(probe, layout)));
        }
        const previous = new Map<string, DependencyEntry>();
        for (const [name, record] of Object.entries(state.harnesses)) {
            previous.set(name, { files: record.files, artifacts: record.artifacts, modules: record.modules });
        }
        const entries = buildGraph({ layout, model, probes, oracles, objects, previous });
        return { entries };
    } catch (error) {
        return { entries: new Map(), reason: (error as Error).message };
    }
}

let ninjaModule: typeof import("../tools/ninja.ts") | undefined;

/** 延迟加载构建图解析器：`--status` 这类命令完全不需要它。 */
function requireNinja(): typeof import("../tools/ninja.ts") {
    if (ninjaModule === undefined) {
        throw new Error("内部错误：ninja 模块尚未加载");
    }
    return ninjaModule;
}

/**
 * 每个能力当前的依赖摘要；依赖图不可用时为空。
 *
 * 共享输入（编排层自身代码、构建入口、版本锁定文件）只哈希一次：它们是所有能力的共同依赖，
 * 逐个能力重算会重复读同一批文件，而缓存查不到时读盘正是最贵的一步。
 */
function computeDigests(
    entries: ReadonlyMap<string, DependencyEntry>,
    cache: HashCache,
    environment: string,
    layout: ProjectLayout,
): Map<string, string> {
    const sharedFiles = hashFiles(SHARED_INPUTS.map((file) => path.join(REPO_ROOT, ...file.split("/"))), cache);
    const digests = new Map<string, string>();
    for (const [name, entry] of entries) {
        digests.set(name, entryDigest(layout, entry, cache, environment, sharedFiles));
    }
    return digests;
}

/** 可运行的能力名（声明表顺序）。 */
function availableHarnesses(): string[] {
    return HARNESSES.map((declaration) => declaration.name);
}

/** 打印选择结果。 */
function printSelection(notes: readonly string[], selected: readonly string[], available: readonly string[]): void {
    for (const note of notes) {
        console.log(`  ${note}`);
    }
    if (selected.length === 0) {
        console.log(`没有需要运行的能力（共 ${available.length} 个，全部在各自输入下通过过）`);
        return;
    }
    console.log(`本次运行 ${selected.length}/${available.length} 个能力：${selected.join("、")}`);
}

/**
 * 从上次记录取出调度用的耗时，并带上能力声明里的互斥组。
 *
 * 互斥组由能力自己声明（`declaration.exclusiveGroup`）：oracle 会占用进程外固定资源的能力
 * 必须串行，见 runner/scheduler.ts 的说明。
 */
function scheduled(selected: readonly string[], state: RunnerState): ScheduledHarness[] {
    return selected.map((name) => ({
        name,
        seconds: state.harnesses[name]?.seconds,
        exclusiveGroup: findHarness(name)?.exclusiveGroup,
    }));
}

/** 把执行结果转成状态记录。 */
function recordOutcome(entry: DependencyEntry | undefined, digest: string, outcome: RunOutcome, graphReason?: string): HarnessRecord {
    const base = entry ?? { files: [], artifacts: [], modules: outcome.modules };
    return buildRecord(base, {
        status: outcome.status,
        digest,
        oracleKey: outcome.oracleKey,
        units: outcome.units,
        matched: outcome.matched,
        knownDifferences: outcome.knownDifferences,
        seconds: Number(outcome.seconds.toFixed(1)),
        finished: new Date().toISOString(),
        failure: outcome.failure,
        graph: graphReason,
        modules: outcome.modules,
    });
}

async function main(argv: readonly string[]): Promise<number> {
    const options = parseArguments(argv);
    const layout = projectLayout(options.buildDir);
    const state = loadState(layout.stateDirectory);
    ninjaModule = await import("../tools/ninja.ts");

    if (options.command === "graph") {
        const { entries, reason } = buildDependencyEntries(layout, state);
        if (reason !== undefined) {
            console.log(`依赖图不可用：${reason}`);
            return 2;
        }
        const lines = [...entries.entries()]
            .map(([name, entry]) => `${name.padEnd(28)} 代码依赖 ${String(entry.files.length).padStart(5)}  模块 ${String(entry.modules.length).padStart(3)}`)
            .sort();
        console.log("能力".padEnd(28) + " 依赖规模");
        for (const line of lines) {
            console.log(line);
        }
        return 0;
    }

    if (options.command === "build") {
        buildProbes(layout, options.names.length > 0 ? options.names : undefined);
        return 0;
    }

    // ---- run ----
    const available = availableHarnesses();
    const environment = environmentDigest(layout);
    const sharedFiles = hashFiles(SHARED_INPUTS.map((file) => path.join(REPO_ROOT, ...file.split("/"))), state.hashes);
    const request = {
        names: options.names,
        changedRef: options.changedRef,
        changedFiles: options.changedFiles,
        failed: options.failed,
        all: options.all,
    };

    if (options.status) {
        const records = new Map<string, DependencyEntry>();
        for (const [name, record] of Object.entries(state.harnesses)) {
            records.set(name, { files: record.files, artifacts: record.artifacts, modules: record.modules });
        }
        const recorded = digestsFromRecords(layout, state, state.hashes, environment, sharedFiles);
        for (const name of available) {
            const record = state.harnesses[name];
            const fresh = record !== undefined && record.status === "pass" && record.graph === undefined &&
                recorded.get(name) === record.digest;
            const label = record === undefined ? "从未运行" : record.status === "pass" ? (fresh ? "通过（可复用）" : "通过（输入已变）") : record.status === "fail" ? "存在差异" : "运行错误";
            const seconds = record?.seconds === undefined ? "" : `${record.seconds.toFixed(1)}s`;
            console.log(`${name.padEnd(28)} ${label.padEnd(16)} ${seconds}`);
        }
        return 0;
    }

    // 快速路径：只看上次记下的依赖清单。建图要十几秒（compdb + 几千次 deps 查询），而绝大多数
    // 情况下答案都是「不用重跑」——那就只哈希一遍已记录的依赖（有缓存时约 0.3 秒）。
    let entries: Map<string, DependencyEntry> = new Map();
    let reason: string | undefined;
    let digests = digestsFromRecords(layout, state, state.hashes, environment, sharedFiles);
    let selection = select(request, { available, entries, digests, records: state.harnesses, graphAvailable: true });
    // 快速路径只能回答「摘要是否一致」。`--changed` 需要依赖图才能回答「改动影响谁」，
    // 在空图下它必然选中 0 个——那不是「无需运行」，是「还算不出来」。
    const needsGraph = selection.selected.length > 0 || request.changedRef !== undefined || request.changedFiles !== undefined;
    if (needsGraph || selection.probes.length > 0) {
        ({ entries, reason } = buildDependencyEntries(layout, state));
        digests = reason === undefined
            ? computeDigests(entries, state.hashes, environment, layout)
            : new Map();
        selection = select(request, { available, entries, digests, records: state.harnesses, graphAvailable: reason === undefined });
    }
    if (options.list) {
        printSelection(selection.notes, selection.selected, available);
        return 0;
    }
    if (selection.selected.length === 0) {
        console.log(`全部 ${available.length} 个能力都已在当前输入下通过，无需运行。`);
        return 0;
    }

    // 需要运行：先构建缺失/过期的探针，再重建依赖图（探针重建后依赖列表与产物哈希都会变）。
    if (!options.noBuild) {
        const probesToBuild = selection.probes.length > 0
            ? [...new Set(selection.probes.map((name) => HARNESSES.find((declaration) => declaration.name === name)?.probe ?? name))]
            : undefined;
        buildProbes(layout, probesToBuild);
        ({ entries, reason } = buildDependencyEntries(layout, state));
        digests = reason === undefined ? computeDigests(entries, state.hashes, environment, layout) : new Map();
        // 建图后重新算一次选择：构建可能让「输入已变」的能力重新变成「可复用」。
        selection = select(request, { available, entries, digests, records: state.harnesses, graphAvailable: reason === undefined });
        if (selection.selected.length === 0) {
            console.log(`全部 ${available.length} 个能力都已在当前输入下通过，无需运行。`);
            return 0;
        }
    }
    if (reason !== undefined) {
        console.warn(`[diff] 依赖图不可用（${reason}）：本次运行的全部能力都无法复用通过记录`);
    }
    printSelection(selection.notes, selection.selected, available);

    const jobs = options.jobs ?? defaultJobs();
    const memoryMiB = freeMemoryMiB();
    console.log(`并行 ${jobs} 个（CPU ${cpuCount()} 核，可用内存 ${memoryMiB} MiB），单个超时 ${options.timeoutSeconds}s`);

    const started = process.hrtime.bigint();
    const outcomes = new Map<string, RunOutcome>();
    const errors = new Map<string, string>();
    let finishedCount = 0;
    const total = selection.selected.length;
    const record = (name: string): void => {
        finishedCount++;
        const outcome = outcomes.get(name);
        const label = outcome?.status === "pass" ? "通过" : outcome?.status === "fail" ? "差异" : "错误";
        const seconds = outcome === undefined ? "" : `${outcome.seconds.toFixed(1)}s`;
        const gaps = outcome?.knownDifferences;
        // 已知缺口必须在结果行里点名：否则「通过」和「闸门被放宽」看起来一模一样。
        const detail = outcome === undefined
            ? errors.get(name) ?? ""
            : `${outcome.status === "pass" ? `${outcome.matched}/${outcome.units} 一致` : outcome.failure ?? ""}` +
                `${gaps !== undefined && gaps > 0 ? `；${gaps} 处已登记的已知差异` : ""}`;
        console.log(`[${String(finishedCount).padStart(String(total).length)}/${total}] ${name.padEnd(28)} ${label.padEnd(4)} ${seconds.padStart(7)}  ${detail}`);
        if (outcome !== undefined && outcome.status !== "pass" && options.samples > 0) {
            for (const line of renderSamples(outcome.reportPath, options.samples)) {
                console.log(`        ${line}`);
            }
        }
        if (outcome === undefined && errors.has(name)) {
            console.log(`        ${errors.get(name)}`);
        }
    };
    await runAll(
        scheduled(selection.selected, state),
        {
            jobs,
            timeoutSeconds: options.timeoutSeconds,
            payload: (harness) => ({ harness: harness.name, buildDir: layout.buildDir, fresh: options.fresh }),
        },
        (harness: ScheduledHarness, result: WorkerResult) => {
            if (result.outcome !== undefined) {
                outcomes.set(harness.name, result.outcome);
                // 运行期模块写回依赖条目：下次 --changed 靠它判断「改动是否影响这个能力」。
                const entry = entries.get(harness.name);
                if (entry !== undefined) {
                    entries.set(harness.name, { ...entry, modules: [...new Set(result.outcome.modules)].sort() });
                }
            } else {
                errors.set(harness.name, result.error ?? "执行进程失败");
                if (result.stderr.trim().length > 0) {
                    console.error(`[diff] ${harness.name} 执行进程 stderr：\n${result.stderr.slice(-4000)}`);
                }
            }
            const digest = digests.get(harness.name) ?? "";
            const entry = entries.get(harness.name);
            const outcome = result.outcome;
            state.harnesses[harness.name] = outcome === undefined
                ? buildRecord(entry ?? { files: [], artifacts: [], modules: [] }, {
                    status: "error",
                    digest,
                    failure: errors.get(harness.name),
                    graph: reason,
                    finished: new Date().toISOString(),
                })
                : recordOutcome(entry, digest, outcome, reason);
            record(harness.name);
            // 每完成一个就落盘：中途中断时，已完成的结论不会丢。
            saveState(layout.stateDirectory, state, liveHashPaths(entries));
        },
    );

    const elapsedSeconds = Number(process.hrtime.bigint() - started) / 1e9;
    const failures = selection.selected.filter((name) => outcomes.get(name)?.status === "fail");
    const errored = selection.selected.filter((name) => outcomes.get(name) === undefined || outcomes.get(name)?.status === "error");
    const summary: RunSummary = {
        total: selection.selected.length,
        passed: selection.selected.length - failures.length - errored.length,
        failed: failures,
        errored,
        skipped: available.length - selection.selected.length,
        seconds: elapsedSeconds,
    };
    state.lastRun = { failures: [...failures, ...errored], finished: new Date().toISOString(), seconds: elapsedSeconds };
    saveState(layout.stateDirectory, state, liveHashPaths(entries));
    printSummary(summary, layout);
    return failures.length === 0 && errored.length === 0 ? 0 : 1;
}

/** 哈希缓存要保留的路径集合。 */
function liveHashPaths(entries: ReadonlyMap<string, DependencyEntry>): Set<string> {
    const live = new Set<string>();
    for (const file of SHARED_INPUTS) {
        live.add(path.join(REPO_ROOT, ...file.split("/")));
    }
    for (const entry of entries.values()) {
        for (const file of [...entry.files, ...entry.modules]) {
            live.add(path.join(REPO_ROOT, ...file.split("/")));
        }
    }
    return live;
}

/** 从报告里取前 N 条差异，渲染成终端行。 */
function renderSamples(reportPath: string, samples: number): string[] {
    try {
        const report = JSON.parse(fs.readFileSync(reportPath, "utf8")) as {
            differences?: Array<{ key: string; expected: unknown; actual: unknown }>;
            error?: string;
        };
        if (report.error !== undefined) {
            return [report.error];
        }
        const lines: string[] = [];
        for (const difference of (report.differences ?? []).slice(0, samples)) {
            lines.push(`- ${difference.key}`);
            lines.push(`    oracle: ${JSON.stringify(difference.expected)}`);
            lines.push(`    probe : ${JSON.stringify(difference.actual)}`);
        }
        return lines;
    } catch {
        return [];
    }
}

/** 打印整体摘要。 */
function printSummary(summary: RunSummary, layout: ProjectLayout): void {
    const parts = [
        `运行 ${summary.total} 个`,
        `通过 ${summary.passed}`,
        `差异 ${summary.failed.length}`,
        `错误 ${summary.errored.length}`,
        `跳过 ${summary.skipped}`,
        `耗时 ${summary.seconds.toFixed(1)}s`,
    ];
    console.log(`\n${parts.join("，")}`);
    for (const name of summary.failed) {
        console.log(`  差异：${name}  报告：${path.join(layout.buildDir, "diff", name, "run", "report.json")}`);
    }
    for (const name of summary.errored) {
        console.log(`  错误：${name}`);
    }
    console.log(`状态文件：${statePath(layout.stateDirectory)}`);
}

// 入口判定与 tests/e2e/bot 的 run.ts 保持一致：比较 argv[1] 与 import.meta.url。
const entryUrl = process.argv[1] === undefined ? "" : pathToFileURL(process.argv[1]).href;
if (import.meta.url === entryUrl) {
    void main(process.argv.slice(2))
        .then((code) => process.exit(code))
        .catch((error: Error) => {
            console.error(error.message);
            process.exit(2);
        });
}

export { main };
export type { Options };