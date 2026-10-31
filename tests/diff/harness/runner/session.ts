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
 * session.ts — 一次运行需要的全部上下文，以及「跑两侧、比对、写报告」的完整流程。
 *
 * 只被执行进程（worker.ts）使用：每个能力一个子进程，独占一份 session。
 *
 * 为什么每个能力一个子进程：
 *   1. 模块解析钩子（module-trace.ts）是进程级的。同一进程里连续运行两个能力时，第二个
 *      会命中 Node 的模块缓存、绕过钩子，记录的依赖列表不完整；
 *   2. 探针或 oracle 崩溃（原生崩溃、OOM）不会带走同批的其它能力；
 *   3. 状态文件由主进程统一写，执行进程只回报结果，不存在并发写。
 *
 * 执行顺序：beforeUnits → 枚举单元 → 剔除已知缺口 → 生成输入 → 原版侧（可能命中缓存）→
 * 探针侧 → 比对 → 写报告。任何一步失败都转成 `error` 状态，不抛出。
 */

import { execFileSync, spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import {
    compareLines,
    projectDeclaredFields,
    stableStringify,
    type CompareTransform,
    type Difference,
} from "../core/comparison.ts";
import type { HarnessContext, HarnessDeclaration, RunOutcome } from "../core/types.ts";
import { ORACLE_PACKAGE } from "../core/types.ts";
import type { OracleCache } from "../oracle/cache.ts";
import type { OracleEnvironment } from "../oracle/environment.ts";
import type { HashCache } from "../core/hash.ts";
import type { ProjectLayout } from "./layout.ts";
import { probeExecutable, REPO_ROOT } from "./layout.ts";
import { oracleSources } from "./graph.ts";
import { loadMinecraftData as loadMinecraftDataModule } from "../data/vanilla.ts";

/** 报告里最多保留的差异条数：完整差异仍在 report.json 里，但不能让报告随差异数无限增长。 */
const REPORT_DIFFERENCES = 200;

/** 运行器构造 session 所需的依赖（全部由主进程准备一次后传入执行进程的输入里）。 */
export interface SessionOptions {
    readonly layout: ProjectLayout;
    readonly hashes: HashCache;
    readonly oracleCache: OracleCache;
    readonly environment: OracleEnvironment;
}

/** 一次运行需要的上下文；实现 core/types.ts 的 HarnessContext。 */
export class HarnessSession implements HarnessContext {
    readonly layout: ProjectLayout;
    readonly hashes: HashCache;
    readonly oracleCache: OracleCache;
    readonly environment: NodeJS.ProcessEnv;
    private readonly oracle: OracleEnvironment;
    private minecraftDataCache: unknown;

    /** core/types.ts 的 HarnessContext：把加载器暴露给纯数据能力。 */
    readonly minecraftData = (): unknown => this.loadMinecraftData();

    constructor(options: SessionOptions) {
        this.layout = options.layout;
        this.hashes = options.hashes;
        this.oracleCache = options.oracleCache;
        this.oracle = options.environment;
        this.environment = options.environment.variables;
    }

    /**
     * 一个能力的产物目录：`<构建目录>/diff/<能力>/run|java`。
     *
     * 两类目录的生命周期不同，必须分开：
     *   - `run`：探针输入、探针输出、报告。每次都重新生成，主进程据此渲染摘要；
     *   - `java`：oracle 的类与工作文件。它同时也是依赖条目里的「构建产物」，**不得**
     *     与 `run` 混放——否则每次运行都会改变依赖摘要，让增量复用永远失效。
     */
    outputDirectory(name: string, kind: "probe" | "oracle" = "probe"): string {
        const directory = path.join(this.layout.buildDir, "diff", name, kind === "oracle" ? "java" : "run");
        fs.mkdirSync(directory, { recursive: true });
        return directory;
    }

    classesDirectory(domain: string): string {
        return path.join(this.layout.buildDir, "diff", domain, "java", "classes");
    }

    oracleClasspath(classes: string): string {
        return `${classes}${path.delimiter}${this.oracle.classpath}`;
    }

    /** 原版 jar 与依赖库（缓存键的「工具链」部分）。 */
    toolchainPaths(): string[] {
        return [this.oracle.minecraftJar, this.oracle.librariesDir];
    }

    /**
     * minecraft-data 的加载器（离线参照数据）。
     *
     * 延迟加载：只有少数纯数据能力用它，主进程与其它能力的执行进程都不该为它付出解析
     * 几十 MB npm 数据包的代价。minecraft-data 是 CommonJS 包，用 createRequire 加载。
     */
    loadMinecraftData(): unknown {
        if (this.minecraftDataCache !== undefined) {
            return this.minecraftDataCache;
        }
        // 加载路径统一由 data/vanilla.ts 负责（它知道 minecraft-data 是「按版本调用的函数」，
        // 也知道 e2e bot 的 node_modules 在哪），这里只做一次性缓存。
        const module = loadMinecraftDataModule() as (version: string) => unknown;
        this.minecraftDataCache = module("1.21.11");
        return this.minecraftDataCache;
    }

    /** 编译 oracle：幂等（javac 会跳过没有变化的类）。 */
    compileOracle(oracle: string, domain: string): void {
        const workDir = this.outputDirectory(domain, "oracle");
        const classes = this.classesDirectory(domain);
        fs.mkdirSync(classes, { recursive: true });
        const sources = oracleSources(oracle).map((file) => path.join(REPO_ROOT, ...file.split("/")));
        execFileSync(
            "javac",
            ["-nowarn", "-proc:none", "-cp", this.oracle.classpath, "-d", classes, ...sources],
            { cwd: workDir, encoding: "utf8", maxBuffer: 64 * 1024 * 1024, stdio: ["ignore", "pipe", "pipe"] },
        );
    }

    /**
     * oracle 的 `list` 子命令：枚举单元。
     *
     * 原版启动时 log4j 会占用 stdout，因此多数 oracle 把单元写进参数给定的文件
     * （`list <units.txt>`）。少数 oracle 没有文件参数、直接写 stdout（BlockStateOracle），
     * 这里两种约定都接受：文件存在就用文件，否则取 stdout。
     */
    listOracleUnits(oracle: string, domain: string): string[] {
        this.compileOracle(oracle, domain);
        const workDir = this.outputDirectory(domain, "oracle");
        const unitsPath = path.join(workDir, "units.txt");
        fs.rmSync(unitsPath, { force: true });
        const output = execFileSync(
            "java",
            ["-cp", this.oracleClasspath(this.classesDirectory(domain)), `${ORACLE_PACKAGE}.${oracle}`, "list", unitsPath],
            { cwd: workDir, encoding: "utf8", maxBuffer: 1024 * 1024 * 1024, env: this.environment, stdio: ["ignore", "pipe", "pipe"] },
        );
        if (fs.existsSync(unitsPath)) {
            return readJsonLines(unitsPath).sort();
        }
        // 不认文件参数的 oracle：单元在 stdout 里（形如 `minecraft:oak_log[axis=y]`）。
        return output.split(/\r?\n/).filter((line) => line.startsWith("minecraft:")).sort();
    }

    /** 探针的 `list` 子命令：枚举被测侧的单元。 */
    listProbeUnits(probe: string): string[] {
        const executable = probeExecutable(probe, this.layout);
        const output = execFileSync(executable, ["list"], {
            encoding: "utf8",
            maxBuffer: 1024 * 1024 * 1024,
            stdio: ["ignore", "pipe", "pipe"],
        });
        return output.split(/\r?\n/).filter((line) => line.length > 0).sort();
    }
}

/** 读非空行。 */
export function readJsonLines(file: string): string[] {
    return fs.readFileSync(file, "utf8").split(/\r?\n/).filter((line) => line.length > 0);
}

/** 运行一个能力并返回结果；任何一步失败都转成 `error` 状态，不抛出。 */
export function runDeclaration(session: HarnessSession, declaration: HarnessDeclaration): RunOutcome {
    const started = process.hrtime.bigint();
    const modules: string[] = [];
    const elapsed = (): number => Number(process.hrtime.bigint() - started) / 1e9;
    const reportPath = path.join(session.outputDirectory(declaration.name), "report.json");
    try {
        declaration.beforeUnits?.(session);
        for (const extra of declaration.extraDependencies?.() ?? []) {
            modules.push(extra);
        }

        // 1. 单元：由 oracle 枚举（它决定「原版有哪些」）、由探针枚举，或由能力自己计算。
        // 默认以 oracle 为准：它决定「原版有哪些」。两侧枚举口径不同时能力改用 computed。
        const listed = declaration.units.kind === "computed"
            ? declaration.units.units().sort()
            : declaration.oracle !== undefined
                ? session.listOracleUnits(declaration.oracle, declaration.name)
                : declaration.probe !== undefined
                    ? session.listProbeUnits(declaration.probe)
                    : (() => {
                        throw new Error("能力既没有 oracle 也没有 probe，且没有给出单元列表");
                    })();
        const enumerated = listed;
        // 2. 已知缺口：从比对集剔除，报告里点名（避免变成静默豁免）。
        // 显式清单优先；没有给时可由能力按两侧实际枚举的单元推导（静态清单会滞后于注册表）。
        const exempt = new Set(
            declaration.oracleMissing ??
                (declaration.deriveMissing !== undefined && declaration.probe !== undefined
                    ? declaration.deriveMissing(listed, session.listProbeUnits(declaration.probe))
                    : []),
        );
        const units = enumerated.filter((unit) => !exempt.has(unit));
        if (units.length === 0) {
            throw new Error("没有可比较的单元（全部被列为已知缺口）");
        }
        const inputs = units.map((unit) =>
            JSON.stringify(declaration.input === undefined ? { [declaration.key]: unit } : declaration.input(unit)),
        );

        // 3. 两侧：有 oracle 走原版 jar，否则用能力自带的参照实现。
        const expectedLines = declaration.oracle !== undefined
            ? produceOracleOutput(session, declaration, units, inputs)
            : referenceOutput(declaration, units);
        const probeLines = produceProbeOutput(session, declaration, units, inputs);

        // 4. 比对与报告。
        const compareOptions = {
            key: declaration.key,
            compareTransform: declaration.normalise,
            unorderedArrays: declaration.unorderedArrays,
            memberGaps: declaration.memberGaps,
        };
        const probeOnlyRemoved = removeProjectOnly(declaration, probeLines);
        const unfaithful = normalisationIsFaithful(expectedLines.lines, declaration);
        if (unfaithful !== undefined) {
            throw new Error(
                `normalise 改动了原版的值（单元 ${unfaithful}）：规范化只能做等价变换，不能删改字段或取值`,
            );
        }
        let summary = compareLines(expectedLines.lines, probeOnlyRemoved.lines, compareOptions);
        let oracleKey = expectedLines.key;
        let oracleOutput = expectedLines.output;
        // 复用的原版结果出现差异时，先排除缓存污染：重跑一次 oracle，重跑仍不一致才算真差异；
        // 一致则说明缓存脏了，删掉该键并且本次按重跑的结果处理。
        if (summary.differences.length > 0 && expectedLines.reused && oracleKey !== undefined) {
            console.warn(`[${declaration.name}] 复用的 oracle 结果出现差异，重新运行 oracle 以排除缓存污染`);
            runOracle(
                session,
                declaration,
                path.join(session.outputDirectory(declaration.name, "oracle"), "inputs.jsonl"),
                oracleOutput,
            );
            summary = compareLines(readJsonLines(oracleOutput), probeOnlyRemoved.lines, compareOptions);
            if (summary.differences.length === 0) {
                // 重跑一致：缓存是被污染的那一份，删掉它。
                session.oracleCache.invalidate(oracleKey);
                oracleKey = undefined;
            } else {
                session.oracleCache.store(oracleKey, oracleOutput);
            }
        }
        // 已知缺口：登记过的差异不判失败，但要单独计数并在报告里点名（不能静默豁免）。
        // 放在缓存污染复检之后：复检可能换掉整份原版输出，判定要用最终那一份。
        const known = classifyKnownDifferences(declaration, summary.differences);
        const ok = known.unexpected.length === 0;
        writeReport(reportPath, {
            harness: declaration.name,
            oracle: declaration.oracle,
            oracleReused: expectedLines.reused,
            oracleKey,
            ok,
            units: summary.units,
            matched: summary.matched,
            differenceCount: summary.differences.length,
            knownDifferenceCount: known.known.length,
            knownDifferences: known.known.slice(0, REPORT_DIFFERENCES),
            knownGapReasons: known.reasons,
            memberGapDropped: summary.memberGapDropped,
            projectOnlyDropped: probeOnlyRemoved.dropped,
            differences: known.unexpected.slice(0, REPORT_DIFFERENCES),
            unitsExempted: exempt.size,
            elapsedSeconds: Number(elapsed().toFixed(3)),
            platform: process.platform,
            node: process.version,
        });
        return {
            name: declaration.name,
            status: ok ? "pass" : "fail",
            units: summary.units,
            matched: summary.matched,
            differences: known.unexpected.length,
            knownDifferences: known.known.length,
            knownKeys: known.keys.slice(0, REPORT_DIFFERENCES),
            oracleKey,
            oracleOutput,
            reportPath,
            seconds: elapsed(),
            failure: ok
                ? undefined
                : `${known.unexpected.length} 处差异（原版 ${enumerated.length} 个单元，剔除 ${exempt.size} 个已知缺口` +
                    `${known.known.length > 0 ? `，另有 ${known.known.length} 处已登记的已知差异` : ""}）`,
            modules,
        };
    } catch (error) {
        const message = (error as Error).message;
        writeReport(reportPath, { harness: declaration.name, ok: false, error: message });
        return {
            name: declaration.name,
            status: "error",
            units: 0,
            matched: 0,
            differences: 0,
            reportPath,
            seconds: elapsed(),
            failure: message,
            modules,
        };
    }
}

/**
 * 把比对差异分成「已登记的已知缺口」与「意外差异」。
 *
 * 一条差异命中的是「完整单元键」或「单元键#字段路径」（报告里的差异已经裁到只剩不相等的
 * 顶层字段，所以这里的路径就是第一层字段名）。登记只影响**判定**，不影响报告：已知差异照样
 * 逐条写进 `knownDifferences` 并附原因，避免变成静默豁免。
 *
 * @returns `unexpected` 决定这次运行是否失败；`known` 进入报告；`reasons` 与 `keys` 进摘要
 */
function classifyKnownDifferences(
    declaration: HarnessDeclaration,
    differences: readonly Difference[],
): { known: Difference[]; unexpected: Difference[]; reasons: Record<string, string>; keys: string[] } {
    const entries = declaration.knownDifferences ?? [];
    const reasons: Record<string, string> = {};
    const known: Difference[] = [];
    const unexpected: Difference[] = [];
    for (const difference of differences) {
        const fields = difference.expected !== null && typeof difference.expected === "object"
            ? Object.keys(difference.expected as Record<string, unknown>)
            : [];
        const hit = entries.find((entry) => {
            if (entry.key !== difference.key && entry.key !== "*") {
                return false;
            }
            if (entry.field === undefined) {
                // 通配键必须限定字段，否则等于关掉整个能力。
                return entry.key !== "*";
            }
            return fields.includes(entry.field);
        });
        if (hit === undefined) {
            unexpected.push(difference);
            continue;
        }
        known.push(difference);
        reasons[hit.field === undefined ? difference.key : `${difference.key}#${hit.field}`] = hit.reason;
    }
    return { known, unexpected, reasons, keys: known.map((difference) => difference.key) };
}

interface OracleOutput {
    readonly lines: string[];
    readonly key?: string;
    readonly output: string;
    readonly reused: boolean;
}

/**
 * 跑原版 oracle（或复用缓存）。
 *
 * 缓存命中却出现差异时，不直接采信：可能是缓存被污染（环境变量、时区、未纳入指纹的本地
 * 文件）。重跑一次 oracle，重跑后仍不一致才算真正的差异；一致则说明缓存脏了，删掉它。
 */
function produceOracleOutput(
    session: HarnessSession,
    declaration: HarnessDeclaration,
    units: readonly string[],
    inputs: readonly string[],
): OracleOutput {
    const oracle = declaration.oracle as string;
    session.compileOracle(oracle, declaration.name);
    const workDir = session.outputDirectory(declaration.name, "oracle");
    const inputsPath = path.join(workDir, "inputs.jsonl");
    fs.writeFileSync(inputsPath, `${inputs.join("\n")}\n`);
    const outputPath = path.join(workDir, "oracle_outputs.jsonl");
    const key = session.oracleCache.key(
        {
            sources: oracleSources(oracle),
            toolchainPaths: session.toolchainPaths(),
            // argv 参与缓存键：oracleArgs 变了，参照侧的输出就可能变（例如多跳过一个命令）。
            argv: [oracle, ...(declaration.oracleArgs ?? [])],
            units,
            inputs,
        },
        session.hashes,
    );
    const cached = session.oracleCache.lookup(key);
    if (cached !== undefined) {
        fs.copyFileSync(cached, outputPath);
        return { lines: readJsonLines(outputPath), key, output: outputPath, reused: true };
    }
    runOracle(session, declaration, inputsPath, outputPath);
    session.oracleCache.store(key, outputPath);
    return { lines: readJsonLines(outputPath), key, output: outputPath, reused: false };
}

/** 跑一次原版 oracle，把输出写到 outputPath。 */
function runOracle(
    session: HarnessSession,
    declaration: HarnessDeclaration,
    inputsPath: string,
    outputPath: string,
): void {
    const workDir = path.dirname(outputPath);
    const oracle = declaration.oracle as string;
    try {
        execFileSync(
            "java",
            [
                "-cp",
                session.oracleClasspath(session.classesDirectory(declaration.name)),
                `${ORACLE_PACKAGE}.${oracle}`,
                "run",
                inputsPath,
                outputPath,
                ...(declaration.oracleArgs ?? []),
            ],
            {
                cwd: workDir,
                encoding: "utf8",
                env: session.environment,
                maxBuffer: 256 * 1024 * 1024,
                stdio: ["ignore", "pipe", "pipe"],
            },
        );
    } catch (error) {
        const failure = error as { status?: number; stderr?: string; stdout?: string };
        // 两侧输出都带出来：JVM 的异常栈常写在 stdout，只截 stderr 会只剩半句 "Exception in thread main"。
        const detail = `${failure.stdout ?? ""}${failure.stderr ?? ""}`.trim().slice(-3000);
        throw new Error(`原版 oracle 运行失败（退出码 ${failure.status ?? "?"}）：${detail}`);
    }
}

/** 纯数据能力的参照输出（每行一个对象）。 */
function referenceOutput(declaration: HarnessDeclaration, units: readonly string[]): OracleOutput {
    if (declaration.reference === undefined) {
        throw new Error("能力没有 oracle 也没有 reference（参照实现）");
    }
    const reference = declaration.reference;
    return { lines: units.map((unit) => JSON.stringify(reference(unit))), output: "", reused: false };
}

/** 跑一次探针：有输出文件就认为成功（非零退出码可能只是诊断信息）。 */
function produceProbeOutput(
    session: HarnessSession,
    declaration: HarnessDeclaration,
    units: readonly string[],
    inputs: readonly string[],
): string[] {
    if (declaration.probe === undefined) {
        throw new Error("能力没有 probe");
    }
    void units;
    const workDir = session.outputDirectory(declaration.name);
    const inputsPath = path.join(workDir, "inputs.jsonl");
    fs.writeFileSync(inputsPath, `${inputs.join("\n")}\n`);
    const outputPath = path.join(workDir, "probe_outputs.jsonl");
    fs.rmSync(outputPath, { force: true });
    const executable = probeExecutable(declaration.probe, session.layout);
    const result = spawnSync(executable, ["run", inputsPath, outputPath], {
        encoding: "utf8",
        env: session.environment,
        maxBuffer: 256 * 1024 * 1024,
    });
    if (result.error !== undefined && result.error !== null) {
        throw new Error(`探针启动失败：${result.error.message}`);
    }
    if (!fs.existsSync(outputPath)) {
        throw new Error(`探针没有产生输出（退出码 ${String(result.status)}）：${(result.stderr ?? "").slice(-3000)}`);
    }
    const stderrText = (result.stderr ?? "").trim();
    if (stderrText.length > 0) {
        console.warn(`[${declaration.name}] 探针 stderr：\n${stderrText.slice(-4000)}`);
    }
    return readJsonLines(outputPath);
}

/**
 * 从被测一侧剔除「本项目独有、原版没有」的成员。
 *
 * 只动一侧：参照侧保持不变（否则两侧都被删掉，差异会被一起抹掉）。这类成员显式登记在
 * 能力的 projectOnly 里，删除后必须在摘要中报告条数，避免变成静默豁免。
 */
function removeProjectOnly(
    declaration: HarnessDeclaration,
    lines: readonly string[],
): { lines: string[]; dropped: number } {
    const projectOnly = declaration.projectOnly;
    if (projectOnly === undefined) {
        return { lines: [...lines], dropped: 0 };
    }
    const exempt = new Set(projectOnly.extra);
    const matches = (value: string): boolean =>
        exempt.has(value) || [...exempt].some((name) => value.startsWith(`${name} `));
    let dropped = 0;
    const cleaned = lines.map((line) => {
        const parsed = JSON.parse(line) as Record<string, unknown>;
        const values = parsed[projectOnly.field];
        let next: Record<string, unknown> = parsed;
        if (Array.isArray(values)) {
            const kept = values.filter((value) => !matches(String(value)));
            dropped += values.length - kept.length;
            next = { ...next, [projectOnly.field]: kept };
        }
        return JSON.stringify(projectOnly.drop === undefined ? next : projectOnly.drop(next, matches));
    });
    return { lines: cleaned, dropped };
}

/**
 * 检查规范化函数是否真的只做「规范化」。
 *
 * 参照侧的每个值经过 normalise 之后必须与自身在「参照声明过的字段」上完全相等。不等说明这个
 * 函数悄悄改动了**值**（例如把某个成员删掉了），而两侧都经过它之后，差异会被同时抹掉——
 * 那是「假一致」，比没有规范化更危险。出现过这种问题：数据集在「把整棵树的可执行性改写成 false」
 * 的同时删除了部分节点，比对结果却是「全部一致」。
 *
 * @returns 第一处不一致的单元（没有则 undefined）
 */
function normalisationIsFaithful(
    lines: readonly string[],
    options: { readonly key: string; readonly normalise?: CompareTransform; readonly unorderedArrays?: readonly string[] },
    limit = 200,
): string | undefined {
    if (options.normalise === undefined) {
        return undefined;
    }
    // 规范形变换（canonicalising）本来就会改写参照侧，恒等校验对它不适用；对它要求的是
    // **确定性**：同一输入连续应用两次结果不变。否则两侧各自落到的写法可能不同，比较失去意义。
    const canonicalising = options.normalise.canonicalising === true;
    for (const line of lines.slice(0, limit)) {
        const parsed = JSON.parse(line) as Record<string, unknown>;
        const normalised = options.normalise(parsed, parsed) as Record<string, unknown>;
        if (canonicalising) {
            const twice = options.normalise(parsed, normalised) as Record<string, unknown>;
            const once = stableStringify(projectDeclaredFields(parsed, normalised, options.unorderedArrays));
            const again = stableStringify(projectDeclaredFields(parsed, twice, options.unorderedArrays));
            if (once !== again) {
                return String(parsed[options.key]);
            }
            continue;
        }
        const before = stableStringify(projectDeclaredFields(parsed, parsed, options.unorderedArrays));
        const after = stableStringify(projectDeclaredFields(parsed, normalised, options.unorderedArrays));
        if (before !== after) {
            return String(parsed[options.key]);
        }
    }
    return undefined;
}

/** 写报告文件（先写临时文件再改名，避免中断留下半个文件）。 */
export function writeReport(reportPath: string, payload: Record<string, unknown>): void {
    fs.mkdirSync(path.dirname(reportPath), { recursive: true });
    const text = `${JSON.stringify({ ...payload, finished: new Date().toISOString() }, null, 2)}\n`;
    const temporary = `${reportPath}.${process.pid}.tmp`;
    fs.writeFileSync(temporary, text);
    fs.renameSync(temporary, reportPath);
}