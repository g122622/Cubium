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

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

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
function run(command: string[], args: string[]): string {
    try {
        return execFileSync(command[0], args, {
            cwd: REPO_ROOT,
            encoding: "utf8",
            stdio: ["ignore", "pipe", "pipe"],
            maxBuffer: 1024 * 1024 * 1024,
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
 * 按 `key` 字段对齐两批 JSON 行并逐字段比较。
 *
 * 行序被刻意忽略：两侧遍历顺序（注册顺序、字典序）本来就可能不同，
 * 要验的是「同一个单元，两侧说法一致」。
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
        } else if (stableStringify(oracleValue) !== stableStringify(probeValue)) {
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
export function runHarness(options: HarnessOptions): HarnessResult {
    const started = process.hrtime.bigint();
    const executable = probeExecutable(options.domain, options.buildDir);
    const outDir = path.join(resolveBuildDir(options.buildDir), "diff", options.harness);
    fs.mkdirSync(outDir, { recursive: true });

    const inputPayload = options.inputPayload ?? DEFAULT_INPUT(options.key);
    const inputsPath = path.join(outDir, "inputs.jsonl");
    fs.writeFileSync(inputsPath, options.units.map((unit) => JSON.stringify(inputPayload(unit))).join("\n") + "\n");

    const outputsPath = path.join(outDir, "probe_outputs.jsonl");
    run([executable], ["run", inputsPath, outputsPath]);
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

/**
 * 命令行入口：
 *   node tests/diff/harness/runner.ts build [domain]
 *   node tests/diff/harness/runner.ts run [harness]
 */
async function main(argv: string[]): Promise<number> {
    const [command, argument] = argv;
    if (command === "build") {
        buildProbes(argument);
        return 0;
    }
    if (command === "run") {
        if (argument !== undefined && argument !== "text_encode") {
            console.error(`未知 harness：${argument}`);
            return 2;
        }
        const { main: textEncodeMain } = await import("./test_text_encode_parity.ts");
        return textEncodeMain([]);
    }
    console.log("用法：\n  node tests/diff/harness/runner.ts build [domain]\n  node tests/diff/harness/runner.ts run [harness]");
    return 2;
}

if (import.meta.main) {
    process.exitCode = await main(process.argv.slice(2));
}