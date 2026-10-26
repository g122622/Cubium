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
 * runner.ts — 差分测试的编排层：构建 probe、运行、比对、写报告。
 *
 * 一个 harness 的完整流程：
 *   1. 构建 probe（cmake --target diff_<domain>，已构建过则跳过）
 *   2. 为每个单元生成一行输入 → 跑 probe
 *   3. 把 probe 输出与参照实现的输出按 key 对齐、逐字段比较
 *   4. 写 report.json 与差异样本
 *
 * 比对语义：两侧同一 key 的输出必须是完全相同的 JSON 对象（键顺序无关，值严格相等）。
 * 本能力没有浮点数，因此不做容差；将来加物理类能力时，容差应由该能力自己的
 * comparator 决定，而不是在这里全局放宽。
 *
 * 退出码：0 全部一致；1 存在差异；2 环境问题（缺 probe、构建失败）。
 */

import { execFileSync, spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { pathToFileURL } from "node:url";

import { vanillaDataPackDir } from "./oracle-env.ts";
import { REPO_ROOT } from "./vanilla.ts";

/** 一处单元级差异。 */
export interface Difference {
    readonly key: string;
    readonly expected: unknown;
    readonly actual: unknown;
}

/** 一次 harness 运行的结果。 */
export interface HarnessResult {
    readonly harness: string;
    readonly domain: string;
    readonly units: number;
    readonly matched: number;
    readonly differences: readonly Difference[];
    readonly elapsedSeconds: number;
    readonly reportPath: string;
}

/**
 * 构建目录：显式参数 > MC_DIFF_BUILD_DIR > 仓库根的 build/。
 *
 * 产物路径由 CMake 的 RUNTIME_OUTPUT_DIRECTORY 决定（bin/diff），这里只拼这一层，
 * 不自己去猜 MSVC/多配置生成器的层级。
 */
export function resolveBuildDir(explicit?: string): string {
    return explicit ?? process.env.MC_DIFF_BUILD_DIR ?? path.join(REPO_ROOT, "build");
}

/** 本 harness 使用的构建配置，与 runner 调用 cmake 时传的 --config 一致。 */
export const BUILD_CONFIG = "RelWithDebInfo";

/**
 * 定位构建出的 probe 可执行文件。
 *
 * 候选路径按 configure 方式依次尝试：先多配置生成器的 bin/<Config>/diff，
 * 再单配置生成器的 bin/diff。不靠猜生成器，直接看文件在不在。
 */
export function probeExecutable(domain: string, buildDir?: string): string {
    const name = `diff_${domain}${process.platform === "win32" ? ".exe" : ""}`;
    const root = resolveBuildDir(buildDir);
    const candidates = [
        // 顶层 CMakeLists 把 CMAKE_RUNTIME_OUTPUT_DIRECTORY 设为 bin/，多配置生成器会
        // 再追加 <Config>/ 一层，而 RUNTIME_OUTPUT_DIRECTORY_<Config> 的赋值会**整体覆盖**
        // 该属性（不叠加），所以 probe 最终落在 bin/<Config>/，而不是 bin/<Config>/diff/。
        path.join(root, "bin", BUILD_CONFIG, name),
        path.join(root, "bin", BUILD_CONFIG, "diff", name),
        path.join(root, "bin", name),
        path.join(root, "bin", "diff", name),
    ];
    for (const candidate of candidates) {
        if (fs.existsSync(candidate)) {
            return candidate;
        }
    }
    throw new Error(
        `找不到 probe 可执行文件（已查找 ${candidates.join("、")}）；` +
            "先运行 `node tests/diff/harness/runner.ts build`，或确认 MC_DIFF_BUILD_DIR 指向正确的构建目录",
    );
}

/** 运行子命令；失败时把 stdout/stderr 带进异常，便于一眼看到原因。 */
function run(command: string[], args: string[], env?: NodeJS.ProcessEnv): string {
    try {
        return execFileSync(command[0], args, {
            cwd: REPO_ROOT,
            encoding: "utf8",
            stdio: ["ignore", "pipe", "pipe"],
            maxBuffer: 1024 * 1024 * 1024,
            // 额外的环境变量（如探针要读的数据包目录）只在宿主环境上叠加，不覆盖。
            env: env === undefined ? process.env : { ...process.env, ...env },
        });
    } catch (error) {
        const failure = error as { status?: number; stdout?: string; stderr?: string; message?: string };
        throw new Error(
            `命令失败（${failure.status ?? "?"}）：${command.join(" ")} ${args.join(" ")}\n` +
                `stdout:\n${(failure.stdout ?? "").slice(-4000)}\nstderr:\n${(failure.stderr ?? "").slice(-4000)}`,
        );
    }
}

/**
 * 用 CMake 构建差分探针。
 *
 * MC_BUILD_DIFF_PROBES 默认 OFF，因此这里显式打开；重复 configure 幂等
 * （CMake 复用缓存），其代价远小于一次完整构建。
 *
 * 在 Windows 上必须先通过 scripts/configure.sh 注入 Visual Studio 开发环境：
 * vcpkg 的 manifest 安装要调 vswhere 找 MSVC，脱离 VS 环境直接跑 `cmake` 会报
 * "Unable to find a valid Visual Studio instance"，并把失败的配置写进缓存。
 * 其余平台的 configure.sh 走 preset，参数透传给 cmake。
 */
export function buildProbes(domain?: string, buildDir?: string): void {
    const resolved = resolveBuildDir(buildDir);
    const target = domain === undefined ? "diff_probes" : `diff_${domain}`;
    if (process.platform === "win32") {
        const configure = path.join(REPO_ROOT, "scripts", "configure.sh");
        // configure.bat 把额外参数原样透传给 cmake，所以 preset 名 + -D 一起给。
        run(["bash"], [configure, "--preset", "windows-clang-relwithdebinfo", "-DMC_BUILD_DIFF_PROBES=ON"]);
    } else {
        run(["cmake"], ["-S", REPO_ROOT, "-B", resolved, "-DMC_BUILD_DIFF_PROBES=ON"]);
    }
    run(["cmake"], ["--build", resolved, "--config", BUILD_CONFIG, "--target", target]);
}

/** 读非空文本行。 */
function readLines(file: string): string[] {
    return fs.readFileSync(file, "utf8").split(/\r?\n/).filter((line) => line.length > 0);
}

/** JSON 规范化：对象键排序，数组保持顺序（数组顺序是否是语义由调用方负责）。 */
function normalise(value: unknown): unknown {
    if (Array.isArray(value)) {
        return value.map(normalise);
    }
    if (value !== null && typeof value === "object") {
        const source = value as Record<string, unknown>;
        const result: Record<string, unknown> = {};
        for (const key of Object.keys(source).sort()) {
            result[key] = normalise(source[key]);
        }
        return result;
    }
    return value;
}

function stableStringify(value: unknown): string {
    return JSON.stringify(normalise(value));
}

/**
 * 把实际输出裁剪成「参照实现声明过的字段」的形状。
 *
 * 参照值是模板：对象只保留模板里出现过的键，数组只保留模板长度的前缀。
 * 这样参照没给的信息不会变成失败，而参照给了的字段一个都不能少、不能改。
 */
function project(template: unknown, actual: unknown): unknown {
    if (Array.isArray(template)) {
        if (!Array.isArray(actual)) {
            return actual;
        }
        return template.map((item, index) => project(item, actual[index]));
    }
    if (template !== null && typeof template === "object") {
        if (actual === null || typeof actual !== "object" || Array.isArray(actual)) {
            return actual;
        }
        const source = actual as Record<string, unknown>;
        const result: Record<string, unknown> = {};
        for (const [field, expected] of Object.entries(template as Record<string, unknown>)) {
            result[field] = project(expected, source[field]);
        }
        return result;
    }
    return actual;
}

/**
 * 按 `key` 字段对齐两批 JSON 行并逐字段比较。
 *
 * 行序被刻意忽略：两侧遍历顺序（注册顺序、字典序）本来就可能不同，
 * 要验的是「同一个单元，两侧说法一致」。
 *
 * **只比参照实现声明过的字段**：参照数据没有的信息（比如 minecraft-data 不给实体
 * 眼睛高度）不是「不一致」，而是「没得比」。要求整个对象相等，会让「对方没给这个
 * 字段」也变成失败，进而逼着人去伪造对侧输出。
 */
export function compareByKey(
    expectedLines: readonly string[],
    actualLines: readonly string[],
    key: string,
    compareTransform: (value: unknown) => unknown = (value) => value,
): Difference[] {
    const index = (lines: readonly string[], side: string): Map<string, unknown> => {
        const map = new Map<string, unknown>();
        lines.forEach((line, index) => {
            let parsed: unknown;
            try {
                parsed = JSON.parse(line);
            } catch {
                throw new Error(`${side} 第 ${index + 1} 行不是合法 JSON：${line.slice(0, 200)}`);
            }
            const unit = (parsed as Record<string, unknown>)[key];
            if (typeof unit !== "string") {
                throw new Error(`${side} 第 ${index + 1} 行缺少字符串字段 ${JSON.stringify(key)}：${line.slice(0, 200)}`);
            }
            map.set(unit, compareTransform(parsed));
        });
        return map;
    };

    const expected = index(expectedLines, "参照实现");
    const actual = index(actualLines, "probe");
    const differences: Difference[] = [];
    const units = [...new Set([...expected.keys(), ...actual.keys()])].sort();
    for (const unit of units) {
        const oracleValue = expected.get(unit);
        const probeValue = actual.get(unit);
        if (oracleValue === undefined) {
            differences.push({ key: unit, expected: "<absent in oracle>", actual: probeValue });
        } else if (probeValue === undefined) {
            differences.push({ key: unit, expected: oracleValue, actual: "<absent in probe>" });
        } else if (stableStringify(project(oracleValue, oracleValue)) !== stableStringify(project(oracleValue, probeValue))) {
            differences.push({ key: unit, expected: oracleValue, actual: probeValue });
        }
    }
    return differences;
}

export interface HarnessOptions {
    readonly harness: string;
    readonly domain: string;
    /** probe `run` 的输入单元（每单元一行）。 */
    readonly units: readonly string[];
    /** 与 `units` 一一对应的参照实现输出（JSON 文本）。 */
    readonly oracleOutputs: readonly string[];
    /** 输出里标识单元的字段名，如 "block"。 */
    readonly key: string;
    /** 把单元转换成 probe 的一行输入。默认 {"<key>": unit}。 */
    readonly inputPayload?: (unit: string) => unknown;
    /** 比对前对两侧输出做的规范化（例如排序值域、归一化等价类型名）。不改变 key。 */
    readonly compareTransform?: (value: unknown) => unknown;
    readonly buildDir?: string;
}

const DEFAULT_INPUT = (key: string) => (unit: string) => ({ [key]: unit });

/**
 * 跑一次差分 harness。
 *
 * 参照输出由各 harness 自己准备好（目前来自 minecraft-data；将来接 Java oracle 时，
 * 改为把同一批输入交给 Java 主类并读其输出，本函数不变）。
 */
/**
 * 探针运行时的额外环境变量。
 *
 * 战利品 / 进阶 / 配方这类能力读的是数据包目录而不是编译期常量，探针通过
 * `MC_DIFF_VANILLA_DATAPACK` 拿到与 Java oracle 完全相同的目录。
 */
function probeEnvironment(domain: string): NodeJS.ProcessEnv {
    if (domain.startsWith("loot_table")) {
        return { MC_DIFF_VANILLA_DATAPACK: vanillaDataPackDir() };
    }
    return {};
}

/**
 * 跑探针的 `run` 子命令。
 *
 * 与 {@link run} 的区别：探针把诊断信息（数据包里的加载失败等）写 stderr、
 * 退出码用于区分「有表没加载」这类非零状态，但 stdout 文件仍然完整。
 * 直接按失败处理会让所有差异变成「探针整个跑挂了」，掩盖真正的原因；
 * 所以这里保留输出、只在没有任何输出时才算失败。
 */
function runProbe(domain: string, executable: string, inputsPath: string, outputsPath: string): void {
    const result = spawnSync(executable, ["run", inputsPath, outputsPath], {
        encoding: "utf8",
        env: { ...process.env, ...probeEnvironment(domain) },
        maxBuffer: 64 * 1024 * 1024,
    });
    if (result.error !== undefined && result.error !== null) {
        throw new Error(`探针 ${executable} 启动失败：${result.error.message}`);
    }
    const produced = fs.existsSync(outputsPath);
    if (result.status !== 0) {
        const detail = `exit=${String(result.status)}
stderr:
${(result.stderr ?? "").slice(-8000)}`;
        if (!produced) {
            throw new Error(`探针 ${executable} 运行失败（${detail}）`);
        }
        // 有输出就继续比对，把探针自身的诊断一并带出来。
        console.warn(`[${domain}] 探针退出码非零（${detail}）`);
    }
    const stderrText = (result.stderr ?? "").trim();
    if (stderrText.length > 0) {
        console.warn(`[${domain}] 探针 stderr：
${stderrText}`);
    }
}

export function runHarness(options: HarnessOptions): HarnessResult {
    const started = process.hrtime.bigint();
    const executable = probeExecutable(options.domain, options.buildDir);
    const outDir = path.join(resolveBuildDir(options.buildDir), "diff", options.harness);
    fs.mkdirSync(outDir, { recursive: true });

    const inputPayload = options.inputPayload ?? DEFAULT_INPUT(options.key);
    const inputsPath = path.join(outDir, "inputs.jsonl");
    fs.writeFileSync(inputsPath, options.units.map((unit) => JSON.stringify(inputPayload(unit))).join("\n") + "\n");

    const outputsPath = path.join(outDir, "probe_outputs.jsonl");
    runProbe(options.domain, executable, inputsPath, outputsPath);
    const probeOutputs = readLines(outputsPath);

    const differences = compareByKey(options.oracleOutputs, probeOutputs, options.key, options.compareTransform);
    const differing = new Set(differences.map((difference) => difference.key));
    const matched = options.units.length - differing.size;
    const elapsedSeconds = Number(process.hrtime.bigint() - started) / 1e9;

    const reportPath = path.join(outDir, "report.json");
    fs.writeFileSync(
        reportPath,
        `${JSON.stringify(
            {
                harness: options.harness,
                domain: options.domain,
                ok: differences.length === 0,
                units: options.units.length,
                matched,
                differenceCount: differences.length,
                differences: differences.slice(0, 200),
                elapsedSeconds: Number(elapsedSeconds.toFixed(3)),
                platform: process.platform,
                node: process.version,
            },
            null,
            2,
        )}\n`,
    );

    return {
        harness: options.harness,
        domain: options.domain,
        units: options.units.length,
        matched,
        differences,
        elapsedSeconds,
        reportPath,
    };
}

/** 把结果渲染成一段终端友好的文本。 */
export function render(result: HarnessResult, samples = 5): string {
    const lines = [
        `[${result.harness}] ${result.units} 个单元，${result.matched} 个一致，` +
            `${result.differences.length} 个不一致，耗时 ${result.elapsedSeconds.toFixed(2)}s`,
    ];
    for (const difference of result.differences.slice(0, samples)) {
        lines.push(`  - ${difference.key}`);
        lines.push(`      oracle: ${JSON.stringify(difference.expected)}`);
        lines.push(`      probe : ${JSON.stringify(difference.actual)}`);
    }
    if (result.differences.length > samples) {
        lines.push(`  ... 另有 ${result.differences.length - samples} 处，见 ${result.reportPath}`);
    }
    return lines.join("\n");
}

// ============================================================================
// HarnessSpec：声明式的 harness，新增能力时不必写一份完整的 main。
// ============================================================================

/**
 * 一个声明式 harness 的描述。
 *
 * `units()` 给出 probe `run` 的输入单元；`oracle(unit)` 给出参照实现的输出。
 * probe 侧的输出按 `key` 与参照实现对齐后逐字段比较，比较前两侧都会经过
 * `normalise`（可选的比较规范化）。
 */
export interface HarnessSpec {
    readonly name: string;
    readonly domain: string;
    /** 输出中标识单元的字段名，如 "item"。 */
    readonly key: string;
    /** probe `run` 的输入单元（每单元一行）。 */
    readonly units: () => string[];
    /** 单元 → probe 的一行输入。 */
    readonly input: (unit: string) => unknown;
    /** 单元 → 参照实现的输出（对象，序列化后参与比较）。 */
    readonly oracle: (unit: string) => unknown;
    /** 比较前对两侧输出的规范化。默认只做键排序。 */
    readonly normalise?: (value: unknown) => unknown;
    /**
     * 参照数据缺失、但被测实现有的单元。这些单元不参与比较，并在报告里单列条数。
     * 与 ORACLE_MISSING_BLOCKS 同理：每次运行都核对「参照补齐后必须删掉」，防止永久豁免。
     */
    readonly oracleMissing?: readonly string[];
}

/** 一个 harness 的执行结果（含退出码）。 */
export interface HarnessOutcome {
    readonly spec: HarnessSpec;
    readonly result: HarnessResult;
    readonly exitCode: number;
}

/**
 * 跑一个声明式 harness。
 *
 * 规范化两侧都做：只在一侧做会让差异永远消不掉（等于没比）。
 */
export function runSpec(spec: HarnessSpec, options: { samples?: number } = {}): HarnessOutcome {
    const all = spec.units();
    const missing = new Set(spec.oracleMissing ?? []);
    const units = all.filter((unit) => !missing.has(unit));
    const exempted = all.length - units.length;

    const result = runHarness({
        harness: spec.name,
        domain: spec.domain,
        units,
        oracleOutputs: units.map((unit) => JSON.stringify(spec.oracle(unit))),
        key: spec.key,
        inputPayload: spec.input,
        compareTransform: spec.normalise,
    });

    console.log(render(result, options.samples ?? 5));
    if (exempted > 0) {
        // 只报条数和前几项：缺口清单可能有上百条，全打出来会把摘要淹掉。
        const sample = [...missing].slice(0, 3).join("、");
        const more = exempted > 3 ? " 等" : "";
        console.log(`  另有 ${exempted} 个单元属于参照数据缺失项，未计入：${sample}${more}`);
    }
    return { spec, result, exitCode: result.differences.length === 0 ? 0 : 1 };
}

/**
 * 命令行入口：
 *   node tests/diff/harness/runner.ts build [domain]
 *   node tests/diff/harness/runner.ts run <harness|all>
 */
function main(argv: string[]): number | Promise<number> {
    const [command, argument] = argv;
    if (command === "build") {
        buildProbes(argument);
        return 0;
    }
    if (command === "run") {
        // 用动态 import 而不是静态 import：harnesses/index.ts 会反向 import 本文件的
        // runSpec，静态互相 import 会在模块图里形成环。动态 import 在入口文件的顶层
        // await 路径下不会结算（Node 24 的格式探测阶段会把入口按 CommonJS 解析一遍），
        // 所以这里把整个 run 分支放进一个自执行的异步闭包，顶层只等这一个 Promise。
        return (async () => {
            const { runAll, runNamed } = await import("./harnesses/index.ts");
            if (argument === undefined || argument === "all") {
                return runAll();
            }
            return runNamed(argument);
        })();
    }
    console.log("用法：\n  node tests/diff/harness/runner.ts build [domain]\n  node tests/diff/harness/runner.ts run <harness|all>");
    return 2;
}

// 不用 `import.meta.main`：Node 24 在本文件同时存在顶层 await 时，会把入口模块
// 先按 CommonJS 解析一遍做格式探测，`import.meta.main` 在该阶段为 false，
// 顶层 await 又永远不会结算，进程最终以 13 退出且不产生任何输出。
// 直接比较 argv[1] 与 import.meta.url 更稳定，也与项目里 e2e/run.ts 的写法一致。
const entryUrl = process.argv[1] === undefined ? "" : pathToFileURL(process.argv[1]).href;
if (import.meta.url === entryUrl) {
    // 同步路径（build / 帮助）直接返回数字，异步路径（run）返回 Promise；统一 await 一次。
    Promise.resolve(main(process.argv.slice(2))).then((code) => process.exit(code));
}

export { main };