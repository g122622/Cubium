/*
 * run_diff.ts — 跨服务端 GameTest 自动对比工具（单文件全流程编排）。
 *
 * 让 Cubium（自研基岩服务端）与官方基岩 BDS 跑同一套 tests/integrated GameTest 用例，
 * 以官方基岩为 ground truth，自动对比两端 pass/fail 与错误差异，输出 Markdown 报告。
 *
 * 流程：
 *   1. 跑 Cubium：minecraft-server.exe --gametest --gametest_packs --gametest_report
 *      双采集 stdout（注册日志建 testName→className 映射 + PASSED/FAILED）+ JUnit XML。
 *      Cubium 跑全部注册测试，结果作测试列表来源。
 *   2. 跑基岩：spawn bedrock_server.exe，注入 level.dat Beta APIs 实验开关，
 *      用 Cubium 的测试列表逐个 'gametest run <className:testName>' 严格串行跑，
 *      解析 onTestPassed/onTestFailed 日志归一化。
 *   3. 按 fullName（className:testName）对齐两端，L1 状态/L2 错误/L3 tick 分级对比。
 *   4. 输出 Markdown 报告 + CI 退出码（0 无 P1，1 有 P1，2 流水线错误）。
 *
 * 运行方式：node scripts/test/run_diff.ts（node >= 22 原生 type stripping，ESM，零外部依赖）。
 * 单步调试：node scripts/test/run_diff.ts --step cubium|bedrock|compare
 *
 * 前置：先跑 node scripts/test/setup.ts 建好环境（junction + server.properties）。
 */

