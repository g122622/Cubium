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

import { oracleClasspath, prepareOracle, vanillaDataPackDir } from "./oracle-env.ts";
import { REPO_ROOT } from "./vanilla.ts";
import { projectDeclaredFields } from "./comparison.ts";
import type { HarnessResult } from "./runner.ts";
import { probeExecutable, resolveBuildDir, render } from "./runner.ts";

/** Java oracle 源码目录（`tests/diff/java/dev/horizonlab/diff/*.java`）。 */
const ORACLE_SOURCE_DIR = path.join(REPO_ROOT, "tests/diff/java");
const ORACLE_PACKAGE = "dev.horizonlab.diff";

interface ExecOptions {
    readonly cwd?: string;
}

function exec(command: string, args: string[], options: ExecOptions = {}): string {
    try {
        return execFileSync(command, args, {
            cwd: options.cwd ?? REPO_ROOT,
            encoding: "utf8",
            stdio: ["ignore", "pipe", "pipe"],
            maxBuffer: 1024 * 1024 * 1024,
            // 原版 oracle 与 C++ 探针必须读同一份数据包：两侧拿到同一个
            // `MC_DIFF_VANILLA_DATAPACK`，否则「jar 内置包」与本地数据包之间的
            // 细微版本差会变成假差异。其余 oracle 不读它，拿不到目录时保持原环境。
            env: oracleEnvironment(),
        });
    } catch (error) {
        const failure = error as {
            status?: number;
            stdout?: string;
            stderr?: string;
            code?: string;
            message?: string;
        };
        // 进程根本没起来（ENOENT / EACCES）时 stdout/stderr 都是空的，只打出命令
        // 本身无法定位；这里把错误码与 message 一并带上。
        throw new Error(
            `命令失败（${failure.status ?? failure.code ?? "?"}）：${command} ${args.join(" ")}\n` +
                `${failure.message ?? ""}\n` +
                `${(failure.stdout ?? "").slice(-3000)}\n${(failure.stderr ?? "").slice(-3000)}`,
        );
    }
}

/** 子进程环境：在原进程环境上叠加数据包目录（不可用时保持原样）。 */
function oracleEnvironment(): NodeJS.ProcessEnv {
    let dataPack: string;
    try {
        dataPack = vanillaDataPackDir();
    } catch {
        return process.env;
    }
    return { ...process.env, MC_DIFF_VANILLA_DATAPACK: dataPack };
}

export interface JavaOracleOptions {
    /** oracle 类名（不含包名），如 "ItemSummaryOracle"。 */
    readonly oracle: string;
    /** 与 probe 同名的 domain，用于定位 probe 可执行文件与产物目录。 */
    readonly domain: string;
    /**
     * 输入单元；不提供时由 oracle 的 `list` 子命令给出。
     *
     * 用 oracle 自己枚举单元是更严格的做法：两侧必须对「有哪些单元」达成一致，
     * 而不是拿第三方的列表去问原版。
     */
    readonly units?: () => string[];
    /** 单元 → probe 输入行对象。 */
    readonly input: (unit: string) => unknown;
    /** 输出里标识单元的字段名。 */
    readonly key: string;
    /** 比对前对两侧输出的规范化。 */
    readonly normalise?: (value: unknown) => unknown;
    /**
     * 原版有、被测实现明确尚未支持的单元。只允许放进有据可查的已知缺口，
     * 并会在报告里单列条数；补齐实现后必须删掉，否则等于永久豁免。
     */
    readonly oracleMissing?: readonly string[];
    /**
     * 成员级缺口：输出里某个数组字段中、被测实现明确尚未注册的成员。
     *
     * 标签这类能力比的是「展开后的集合」，未实现方块/物品会在好几个标签里反复出现；
     * 把它们逐条写进 oracleMissing 既写不全也读不出重点。这里按成员名剔除，
     * 并在摘要里报告被剔除了多少条（有成员级缺口才算通过，空数组不算「一致」）。
     */
    readonly memberGaps?: { readonly field: string; readonly missing: readonly string[] };
    /**
     * 本项目独有、原版没有的成员（例如自研的 `/panel` 命令）。
     *
     * 与 `memberGaps` 方向相反：`memberGaps` 剔除「原版有、项目没有」，这里剔除
     * 「项目有、原版没有」，否则自研命令会永远表现为差异。必须显式登记，不能静默放过。
     */
    readonly projectOnly?: {
        readonly field: string;
        readonly extra: readonly string[];
        /**
         * 对象级清理：成员数组之外的派生字段（命令树里按节点路径索引的 `arguments` /
         * `components`、以及整棵树编码后的 `wire`）也要同步摘掉，否则仍然对不上。
         */
        readonly drop?: (record: Record<string, unknown>, matches: (name: string) => boolean) => Record<string, unknown>;
    };
    /**
     * 传给 oracle `run` 的额外参数（原样追加在输入/输出路径之后）。命令树这类
     * 「注册会互相影响」的能力需要它：跳过原版有、项目尚未注册的命令，才能让其余
     * 命令在同一套注册状态下比对。
     */
    readonly oracleArgs?: readonly string[];
    /**
     * 语义上无序的数组路径（`/` 连接字段名，根为 `""`）。列在这里的数组在比对前按规范序
     * 重排，元素相同、顺序不同算一致。默认空：普通数组按顺序逐元素比较。
     *
     * 典型场景：属性修饰符按槽位展开成数组，展开顺序取决于注册顺序，不承载语义。
     * 有顺序语义的数组（如命令树的遍历轨迹）不得列入。
     */
    readonly unorderedArrays?: readonly string[];
    readonly buildDir?: string;
    readonly samples?: number;
}

