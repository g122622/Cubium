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
 * java-oracle.ts — 编译并运行原版 Java oracle，结果与 C++ probe 比对。
 *
 * 这是 Horizon 做法的核心：参照实现的**代码也是原版的**，不是第三方转写的数据。
 * 两侧读同一份 inputs.jsonl，各写一份输出，逐字节比对。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import { oracleClasspath, prepareOracle } from "./oracle-env.ts";
import { REPO_ROOT } from "./vanilla.ts";
import type { HarnessResult } from "./runner.ts";
import { probeExecutable, resolveBuildDir, render } from "./runner.ts";

/** Java oracle 源码目录（`tests/diff/java/dev/horizonlab/diff/*.java`）。 */
const ORACLE_SOURCE_DIR = path.join(REPO_ROOT, "tests/diff/java");
const ORACLE_PACKAGE = "dev.horizonlab.diff";

function exec(command: string, args: string[], options: { cwd?: string } = {}): string {
    try {
        return execFileSync(command, args, {
            cwd: options.cwd ?? REPO_ROOT,
            encoding: "utf8",
            stdio: ["ignore", "pipe", "pipe"],
            maxBuffer: 1024 * 1024 * 1024,
        });
    } catch (error) {
        const failure = error as { status?: number; stdout?: string; stderr?: string };
        throw new Error(
            `命令失败（${failure.status ?? "?"}）：${command} ${args.join(" ")}\n` +
                `${(failure.stdout ?? "").slice(-3000)}\n${(failure.stderr ?? "").slice(-3000)}`,
        );
    }
}

export interface JavaOracleOptions {
    /** oracle 类名（不含包名），如 "ItemSummaryOracle"。 */
    readonly oracle: string;
    /** 与 probe 同名的 domain，用于定位 probe 可执行文件与产物目录。 */
    readonly domain: string;
    /** 输入单元（与 probe 的 run 输入一致）；用函数提供，避免 harness 注册时就加载数据。 */
    readonly units: () => string[];
    /** 单元 → probe 输入行对象。 */
    readonly input: (unit: string) => unknown;
    /** 输出里标识单元的字段名。 */
    readonly key: string;
    /** 比对前对两侧输出的规范化。 */
    readonly normalise?: (value: unknown) => unknown;
    readonly buildDir?: string;
    readonly samples?: number;
}

export interface JavaOracleOutcome {
    readonly oracle: string;
    readonly result: HarnessResult;
    readonly exitCode: number;
}

/**
 * 用原版 Java oracle 跑一次差分。
 *
 * 步骤：编译 oracle → 写 inputs.jsonl → 两侧各跑一次 → 按 key 比对。
 * 编译与运行都走本机已有的 JDK；找不到 jar 时 `prepareOracle` 会给出明确指引。
 */