import { spawn, type ChildProcessWithoutNullStreams } from "node:child_process";
import fs from "node:fs/promises";
import { existsSync, readFileSync, writeFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const cubiumRoot = path.resolve(__dirname, "..", "..");

// ============================================================================
// 配置常量
// ============================================================================

const BEDROCK_DIR = "D:/Minecraft/bedrock-server-1.26.43.1";
const BEDROCK_EXE = path.join(BEDROCK_DIR, "bedrock_server.exe");
const BEDROCK_WORLD_DIR = path.join(BEDROCK_DIR, "worlds", "gametest-diff");

const CUBIUM_EXE = path.join(cubiumRoot, "build", "bin", "RelWithDebInfo", "minecraft-server.exe");
const PACKS_SRC = path.join(cubiumRoot, "tests", "integrated");

const REPORT_DIR = path.join(cubiumRoot, "build");
const CUBIUM_XML = path.join(REPORT_DIR, "cubium-report.xml");
const CUBIUM_STDOUT = path.join(REPORT_DIR, "cubium-stdout.log");
const BEDROCK_STDOUT = path.join(REPORT_DIR, "bedrock-stdout.log");
const DIFF_REPORT = path.join(REPORT_DIR, "gametest-diff-report.md");

const TICK_TOLERANCE = 20; // L3 tick 容差（ticks，20 = 1 秒）
const BEDROCK_STARTUP_TIMEOUT_MS = 60_000; // 基岩世界加载超时
const BEDROCK_SINGLE_TEST_TIMEOUT_MS = 120_000; // 单个 gametest run 完成超时（含 maxTicks 最长的 iron_golem_arena 810t≈40s，留余量）
const BEDROCK_TOTAL_TIMEOUT_MS = 1_200_000; // 基岩侧全部测试跑完总超时（8 测试 × 120s 上限）
const CUBIUM_RUN_TIMEOUT_MS = 600_000; // Cubium --gametest 完成超时
const BEDROCK_SHUTDOWN_TIMEOUT_MS = 15_000; // 发 stop 后等待退出超时

// 基岩世界引用的 6 个 pack（header.uuid，与 setup.ts 的 PACKS 对应，已核对各 pack manifest）。
const PACK_UUIDS = [
    "a1c1c000-0004-4a75-91b7-4cdf0a04c007", // starter
    "a1c1c000-0003-4a75-91b7-4cdf0a03c005", // mob_behavior
    "a1c1c000-0002-4a75-91b7-4cdf0a02c003", // command
    "a1c1c000-0001-4a75-91b7-4cdf0a01c001", // challenge
    "a1c1c000-0004-4a75-91b7-4cdf0a03c007", // block_behavior
    "a1c1c000-0006-4a75-91b7-4cdf0a05c011", // teleport
];

// ============================================================================
// 类型定义
// ============================================================================

type TestStatus = "passed" | "failed" | "skipped" | "timeout";

interface TestResult {
    testName: string;
    className: string | null; // Cubium 从 stdout 映射补全；基岩从日志解析，可能 null
    fullName: string; // className:testName 或 testName（className 缺失时）
    status: TestStatus;
    errorType: string | null;
    errorMessage: string | null;
    ticks: number | null;
    required: boolean; // Cubium JUnit XML 有；基岩默认 true（除非显式 optional）
}

interface NormalizedReport {
    source: "bedrock" | "cubium";
    tests: TestResult[];
    summary: { total: number; passed: number; failed: number; skipped: number };
}

interface ComparisonRow {
    fullName: string;
    bedrock: TestResult | null;
    cubium: TestResult | null;
    category: Category;
    note: string;
}

type Category =
    | "match" // 两端状态一致
    | "cubium-defect" // 基岩 pass + Cubium fail（P1）
    | "suspicious" // 基岩 fail + Cubium pass（P1，Cubium 可能误判通过）
    | "stub-defect" // Cubium 失败且错误含 stub/未实现标记
    | "error-mismatch" // 两端都失败但错误类型/信息不同（P2）
    | "tick-drift" // 两端状态一致但 tick 差异超容差（P3）
    | "one-sided" // 仅一端运行
    | "both-skipped"; // 两端都跳过

// ============================================================================
// 工具函数
// ============================================================================

function log(msg: string): void {
    console.log(`[run_diff] ${msg}`);
}

function warn(msg: string): void {
    console.warn(`[run_diff] WARN: ${msg}`);
}

function sleep(ms: number): Promise<void> {
    return new Promise((r) => setTimeout(r, ms));
}

async function writeText(p: string, content: string): Promise<void> {
    await fs.mkdir(path.dirname(p), { recursive: true });
    await fs.writeFile(p, content, "utf-8");
}

function makeFullName(className: string | null, testName: string): string {
    return className ? `${className}:${testName}` : testName;
}

// ============================================================================
// Cubium 侧：跑 --gametest + 双采集
// ============================================================================

interface CubiumCollect {
    report: NormalizedReport;
    exitCode: number;
}

/**
 * 跑 Cubium minecraft-server.exe --gametest，采集 stdout + JUnit XML，归一化。
 *
 * stdout 提取两类信息：
 *   - 注册日志 [GameTest] Registered test '<className>.<testName>' (structure=...) → 建 testName→className 映射。
 *     （见 GameTestRegistry.cpp:25。testName 不含 className，故 className 只能从此日志提取。）
 *   - 运行日志 [GameTest] PASSED/FAILED (<level>): <testName> - <msg>（LogTestReporter.cpp）。
 *     主要用于交叉校验 JUnit XML。
 *
 * JUnit XML（JUnitTestReporter.cpp:102-127）是结构化主数据源：
 *   testcase.name=testName, testcase.classname=structure（注意不是 className！）,
 *   time=秒（ticks=time*20），有 <failure>=required 失败，有 <skipped>=optional 失败。
 */
async function runCubium(): Promise<CubiumCollect> {
    log("=== Running Cubium --gametest ===");
    if (!existsSync(CUBIUM_EXE)) {
        throw new Error(`Cubium server exe not found: ${CUBIUM_EXE}. Run ./scripts/configure.sh build first.`);
    }

    await fs.mkdir(REPORT_DIR, { recursive: true });

    return new Promise<CubiumCollect>((resolve, reject) => {
        const args = [
            "--gametest",
            `--gametest_packs=${PACKS_SRC}`,
            `--gametest_report=${CUBIUM_XML}`,
        ];
        const proc = spawn(CUBIUM_EXE, args, { cwd: cubiumRoot, stdio: ["ignore", "pipe", "pipe"] });
        let stdout = "";

        const timer = setTimeout(() => {
            proc.kill("SIGKILL");
            reject(new Error(`Cubium run timed out after ${CUBIUM_RUN_TIMEOUT_MS}ms`));
        }, CUBIUM_RUN_TIMEOUT_MS);

        proc.stdout.on("data", (d: Buffer) => {
            stdout += d.toString("utf-8");
        });
        proc.stderr.on("data", (d: Buffer) => {
            stdout += d.toString("utf-8");
        });

        proc.on("close", (code) => {
            clearTimeout(timer);
            void (async () => {
                await writeText(CUBIUM_STDOUT, stdout);
                log(`Cubium exited with code ${code}. stdout → ${path.relative(cubiumRoot, CUBIUM_STDOUT)}`);

                // 解析 stdout 建 testName→className 映射。
                const classNameMap = extractClassNameMap(stdout);

                // 解析 JUnit XML（主数据源）。
                let xml: string;
                try {
                    xml = await fs.readFile(CUBIUM_XML, "utf-8");
                } catch (e) {
                    reject(new Error(`Failed to read JUnit XML '${CUBIUM_XML}': ${(e as Error).message}`));
                    return;
                }
                const tests = parseJUnitXml(xml, classNameMap);
                const report: NormalizedReport = {
                    source: "cubium",
                    tests,
                    summary: summarize(tests),
                };
                log(`Cubium: ${report.summary.total} tests, ${report.summary.passed} passed, ${report.summary.failed} failed.`);
                resolve({ report, exitCode: code ?? -1 });
            })();
        });

        proc.on("error", (err) => {
            clearTimeout(timer);
            reject(new Error(`Failed to spawn Cubium: ${err.message}`));
        });
    });
}

/**
 * 从 stdout 提取 testName → className 映射。
 * 匹配 [GameTest] Registered test '<className>.<testName>' (structure=...)。
 */
function extractClassNameMap(stdout: string): Map<string, string> {
    const map = new Map<string, string>();
    const re = /\[GameTest\] Registered test '([^.]+)\.([^']+)' \(structure=/g;
    let m: RegExpExecArray | null;
    while ((m = re.exec(stdout)) !== null) {
        const className = m[1];
        const testName = m[2];
        if (!map.has(testName)) {
            map.set(testName, className);
        }
    }
    return map;
}

/**
 * 解析 Cubium JUnit XML。零依赖手写解析（结构简单，正则提取 testcase 即可）。
 *
 * 注意 classname 属性存的是 structure 不是 className，className 从 classNameMap 补。
 */