export interface JavaOracleOutcome {
    readonly oracle: string;
    readonly result: HarnessResult;
    readonly exitCode: number;
    /** oracle 这一侧的输出文件（JSONL），供生成参考表的脚本复用。 */
    readonly oracleOutputPath: string;
}

/**
 * 由 oracle 自己枚举单元。
 *
 * oracle 把单元写到 `<workDir>/units.txt` 而不是 stdout：原版 log4j 的配置会把 stdout
 * 拿去记日志，System.err 也会被套成 "[time] [main/INFO]: [STDERR]: " 前缀行，两个流
 * 都不能当协议通道。
 */
function listUnits(javaClass: string, classpathWithClasses: string, workDir: string): string[] {
    const unitsPath = path.join(workDir, "units.txt");
    fs.rmSync(unitsPath, { force: true });
    exec("java", ["-cp", classpathWithClasses, javaClass, "list", unitsPath], { cwd: workDir });
    return fs
        .readFileSync(unitsPath, "utf8")
        .split(/\r?\n/)
        .filter((line) => line.length > 0)
        .sort();
}

/**
 * 一个 oracle 要编译的源文件。
 *
 * 数据驱动的能力（附魔、生物变种……）要先加载原版数据包，共用
 * `DataPackRegistries.java`；javac 不会自动在同一目录里找依赖，必须显式列出。
 * 其余 oracle 只依赖原版 jar，单独一个文件即可。
 */
function oracleSources(oracle: string): string[] {
    const directory = path.join(ORACLE_SOURCE_DIR, ...ORACLE_PACKAGE.split("."));
    const sources = [path.join(directory, `${oracle}.java`)];
    // 同目录下的共享辅助类：oracle 自己不是它们时一并编译。
    // 早期只有 DataPackRegistries，战利品表又引出了 LootRoll / LootServerData / LootLevel；
    // LootTableBlockOracle / LootTableEntityOracle 复用 LootTableOracle.origin，也要一并编进来。
    const shared = [
        "DataPackRegistries.java",
        "LootRoll.java",
        "LootServerData.java",
        "LootLevel.java",
        "LootTableOracle.java",
    ];
    for (const name of shared) {
        if (name === `${oracle}.java`) {
            continue;
        }
        const candidate = path.join(directory, name);
        if (fs.existsSync(candidate)) {
            sources.push(candidate);
        }
    }
    return sources;
}

/**
 * 编译一个 oracle 源文件到共享的 classes 目录。
 *
 * 供 harness 在 `runJavaOracle` 之外单独使用 `list` 子命令时调用——否则 classes
 * 目录里可能还没有这个类。javac 幂等，重复编译没有额外开销。
 */