export function runJavaOracle(options: JavaOracleOptions): JavaOracleOutcome {
    const started = process.hrtime.bigint();
    const units = options.units();
    const buildDir = resolveBuildDir(options.buildDir);
    const workDir = path.join(buildDir, "diff", options.domain, "java");
    fs.mkdirSync(workDir, { recursive: true });

    const environment = prepareOracle(workDir);
    const classpath = oracleClasspath(environment);

    // 1. 编译 oracle（幂等：javac 会跳过没变的类）。
    fs.mkdirSync(environment.classesDir, { recursive: true });
    const source = path.join(ORACLE_SOURCE_DIR, ...ORACLE_PACKAGE.split("."), `${options.oracle}.java`);
    exec("javac", ["-nowarn", "-proc:none", "-cp", classpath, "-d", environment.classesDir, source]);

    // 2. 输入：两侧读同一份。
    const inputsPath = path.join(workDir, "inputs.jsonl");
    fs.writeFileSync(inputsPath, `${units.map((unit) => JSON.stringify(options.input(unit))).join("\n")}\n`);

    // 3. 原版侧。
    // cwd 指向工作目录：原版用 log4j 的 FileAppender，会把 logs/*.log.gz 写到进程 cwd，
    // 若沿用仓库根就会污染工作树。
    const oracleOutput = path.join(workDir, "oracle_outputs.jsonl");
    exec(
        "java",
        [
            "-cp",
            `${environment.classesDir}${path.delimiter}${classpath}`,
            `${ORACLE_PACKAGE}.${options.oracle}`,
            "run",
            inputsPath,
            oracleOutput,
        ],
        { cwd: workDir },
    );

    // 4. probe 侧。
    const probeOutput = path.join(workDir, "probe_outputs.jsonl");
    const executable = probeExecutable(options.domain, options.buildDir);
    exec(executable, ["run", inputsPath, probeOutput]);

    // 5. 比对。复用 runner.ts 的比对语义（只比 oracle 声明过的字段、按 key 对齐）。
    const expectedLines = fs.readFileSync(oracleOutput, "utf8").split(/\r?\n/).filter(Boolean);
    const actualLines = fs.readFileSync(probeOutput, "utf8").split(/\r?\n/).filter(Boolean);
    const differences = compareLines(expectedLines, actualLines, options.key, options.normalise);

    const elapsedSeconds = Number(process.hrtime.bigint() - started) / 1e9;
    const reportPath = path.join(workDir, "report.json");
    fs.writeFileSync(
        reportPath,
        `${JSON.stringify(
            {
                harness: options.domain,
                oracle: options.oracle,
                ok: differences.length === 0,
                units: units.length,
                differenceCount: differences.length,
                differences: differences.slice(0, 200),
                elapsedSeconds: Number(elapsedSeconds.toFixed(3)),
            },
            null,
            2,
        )}\n`,
    );

    const result: HarnessResult = {
        harness: options.domain,
        domain: options.domain,
        units: units.length,
        matched: units.length - new Set(differences.map((d) => d.key)).size,
        differences,
        elapsedSeconds,
        reportPath,
    };
    console.log(render(result, options.samples ?? 5));
    return { oracle: options.oracle, result, exitCode: differences.length === 0 ? 0 : 1 };
}

/**
 * 逐行比对：oracle 的每一行都是模板，probe 必须声明同样的字段、同样的值。
 *
 * 与 runner.ts 的 compareByKey 同语义，但输入来自两个文件而不是内存数组。
 */
function compareLines(
    expectedLines: readonly string[],
    actualLines: readonly string[],
    key: string,
    transform?: (value: unknown) => unknown,
): HarnessResult["differences"] extends readonly (infer D)[] ? D[] : never {
    type Difference = { key: string; expected: unknown; actual: unknown };
    const apply = transform ?? ((value: unknown) => value);

    const index = (lines: readonly string[]): Map<string, unknown> => {
        const map = new Map<string, unknown>();
        for (const line of lines) {
            const parsed = JSON.parse(line) as Record<string, unknown>;
            map.set(String(parsed[key]), apply(parsed));
        }
        return map;
    };
    const expected = index(expectedLines);
    const actual = index(actualLines);

    const differences: Difference[] = [];
    for (const unit of [...new Set([...expected.keys(), ...actual.keys()])].sort()) {
        const oracleValue = expected.get(unit);
        const probeValue = actual.get(unit);
        if (oracleValue === undefined) {
            differences.push({ key: unit, expected: "<absent in oracle>", actual: probeValue });
        } else if (probeValue === undefined) {
            differences.push({ key: unit, expected: oracleValue, actual: "<absent in probe>" });
        } else if (JSON.stringify(project(oracleValue, oracleValue)) !== JSON.stringify(project(oracleValue, probeValue))) {
            differences.push({ key: unit, expected: oracleValue, actual: probeValue });
        }
    }
    return differences;
}

/** 只保留 oracle 声明过的字段（同 runner.ts 的 project）。 */
function project(template: unknown, actual: unknown): unknown {
    if (template !== null && typeof template === "object" && !Array.isArray(template)) {
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