function parseJUnitXml(xml: string, classNameMap: Map<string, string>): TestResult[] {
    const results: TestResult[] = [];
    const caseRe = /<testcase\s+([^>]*)>([\s\S]*?)<\/testcase>/g;
    let cm: RegExpExecArray | null;
    while ((cm = caseRe.exec(xml)) !== null) {
        const attrs = cm[1];
        const body = cm[2];
        const name = matchAttr(attrs, "name") ?? "";
        const timeStr = matchAttr(attrs, "time");
        const ticks = timeStr != null ? Math.round(parseFloat(timeStr) * 20) : null;

        const hasFailure = body.includes("<failure");
        const hasSkipped = body.includes("<skipped");
        const failMsg = extractFailureMessage(body);

        let status: TestStatus;
        let required = true;
        if (hasFailure) {
            status = "failed";
            required = true;
        } else if (hasSkipped) {
            status = "skipped";
            required = false;
        } else {
            status = "passed";
        }

        const className = classNameMap.get(name) ?? null;
        results.push({
            testName: name,
            className,
            fullName: makeFullName(className, name),
            status,
            errorType: hasFailure ? "Assert" : null, // JUnit XML 不含 errorType，降级用 Assert
            errorMessage: failMsg,
            ticks,
            required,
        });
    }
    return results;
}

function matchAttr(attrs: string, key: string): string | null {
    const re = new RegExp(`${key}="([^"]*)"`);
    const m = re.exec(attrs);
    return m ? m[1] : null;
}