export function compileOracle(oracle: string, workDir: string): { classesDir: string; classpath: string } {
    fs.mkdirSync(workDir, { recursive: true });
    const environment = prepareOracle(workDir);
    const classpath = oracleClasspath(environment);
    fs.mkdirSync(environment.classesDir, { recursive: true });
    exec("javac", ["-nowarn", "-proc:none", "-cp", classpath, "-d", environment.classesDir, ...oracleSources(oracle)]);
    return { classesDir: environment.classesDir, classpath };
}

/**
 * 用原版 Java oracle 跑一次差分。
 *
 * 步骤：编译 oracle → 写 inputs.jsonl → 两侧各跑一次 → 按 key 比对。
 * 编译与运行都走本机已有的 JDK；找不到 jar 时 `prepareOracle` 会给出明确指引。
 */
export function runJavaOracle(options: JavaOracleOptions): JavaOracleOutcome {
    const started = process.hrtime.bigint();
    const buildDir = resolveBuildDir(options.buildDir);
    const workDir = path.join(buildDir, "diff", options.domain, "java");
    fs.mkdirSync(workDir, { recursive: true });

    const environment = prepareOracle(workDir);
    const classpath = oracleClasspath(environment);

    // 1. 编译 oracle（幂等：javac 会跳过没变的类）。
    fs.mkdirSync(environment.classesDir, { recursive: true });
    exec("javac", ["-nowarn", "-proc:none", "-cp", classpath, "-d", environment.classesDir, ...oracleSources(options.oracle)]);

    // 2. 单元：优先用 harness 给的；否则由 oracle 自己枚举。
    const javaClass = `${ORACLE_PACKAGE}.${options.oracle}`;
    const classpathWithClasses = `${environment.classesDir}${path.delimiter}${classpath}`;
    const enumerated = options.units?.() ?? listUnits(javaClass, classpathWithClasses, workDir);
    // 已知缺口：从比对集里剔除，但要在摘要里点名，避免变成静默豁免。
    // 注意 oracleMissing 必须覆盖枚举到的全部缺口：清单只写一部分时，剩下的缺口会
    // 以差异形式出现在报告里（方向正确：宁可报错，也不要漏比）。
    const exempt = new Set(options.oracleMissing ?? []);
    const units = enumerated.filter((unit) => !exempt.has(unit));

    // 3. 输入：两侧读同一份。
    const inputsPath = path.join(workDir, "inputs.jsonl");
    fs.writeFileSync(inputsPath, `${units.map((unit) => JSON.stringify(options.input(unit))).join("\n")}\n`);

    // 4. 原版侧。
    // cwd 指向工作目录：原版用 log4j 的 FileAppender，会把 logs/*.log.gz 写到进程 cwd，
    // 若沿用仓库根就会污染工作树。
    const oracleOutput = path.join(workDir, "oracle_outputs.jsonl");
    exec(
        "java",
        [
            "-cp",
            classpathWithClasses,
            javaClass,
            "run",
            inputsPath,
            oracleOutput,
            ...(options.oracleArgs ?? []),
        ],
        { cwd: workDir },
    );

    // 5. probe 侧。
    const probeOutput = path.join(workDir, "probe_outputs.jsonl");
    const executable = probeExecutable(options.domain, options.buildDir);
    exec(executable, ["run", inputsPath, probeOutput]);

    // 6. 比对。复用 runner.ts 的比对语义（只比 oracle 声明过的字段、按 key 对齐）。
    const expectedLines = fs.readFileSync(oracleOutput, "utf8").split(/\r?\n/).filter(Boolean);
    const actualLines = fs.readFileSync(probeOutput, "utf8").split(/\r?\n/).filter(Boolean);
    const finish = (differences: ReturnType<typeof compareLines>) => {
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
                    // 报告保留全部差异：harness 的已知缺口清单需要完整集合，
                    // 截断会让清单漏项、差异在下一轮又「新出现」。数量大时报告文件会变长，可接受。
                    differences,
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
        if (units.length !== enumerated.length) {
            const dropped = enumerated.filter((unit) => exempt.has(unit));
            // 只报条数和前几项：缺口清单可能有上百条，全打出来会把摘要淹掉。
            const sample = dropped.slice(0, 3).join("、");
            const more = dropped.length > 3 ? " 等" : "";
            console.log(`  另有 ${dropped.length} 个单元属于已知缺口（原版有、本项目尚未实现），未计入：${sample}${more}`);
        }
        return {
            oracle: options.oracle,
            result,
            exitCode: differences.length === 0 ? 0 : 1,
            oracleOutputPath: oracleOutput,
        };
    };
    if (options.memberGaps !== undefined) {
        const { field, missing } = options.memberGaps;
        const exemptMembers = new Set(missing);
        let dropped = 0;
        // 只从原版一侧剔除：probe 多出来的成员永远算差异。
        const expected = expectedLines.map((line) => {
            const parsed = JSON.parse(line) as Record<string, unknown>;
            const values = parsed[field];
            if (!Array.isArray(values)) {
                return line;
            }
            const kept = values.filter((value) => !exemptMembers.has(String(value)));
            dropped += values.length - kept.length;
            return JSON.stringify({ ...parsed, [field]: kept });
        });
        if (dropped > 0) {
            console.log(`  另有 ${dropped} 个成员属于已知缺口（原版有、本项目尚未实现），未计入比对`);
        }
        return finish(compareLines(expected, actualLines, options.key, options.normalise, options.unorderedArrays));
    }
    if (options.projectOnly !== undefined) {
        const { field, extra, drop } = options.projectOnly;
        const exemptMembers = new Set(extra);
        // 「成员」指数组里以该名字开头的项（命令树的 `tree` 每项形如 "panel biome(pos:..)"）；
        // 需要精确匹配时由调用方用 `drop` 自己处理对象里的其余字段。
        const matches = (value: string): boolean =>
            exemptMembers.has(value) || [...exemptMembers].some((name) => value.startsWith(`${name} `));
        let dropped = 0;
        // 只从探针一侧剔除：原版多出来的成员仍然算差异。
        const actual = actualLines.map((line) => {
            const parsed = JSON.parse(line) as Record<string, unknown>;
            const values = parsed[field];
            let next: Record<string, unknown> = parsed;
            if (Array.isArray(values)) {
                const kept = values.filter((value) => !matches(String(value)));
                dropped += values.length - kept.length;
                next = { ...next, [field]: kept };
            }
            return JSON.stringify(drop === undefined ? next : drop(next, matches));
        });
        // 派生字段要两侧一起清理：只清探针一侧会让原版那侧「独自存在」而报成缺席。
        const expected = drop === undefined
            ? expectedLines
            : expectedLines.map((line) => JSON.stringify(drop(JSON.parse(line) as Record<string, unknown>, matches)));
        if (dropped > 0) {
            console.log(`  另有 ${dropped} 个成员属于本项目独有能力（原版没有），未计入比对`);
        }
        return finish(compareLines(expected, actual, options.key, options.normalise, options.unorderedArrays));
    }
    return finish(compareLines(expectedLines, actualLines, options.key, options.normalise, options.unorderedArrays));
}