function extractFailureMessage(body: string): string | null {
    const m = /<failure\s+message="([^"]*)"/.exec(body) || /<skipped\s+message="([^"]*)"/.exec(body);
    return m ? decodeXmlEntities(m[1]) : null;
}

function decodeXmlEntities(s: string): string {
    return s
        .replace(/&lt;/g, "<")
        .replace(/&gt;/g, ">")
        .replace(/&quot;/g, '"')
        .replace(/&apos;/g, "'")
        .replace(/&amp;/g, "&");
}

// ============================================================================
// 基岩侧：spawn + stdin 喂命令 + stdout 采集解析
// ============================================================================

/**
 * 零依赖改 Bedrock level.dat，开启 Beta APIs 实验开关。
 *
 * GameTest 框架整体属 Beta APIs 实验（@minecraft/server-gametest 只有 1.0.0-beta），
 * BDS 无法经 server.properties 开实验，必须改 level.dat 的 experiments compound：
 *   - byte tag gametest=1（Beta APIs 开关，wiki.bedrock.dev 字段名）
 *   - byte tag experiments_ever_used=1（配套标志，基岩校验）
 *   - byte tag saved_with_toggled_experiments=1（配套标志，基岩校验）
 * 仅 gametest=1 而两个标志为 0 时基岩不放行（实测）。三字段都置 1 才放行。
 *
 * Bedrock level.dat = 8 字节头（version u32 LE + length u32 LE）+ little-endian NBT payload。
 * 在 experiments compound 的 TAG_End 前插入/覆盖三字段后，必须重算 header length，
 * 否则基岩按旧 length 读取会截断注入字段（实测：不更新 length 则报 Beta APIs not enabled）。
 *
 * 零依赖手写 NBT：只处理 byte tag（experiments compound 内全是 byte tag），定位简单。
 */
function enableBetaApiExperiment(levelDatPath: string): void {
    const buf = readFileSync(levelDatPath); // 同步读，level.dat 很小（~3KB）
    if (buf.length < 8) {
        throw new Error(`level.dat too small: ${levelDatPath} (${buf.length} bytes)`);
    }
    const headerVersion = buf.readUInt32LE(0);
    const payload = buf.subarray(8); // 去头部后的 NBT payload

    // 定位 experiments compound：模式 0a 0b 00 "experiments"（TAG_Compound + 名长 11 LE + 名）。
    const probe = Buffer.concat([Buffer.from([0x0a, 0x0b, 0x00]), Buffer.from("experiments", "latin1")]);
    const compoundNameStart = payload.indexOf(probe);
    if (compoundNameStart < 0) {
        throw new Error("experiments compound not found in level.dat — file format may have changed");
    }
    let p = compoundNameStart + probe.length; // compound 内容起点
    const out = Buffer.from(payload); // 可写副本

    // 三目标字段。遍历现有 byte tag：已存在则覆盖值为 1，记录已有哪些。
    const targets = new Map<string, number>([
        ["gametest", 1],
        ["experiments_ever_used", 1],
        ["saved_with_toggled_experiments", 1],
    ]);
    const existing = new Set<string>();
    while (p < out.length && out[p] !== 0x00) {
        // experiments compound 内应全是 byte tag（0x01）。遇其他类型说明结构不符预期。
        if (out[p] !== 0x01) {
            throw new Error(`unexpected tag type 0x${out[p].toString(16)} at ${p} in experiments compound`);
        }
        const nameLen = out.readUInt16LE(p + 1);
        const name = out.subarray(p + 3, p + 3 + nameLen).toString("latin1");
        const valuePos = p + 3 + nameLen;
        existing.add(name);
        if (targets.has(name)) {
            out[valuePos] = targets.get(name)!; // 覆盖值为 1
        }
        p = p + 3 + nameLen + 1; // 跳过 tag+namelen+name+value(byte)
    }
    const tagEndPos = p; // experiments compound 的 TAG_End (0x00) 位置

    // 尚不存在的目标字段，构造 byte tag 注入。
    const toAdd: Buffer[] = [];
    for (const [name, val] of targets) {
        if (!existing.has(name)) {
            const nb = Buffer.from(name, "latin1");
            toAdd.push(Buffer.concat([
                Buffer.from([0x01, nb.length & 0xff, (nb.length >> 8) & 0xff]),
                nb,
                Buffer.from([val & 0xff]),
            ]));
        }
    }
    const newPayload = toAdd.length > 0
        ? Buffer.concat([out.subarray(0, tagEndPos), ...toAdd, out.subarray(tagEndPos)])
        : out;

    // 重组：header（原 version + 新 length）+ 新 payload。同步写。
    const header = Buffer.allocUnsafe(8);
    header.writeUInt32LE(headerVersion, 0);
    header.writeUInt32LE(newPayload.length, 4);
    writeFileSync(levelDatPath, Buffer.concat([header, newPayload]));
}

/**
 * 确保基岩专用世界的 world_behavior_packs.json 存在（引用 5 个 pack）。
 * 基岩首次启动 gametest-diff 世界后才生成该目录，故在 runset 前检查。
 */
async function ensureWorldBehaviorPacks(): Promise<void> {
    const p = path.join(BEDROCK_WORLD_DIR, "world_behavior_packs.json");
    if (existsSync(p)) {
        return;
    }
    if (!existsSync(BEDROCK_WORLD_DIR)) {
        throw new Error(
            `Bedrock world dir not found: ${BEDROCK_WORLD_DIR}. ` +
                "Manually start bedrock_server.exe once to generate the gametest-diff world, then rerun.",
        );
    }
    const content = JSON.stringify(
        PACK_UUIDS.map((pack_id) => ({ pack_id, version: [1, 0, 0] })),
        null,
        2,
    );
    await writeText(p, content);
    log(`Created ${path.relative(BEDROCK_DIR, p)}`);
}

/**
 * 准备基岩专用世界：确保世界目录存在（首次启动基岩生成）+ 注入 Beta APIs 实验开关 +
 * 写 world_behavior_packs.json。runBedrock 前调用。
 *
 * 世界首次不存在时，启动一次基岩让其生成 gametest-diff 空世界，等 "Server started" 后 stop 关服。
 * 之后每次跑都重注入实验开关（幂等：已存在则覆盖值为 1，无副作用）。
 */
async function prepareBedrockWorld(): Promise<void> {
    // 1. 世界不存在则启动基岩生成（首次）。
    if (!existsSync(BEDROCK_WORLD_DIR)) {
        log("Bedrock world gametest-diff not found, generating (首次启动)...");
        await generateBedrockWorld();
    }
    // 2. 注入 Beta APIs 实验开关（幂等）。
    const levelDat = path.join(BEDROCK_WORLD_DIR, "level.dat");
    if (!existsSync(levelDat)) {
        throw new Error(`level.dat not found after world gen: ${levelDat}`);
    }
    enableBetaApiExperiment(levelDat);
    log(`Beta APIs experiment injected into ${path.relative(BEDROCK_DIR, levelDat)}`);
    // 3. world_behavior_packs.json（首次创建，已存在则跳过）。
    await ensureWorldBehaviorPacks();
}

/** 启动基岩生成 gametest-diff 空世界，等 Server started 后 stop 关服。仅首次世界不存在时调用。 */
function generateBedrockWorld(): Promise<void> {
    return new Promise<void>((resolve, reject) => {
        const proc: ChildProcessWithoutNullStreams = spawn(BEDROCK_EXE, [], {
            cwd: BEDROCK_DIR,
            stdio: ["pipe", "pipe", "pipe"],
        });
        let stdout = "";
        const timer = setTimeout(() => {
            try { proc.kill("SIGKILL"); } catch { /* ignore */ }
            reject(new Error(`Bedrock world gen timed out after ${BEDROCK_STARTUP_TIMEOUT_MS}ms`));
        }, BEDROCK_STARTUP_TIMEOUT_MS);

        proc.stdout.on("data", (d: Buffer) => { stdout += d.toString("utf-8"); });
        proc.stderr.on("data", (d: Buffer) => { stdout += d.toString("utf-8"); });

        // 等 Server started 即关服（世界已生成）。
        const check = setInterval(() => {
            if (/Server started\./i.test(stdout)) {
                clearInterval(check);
                clearTimeout(timer);
                try { proc.stdin.write("stop\n"); } catch { /* ignore */ }
                setTimeout(() => {
                    try { proc.kill(); } catch { /* ignore */ }
                    resolve();
                }, 5000);
            }
        }, 500);

        proc.on("close", () => { clearInterval(check); clearTimeout(timer); resolve(); });
        proc.on("error", (e) => { clearTimeout(timer); reject(new Error(`Bedrock spawn failed: ${e.message}`)); });
    });
}

interface BedrockCollect {
    report: NormalizedReport;
    error: string | null; // 非 null 表示流水线错误（如报需实验开关）
}

/**
 * 启动基岩 BDS，等世界加载，逐个 'gametest run <className:testName>' 严格串行跑，
 * 每个 stdout 出现 onTestPassed/onTestFailed 再发下一个，全部跑完发 'stop' 关服。
 *
 * 测试列表由 Cubium 报告提供（Cubium 跑全部注册测试，基岩按相同列表逐个跑）。
 * 基岩 runset 默认按 tag 'suite:default' 跑，用例没打此 tag 跑不了（实测 No tests found），
 * 故用 'gametest run <fullName>' 逐个跑。fullName 必须是 className:testName 格式（基岩要求）。
 *
 * 严格串行：不等前一个完成就发下一个，会触发结构方块冲突（实测 onTestFailed:
 * Could not find StructureBlockActor）。
 */
async function runBedrock(testList: TestResult[]): Promise<BedrockCollect> {
    log(`=== Running Bedrock gametest (逐个串行, ${testList.length} tests) ===`);
    if (!existsSync(BEDROCK_EXE)) {
        throw new Error(`Bedrock exe not found: ${BEDROCK_EXE}`);
    }
    await prepareBedrockWorld();

    return new Promise<BedrockCollect>((resolve) => {
        const proc: ChildProcessWithoutNullStreams = spawn(BEDROCK_EXE, [], {
            cwd: BEDROCK_DIR,
            stdio: ["pipe", "pipe", "pipe"],
        });
        let stdout = "";
        let worldReady = false;
        let pipelineError: string | null = null;
        let resolved = false;
        let testIdx = 0; // 已发出的测试索引
        const finishedNames = new Set<string>(); // 已完成的测试 fullName

        // 增量行解析：每来一段 stdout 扫 onTestPassed/onTestFailed，按 fullName 标记完成。
        // 用扫描游标避免重复扫描全量 stdout。
        let scanCursor = 0;

        const finish = (err: string | null, reason?: string): void => {
            if (resolved) return;
            resolved = true;
            clearTimeout(startupTimer);
            clearTimeout(totalTimer);
            if (singleTestTimer) { clearTimeout(singleTestTimer); singleTestTimer = null; }
            log(`Bedrock finish triggered${reason ? ` (reason: ${reason})` : ""}${err ? ` err: ${err}` : ""}`);
            void (async () => {
                await writeText(BEDROCK_STDOUT, stdout);
                log(`Bedrock stdout → ${path.relative(cubiumRoot, BEDROCK_STDOUT)} (${stdout.length} bytes)`);
                if (err) log(`Bedrock pipeline error: ${err}`);
                try { proc.stdin.write("stop\n"); } catch { /* ignore */ }
                const killed = await new Promise<boolean>((r) => {
                    const t = setTimeout(() => { proc.kill("SIGKILL"); r(true); }, BEDROCK_SHUTDOWN_TIMEOUT_MS);
                    proc.on("close", () => { clearTimeout(t); r(false); });
                });
                if (killed) warn("Bedrock force-killed after stop timeout.");
                const report = parseBedrockLog(stdout);
                log(`Bedrock: ${report.summary.total} tests, ${report.summary.passed} passed, ${report.summary.failed} failed.`);
                resolve({ report, error: err ?? pipelineError });
            })();
        };

        const startupTimer = setTimeout(() => {
            if (!worldReady) finish(`Bedrock world did not become ready within ${BEDROCK_STARTUP_TIMEOUT_MS}ms`, "startupTimer");
        }, BEDROCK_STARTUP_TIMEOUT_MS);

        const totalTimer = setTimeout(() => {
            if (!resolved) finish(`Bedrock total timeout ${BEDROCK_TOTAL_TIMEOUT_MS}ms exceeded`, "totalTimer");
        }, BEDROCK_TOTAL_TIMEOUT_MS);

        // 单测试超时定时器（每个测试发出时启动，完成时清除）。
        let singleTestTimer: NodeJS.Timeout | null = null;
        const currentFullName = (): string | null => {
            if (testIdx === 0) return null;
            return testList[testIdx - 1]?.fullName ?? null;
        };

        const sendNext = (): void => {
            if (resolved) return;
            if (testIdx >= testList.length) {
                // 全部跑完。
                if (singleTestTimer) { clearTimeout(singleTestTimer); singleTestTimer = null; }
                finish(null, "all tests done");
                return;
            }
            const t = testList[testIdx];
            log(`Bedrock run [${testIdx + 1}/${testList.length}]: ${t.fullName}`);
            try {
                proc.stdin.write(`gametest run ${t.fullName}\n`);
            } catch (e) {
                finish(`stdin write failed: ${(e as Error).message}`, "stdin write");
                return;
            }
            // 立即递增：testIdx 语义为"已发出数量"，currentFullName() 用 testIdx-1 取当前正在跑的。
            // 必须在发命令后递增，否则 onOutput 完成检测的 `testIdx > 0` 守卫会挡住第 1 个测试
            // （testIdx=0 时进不去完成检测块，导致首个 not-found/passed/failed 永远不被处理 → 卡死）。
            testIdx++;
            // 启动单测试超时。
            if (singleTestTimer) clearTimeout(singleTestTimer);
            singleTestTimer = setTimeout(() => {
                if (resolved) return;
                const fn = currentFullName();
                if (fn && !finishedNames.has(fn)) {
                    warn(`Bedrock single test timeout: ${fn} (${BEDROCK_SINGLE_TEST_TIMEOUT_MS}ms), marking failed.`);
                    finishedNames.add(fn);
                    // 发 stopall 中断卡住的测试，然后继续下一个。
                    try { proc.stdin.write("gametest stopall\n"); } catch { /* ignore */ }
                    setTimeout(sendNext, 2000);
                }
            }, BEDROCK_SINGLE_TEST_TIMEOUT_MS);
        };

        const onOutput = (): void => {
            // 检测实验开关错误（致命，基岩侧无法跑）。
            if (pipelineError === null && /beta apis experiment is not enabled/i.test(stdout)) {
                pipelineError =
                    "Bedrock requires Beta APIs experiment toggle but it is not enabled. " +
                    "This should have been set by enableBetaApiExperiment on level.dat — " +
                    "check build/bedrock-stdout.log for details.";
                log(pipelineError);
            }

            // 等世界加载完成。
            if (!worldReady && /Server started\./i.test(stdout)) {
                worldReady = true;
                clearTimeout(startupTimer);
                log("Bedrock world ready. Sending 'gametest clearall' then逐个 run.");
                try { proc.stdin.write("gametest clearall\n"); } catch { /* ignore */ }
                testIdx = 0;
                // clearall 后稍等再发第一个测试（避免 clearall 与 run 抢结构方块）。
                setTimeout(sendNext, 1000);
            }

            // 扫描新输出找 onTestPassed/onTestFailed/找不到测试，标记当前测试完成。
            if (worldReady && testIdx > 0 && !resolved) {
                const fn = currentFullName();
                if (fn && !finishedNames.has(fn)) {
                    // 转义 fullName 中的正则元字符（测试名通常无，但保险）。
                    const esc = fn.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
                    const passedRe = new RegExp(`onTestPassed:\\s*${esc}\\b`);
                    const failedRe = new RegExp(`onTestFailed:\\s*${esc}\\s*-\\s*(.+)`);
                    // 基岩找不到该测试（如 Cubium 内置测试 ExampleTests，基岩无此注册）。
                    // 当作完成处理：该测试基岩侧缺失，对比时为 one-sided。
                    const notFoundRe = new RegExp(`Could not find test with name '${esc}'`);
                    // 只扫 scanCursor 之后的新内容（含本次 data）。
                    const newPart = stdout.slice(scanCursor);
                    if (passedRe.test(newPart) || failedRe.test(newPart) || notFoundRe.test(newPart)) {
                        if (notFoundRe.test(newPart) && !passedRe.test(newPart) && !failedRe.test(newPart)) {
                            warn(`Bedrock could not find test: ${fn} (likely Cubium-only builtin) — marking as bedrock-missing.`);
                        }
                        finishedNames.add(fn);
                        if (singleTestTimer) { clearTimeout(singleTestTimer); singleTestTimer = null; }
                        // testIdx 已在 sendNext 发命令时递增，此处不再动；sendNext 自然取下一个。
                        // 测试间稍等，避免结构方块残留冲突。
                        setTimeout(sendNext, 1500);
                    }
                }
            }
            scanCursor = stdout.length;
        };

        proc.stdout.on("data", (d: Buffer) => { stdout += d.toString("utf-8"); onOutput(); });
        proc.stderr.on("data", (d: Buffer) => { stdout += d.toString("utf-8"); onOutput(); });
        proc.on("close", () => { if (!resolved) finish("Bedrock process closed unexpectedly", "proc.close"); });
        proc.on("error", (err) => {
            resolve({
                report: { source: "bedrock", tests: [], summary: { total: 0, passed: 0, failed: 0, skipped: 0 } },
                error: `Failed to spawn Bedrock: ${err.message}`,
            });
        });
    });
}

/**
 * 解析基岩 stdout 日志归一化。基岩 GameTest 结果日志精确格式（阶段 2 实测）：
 *   onTestStructureLoaded: <className>:<testName>   — 测试开始（结构加载）
 *   onTestPassed: <className>:<testName>            — 通过
 *   onTestFailed: <className>:<testName> - <reason> — 失败，' - ' 后是原因文本
 * 日志无 tick 数，ticks 字段为 null。完成信号即每个测试的 onTestPassed/onTestFailed。
 */
function parseBedrockLog(stdout: string): NormalizedReport {
    const tests: TestResult[] = [];
    const lines = stdout.split(/\r?\n/);
    // 去掉时间戳前缀，便于匹配行内任意位置。
    const passedRe = /onTestPassed:\s*(.+)/i;
    const failedRe = /onTestFailed:\s*(.+?)\s*-\s*(.+)/i;
    const seen = new Set<string>(); // 同一测试可能多次出现（重跑），只取首次结果

    for (const line of lines) {
        let m: RegExpExecArray | null;
        if ((m = passedRe.exec(line)) !== null) {
            const raw = m[1].trim();
            const { className, testName } = splitTestName(raw);
            const fullName = makeFullName(className, testName);
            if (seen.has(fullName)) continue;
            seen.add(fullName);
            tests.push({
                testName,
                className,
                fullName,
                status: "passed",
                errorType: null,
                errorMessage: null,
                ticks: null, // 基岩日志不含 tick 数
                required: true,
            });
        } else if ((m = failedRe.exec(line)) !== null) {
            const raw = m[1].trim();
            const reason = m[2].trim();
            const { className, testName } = splitTestName(raw);
            const fullName = makeFullName(className, testName);
            if (seen.has(fullName)) continue;
            seen.add(fullName);
            tests.push({
                testName,
                className,
                fullName,
                status: "failed",
                errorType: mapBedrockErrorType(reason),
                errorMessage: reason,
                ticks: null,
                required: true,
            });
        }
    }

    return { source: "bedrock", tests, summary: summarize(tests) };
}

/** 拆分基岩日志中的测试名 'Class:name'。基岩用 className:testName 格式。 */
function splitTestName(raw: string): { className: string | null; testName: string } {
    const colon = raw.indexOf(":");
    if (colon > 0) {
        return { className: raw.slice(0, colon).trim(), testName: raw.slice(colon + 1).trim() };
    }
    return { className: null, testName: raw };
}

/** 把基岩错误文本关键词映射到 Cubium GameTestErrorType 枚举值。 */
function mapBedrockErrorType(reason: string): string {
    const r = reason.toLowerCase();
    if (r.includes("timeout") || r.includes("timed out")) return "ExecutionTimeout";
    if (r.includes("assert")) return "Assert";
    if (r.includes("exhausted") || r.includes("attempts")) return "ExhaustedAttempts";
    if (r.includes("not implemented") || r.includes("stub")) return "MethodNotImplemented";
    if (r.includes("out of bounds")) return "SimulatedPlayerOutOfBounds";
    if (r.includes("fail condition") || r.includes("failif")) return "FailConditionsMet";
    if (r.includes("waiting")) return "Waiting";
    return "Unknown";
}

// ============================================================================
// 归一化与对比
// ============================================================================

function summarize(tests: TestResult[]): { total: number; passed: number; failed: number; skipped: number } {
    let passed = 0,
        failed = 0,
        skipped = 0;
    for (const t of tests) {
        if (t.status === "passed") passed++;
        else if (t.status === "failed") failed++;
        else if (t.status === "skipped") skipped++;
    }
    return { total: tests.length, passed, failed, skipped };
}

/** errorMessage 归一化：去坐标、tick、统一大小写、trim，用于 L2 语义对比。 */
function normalizeErrorMessage(msg: string | null): string {
    if (msg == null) return "";
    return msg
        .replace(/\(x:\s*-?\d+,\s*y:\s*-?\d+,\s*z:\s*-?\d+\)/gi, "") // 去坐标
        .replace(/\b\d+\s*ticks?\b/gi, "") // 去 tick 数
        .replace(/\bat\s+\d+/gi, "") // 去 "at <tick>"
        .toLowerCase()
        .replace(/\s+/g, " ")
        .trim();
}

function isStubFailure(t: TestResult | null): boolean {
    if (t == null) return false;
    if (t.errorType === "MethodNotImplemented") return true;
    const msg = (t.errorMessage ?? "").toLowerCase();
    return msg.includes("not implemented") || msg.includes("stub") || msg.includes("todo");
}

/**
 * 按 fullName 对齐两端，分级对比。
 */
function compare(bedrock: NormalizedReport, cubium: NormalizedReport): ComparisonRow[] {
    const bedrockMap = new Map(bedrock.tests.map((t) => [t.fullName, t]));
    const cubiumMap = new Map(cubium.tests.map((t) => [t.fullName, t]));
    const allKeys = new Set<string>([...bedrockMap.keys(), ...cubiumMap.keys()]);

    const rows: ComparisonRow[] = [];
    for (const key of allKeys) {
        const b = bedrockMap.get(key) ?? null;
        const c = cubiumMap.get(key) ?? null;
        rows.push({ fullName: key, bedrock: b, cubium: c, ...classify(b, c) });
    }
    // 排序：缺陷优先，然后按 fullName。
    rows.sort((a, b) => {
        const order = (cat: Category): number => {
            const r: Category[] = [
                "cubium-defect",
                "suspicious",
                "stub-defect",
                "error-mismatch",
                "one-sided",
                "tick-drift",
                "both-skipped",
                "match",
            ];
            return r.indexOf(cat);
        };
        const d = order(a.category) - order(b.category);
        return d !== 0 ? d : a.fullName.localeCompare(b.fullName);
    });
    return rows;
}

function classify(
    b: TestResult | null,
    c: TestResult | null,
): { category: Category; note: string } {
    if (b == null && c == null) return { category: "match", note: "missing on both sides" };
    if (b == null) return { category: "one-sided", note: "only ran on Cubium" };
    if (c == null) return { category: "one-sided", note: "only ran on Bedrock" };

    // 两端都跳过。
    if (b.status === "skipped" && c.status === "skipped") {
        return { category: "both-skipped", note: "skipped on both sides" };
    }

    // L1 状态对比。
    const bOk = b.status === "passed";
    const cOk = c.status === "passed";
    if (bOk && !cOk) {
        // 基岩 pass + Cubium fail。
        if (isStubFailure(c)) return { category: "stub-defect", note: `Cubium stub: ${c.errorType ?? c.errorMessage}` };
        return { category: "cubium-defect", note: `Cubium ${c.status}: ${c.errorMessage ?? ""}` };
    }
    if (!bOk && cOk) {
        // 基岩 fail + Cubium pass。
        return { category: "suspicious", note: `Bedrock ${b.status} but Cubium passed; possible false-pass` };
    }
    if (!bOk && !cOk) {
        // 两端都失败：L2 错误对比。
        const bErr = normalizeErrorMessage(b.errorMessage);
        const cErr = normalizeErrorMessage(c.errorMessage);
        if (bErr !== cErr || b.errorType !== c.errorType) {
            return {
                category: "error-mismatch",
                note: `both failed; errorType ${b.errorType} vs ${c.errorType}`,
            };
        }
        return { category: "match", note: "both failed with matching error" };
    }

    // 两端都 passed：L3 tick 对比。
    if (b.ticks != null && c.ticks != null) {
        const drift = Math.abs(b.ticks - c.ticks);
        if (drift > TICK_TOLERANCE) {
            return { category: "tick-drift", note: `tick drift ${drift} (bedrock ${b.ticks}, cubium ${c.ticks})` };
        }
    }
    return { category: "match", note: "both passed" };
}

// ============================================================================
// 报告生成
// ============================================================================

function generateReport(
    rows: ComparisonRow[],
    bedrock: NormalizedReport,
    cubium: NormalizedReport,
    bedrockError: string | null,
): string {
    const lines: string[] = [];
    lines.push("# GameTest 跨服务端对比报告");
    lines.push("");
    lines.push(`生成时间: run_diff.ts（日期由运行时决定）`);
    lines.push("");
    lines.push("## 摘要");
    lines.push("");
    lines.push(`- 基准: 官方基岩 BDS 1.26.43.1 (ground truth)`);
    lines.push(`- 对比对象: Cubium (自研基岩服务端)`);
    lines.push(`- 基岩用例: ${bedrock.summary.total} (passed ${bedrock.summary.passed}, failed ${bedrock.summary.failed}, skipped ${bedrock.summary.skipped})`);
    lines.push(`- Cubium用例: ${cubium.summary.total} (passed ${cubium.summary.passed}, failed ${cubium.summary.failed}, skipped ${cubium.summary.skipped})`);
    if (bedrockError) {
        lines.push(`- ⚠️ 基岩侧流水线错误: ${bedrockError}`);
    }
    const counts = countByCategory(rows);
    const n = (cat: string): number => counts[cat] ?? 0;
    lines.push(`- 对比结果: 一致 ${n("match")}, Cubium缺陷 ${n("cubium-defect")}, 可疑 ${n("suspicious")}, stub缺陷 ${n("stub-defect")}, 错误不符 ${n("error-mismatch")}, 偏差 ${n("tick-drift")}, 仅一端 ${n("one-sided")}, 共同跳过 ${n("both-skipped")}`);
    lines.push("");

    // 各分类表格。
    const sections: { cat: Category; title: string }[] = [
        { cat: "cubium-defect", title: "## Cubium 缺陷 (P1: 基岩通过 Cubium失败)" },
        { cat: "suspicious", title: "## 可疑 (P1: 基岩失败 Cubium通过, 疑似误判)" },
        { cat: "stub-defect", title: "## 已知 stub 缺陷 (Cubium 未实现, 非真实缺陷)" },
        { cat: "error-mismatch", title: "## 错误不符 (P2: 两端都失败但错误不同)" },
        { cat: "tick-drift", title: "## 时序偏差 (P3: 状态一致但 tick 差异超容差)" },
        { cat: "one-sided", title: "## 仅一端运行" },
        { cat: "both-skipped", title: "## 两端共同跳过" },
        { cat: "match", title: "## 一致" },
    ];

    for (const sec of sections) {
        const secRows = rows.filter((r) => r.category === sec.cat);
        if (secRows.length === 0) continue;
        lines.push(sec.title);
        lines.push("");
        lines.push("| fullName | 基岩 | Cubium | 说明 |");
        lines.push("|---|---|---|---|");
        for (const r of secRows) {
            lines.push(
                `| ${r.fullName} | ${fmtStatus(r.bedrock)} | ${fmtStatus(r.cubium)} | ${r.note} |`,
            );
        }
        lines.push("");
    }

    return lines.join("\n");
}

function fmtStatus(t: TestResult | null): string {
    if (t == null) return "—";
    let s = t.status;
    if (t.ticks != null) s += ` (${t.ticks}t)`;
    if (t.errorMessage) s += `<br>${t.errorMessage.slice(0, 120)}`;
    return s;
}

function countByCategory(rows: ComparisonRow[]): Record<string, number> {
    const c: Record<string, number> = {};
    for (const r of rows) {
        c[r.category] = (c[r.category] ?? 0) + 1;
    }
    return c;
}

// ============================================================================
// 主流程
// ============================================================================

async function main(): Promise<void> {
    const args = process.argv.slice(2);
    const stepIdx = args.indexOf("--step");
    const step = stepIdx >= 0 ? args[stepIdx + 1] : "all";

    if (step !== "all" && step !== "cubium" && step !== "bedrock") {
        log(`Unknown --step '${step}'. Valid: all | cubium | bedrock.`);
        process.exit(2);
    }

    log(`=== GameTest cross-server diff (step=${step}) ===`);
    await fs.mkdir(REPORT_DIR, { recursive: true });

    let cubium: NormalizedReport | null = null;
    let bedrock: NormalizedReport | null = null;
    let bedrockError: string | null = null;

    // 基岩侧逐个 run 需要 Cubium 提供的测试列表（className:testName）。
    // 故 bedrock 单步模式也先跑 Cubium 拿列表。
    if (step === "all" || step === "cubium" || step === "bedrock") {
        const c = await runCubium();
        cubium = c.report;
    }
    if (step === "all" || step === "bedrock") {
        const b = await runBedrock(cubium!.tests);
        bedrock = b.report;
        bedrockError = b.error;
    }

    if (step === "cubium" || step === "bedrock") {
        // 单步模式：只跑不对比，输出采集结果摘要后退出。
        log(`Single-step '${step}' done. Reports written under ${path.relative(cubiumRoot, REPORT_DIR)}/`);
        if (bedrockError) {
            log(`Bedrock pipeline error: ${bedrockError}`);
            process.exit(2);
        }
        process.exit(0);
    }

    // 对比 + 报告。
    if (cubium == null || bedrock == null) {
        log("Internal error: missing report for comparison.");
        process.exit(2);
    }

    const rows = compare(bedrock, cubium);
    const reportMd = generateReport(rows, bedrock, cubium, bedrockError);
    await writeText(DIFF_REPORT, reportMd);
    log(`Diff report → ${path.relative(cubiumRoot, DIFF_REPORT)}`);

    // CI 退出码：2=流水线错误，1=有 P1，0=无 P1。
    const counts = countByCategory(rows);
    if (bedrockError) {
        log(`Exiting 2 (bedrock pipeline error).`);
        process.exit(2);
    }
    const p1 = (counts["cubium-defect"] ?? 0) + (counts["suspicious"] ?? 0) + (counts["stub-defect"] ?? 0);
    if (p1 > 0) {
        log(`Exiting 1 (${p1} P1 issue(s) found).`);
        process.exit(1);
    }
    log("Exiting 0 (no P1 issues).");
    process.exit(0);
}

main().catch((err: unknown) => {
    console.error(`[run_diff] FAILED: ${err instanceof Error ? err.message : String(err)}`);
    process.exit(2);
});