/**
 * 规范化后的 JSON 文本：对象键按字典序排列，数组保持原顺序。
 *
 * 比对两侧值时用它而不是直接 JSON.stringify——JSON 文本里的字段顺序取决于各自的
 * 序列化实现（C++ 的 nlohmann 默认按 alphabetic 输出，Gson 按写入顺序），
 * 键序不同而值相同的对象本来就不是差异。语义上的「数组是否有序」仍由
 * {@link projectDeclaredFields} 的 unorderedArrays 决定，这里不额外排序数组。
 */
function stableStringify(value: unknown): string {
    const normalise = (node: unknown): unknown => {
        if (Array.isArray(node)) {
            return node.map(normalise);
        }
        if (node !== null && typeof node === "object") {
            const record = node as Record<string, unknown>;
            const sorted: Record<string, unknown> = {};
            for (const key of Object.keys(record).sort()) {
                sorted[key] = normalise(record[key]);
            }
            return sorted;
        }
        return node;
    };
    return JSON.stringify(normalise(value));
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
    unorderedArrays?: readonly string[],
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
        } else if (
            // 先各自规范化（递归排序对象键）：两侧的键序不同不代表值不同，
            // 直接对 JSON 文本显然会把「同一个对象、字段顺序相反」误报成差异。
            stableStringify(projectDeclaredFields(oracleValue, oracleValue, unorderedArrays)) !==
            stableStringify(projectDeclaredFields(oracleValue, probeValue, unorderedArrays))
        ) {
            differences.push({ key: unit, expected: oracleValue, actual: probeValue });
        }
    }
    return differences;
}
