/*
 * run-gametests.ts — GameTest 外层统一协调者（全量跑 + 失败隔离重跑 + JUnit XML 聚合）。
 *
 * 职责：
 *   1. 启动 minecraft-server --gametest 全量跑进程，等待完成。
 *   2. 解析 JUnit XML（JUnitTestReporter 输出格式）收集失败测试列表。
 *   3. 对每个失败的测试启动独立重跑进程（独立世界目录，隔离环境）。
 *   4. 聚合最终结果：重跑通过算通过；输出最终 JUnit XML + CI 退出码。
 *
 * 用法（node >= 22 原生 type stripping，ESM，零外部依赖）：
 *   node scripts/test/run-gametests.ts [--shards=N] [--filter=PATTERN] [--out-dir=PATH]
 *                                      [--server=PATH] [--max-retries=N] [--dry-run]
 *
 * 示例：
 *   # 全量跑（单进程）
 *   node scripts/test/run-gametests.ts
 *
 *   # 只跑 mob_behavior 前缀的测试
 *   node scripts/test/run-gametests.ts --filter='mob_*'
 *
 *   # 指定 server 二进制路径（默认 build/bin/RelWithDebInfo/minecraft-server.exe）
 *   node scripts/test/run-gametests.ts --server=./build/bin/RelWithDebInfo/minecraft-server.exe
 *
 * 依赖：仅 Node.js 标准库（fs/path/child_process），无第三方依赖。
 * JUnit XML 解析用手写轻量正则（本项目 JUnitTestReporter 输出格式固定，无需完整 XML parser）。
 */

import { spawn } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const REPO_ROOT = path.resolve(__dirname, "..", "..");

// ============================================================================
// 类型定义
// ============================================================================

/** 单个测试用例的 JUnit XML 解析结果。 */
interface TestcaseResult {
    /** 测试名（testName，不含 className 前缀）。 */
    name: string;
    /** 测试类名。 */
    classname: string;
    /** 测试耗时（秒）。 */
    time: number;
    /** 是否失败（有 <failure> 子元素）。 */
    failed: boolean;
    /** 失败消息（escaped）。 */
    message: string;
}

/** 解析后的 CLI 参数。 */
interface CliArgs {
    [key: string]: string;
}

// ============================================================================
// CLI 参数解析
// ============================================================================

/**
 * 解析 CLI 参数。
 *
 * 支持 `--key=value` 与 `--key`（等价 `--key=true`）两种形式。
 *
 * @returns 解析后的参数字典
 */
function parseArgs(): CliArgs {
    const args: CliArgs = {};
    for (let i = 2; i < process.argv.length; ++i) {
        const arg = process.argv[i];
        if (arg.startsWith("--")) {
            const body = arg.slice(2);
            const eq = body.indexOf("=");
            if (eq >= 0) {
                args[body.slice(0, eq)] = body.slice(eq + 1);
            } else {
                args[body] = "true";
            }
        }
    }
    return args;
}

// ============================================================================
// 轻量 JUnit XML 解析
// ============================================================================

/**
 * 解析 JUnitTestReporter 输出的 JUnit XML。
 *
 * 预期格式（JUnitTestReporter::_writeXml）：
 *   <testsuites>
 *     <testsuite name="GameTest" tests="N" failures="F" skipped="S">
 *       <testcase name="..." classname="..." time="...">
 *         [<failure message="..."/> | <skipped message="..."/>]
 *       </testcase>
 *     </testsuite>
 *   </testsuites>
 *
 * @param xmlText JUnit XML 文本
 * @returns 解析后的测试结果列表
 */
function parseJunitXml(xmlText: string): TestcaseResult[] {
    const results: TestcaseResult[] = [];

    // 逐 testcase 块解析
    const testcaseRegex = /<testcase\s+([^>]*?)\s*>\s*([\s\S]*?)\s*<\/testcase>/g;
    const selfClosingRegex = /<testcase\s+([^>]*?)\/>/g;

    /**
     * 解析单个 testcase。
     *
     * @param attrString testcase 标签的属性字符串
     * @param innerXml testcase 内部 XML（failure/skipped 子元素）
     */
    function parseTestcase(attrString: string, innerXml: string): void {
        const nameMatch = attrString.match(/name="([^"]*)"/);
        const clsMatch = attrString.match(/classname="([^"]*)"/);
        const timeMatch = attrString.match(/time="([^"]*)"/);
        if (!nameMatch) {
            return;
        }
        const failureMatch = innerXml.match(/<failure\s+([^>]*?)\/>/);
        const skippedMatch = innerXml.match(/<skipped\s+([^>]*?)\/>/);
        let failed = false;
        let message = "";
        if (failureMatch) {
            failed = true;
            const msgMatch = failureMatch[1].match(/message="([^"]*)"/);
            if (msgMatch) {
                message = unescapeXml(msgMatch[1]);
            }
        } else if (skippedMatch) {
            // optional 失败 → skipped：不算失败（不计入退出码）
            failed = false;
            const msgMatch = skippedMatch[1].match(/message="([^"]*)"/);
            if (msgMatch) {
                message = unescapeXml(msgMatch[1]);
            }
        }
        results.push({
            name: unescapeXml(nameMatch[1]),
            classname: clsMatch ? unescapeXml(clsMatch[1]) : "",
            time: timeMatch ? Number.parseFloat(timeMatch[1]) : 0.0,
            failed,
            message,
        });
    }

    let m: RegExpExecArray | null;
    while ((m = testcaseRegex.exec(xmlText)) !== null) {
        parseTestcase(m[1], m[2]);
    }
    while ((m = selfClosingRegex.exec(xmlText)) !== null) {
        parseTestcase(m[1], "");
    }
    return results;
}

/** XML 实体反转义（对齐 JUnitTestReporter::_xmlEscape 的五种实体）。 */
function unescapeXml(s: string): string {
    return s
        .replace(/&lt;/g, "<")
        .replace(/&gt;/g, ">")
        .replace(/&quot;/g, '"')
        .replace(/&apos;/g, "'")
        .replace(/&amp;/g, "&");
}

/** XML 实体转义。 */
function escapeXml(s: string): string {
    return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;").replace(/'/g, "&apos;");
}

// ============================================================================
// JUnit XML 生成（聚合最终结果）
// ============================================================================

/**
 * 生成聚合后的 JUnit XML。
 *
 * @param results 聚合后的测试结果列表
 * @param rerunInfo 重跑轮次结果
 * @returns JUnit XML 文本
 */
function generateJunitXml(
    results: TestcaseResult[],
    rerunInfo: { rerunPassed: string[]; rerunFailed: string[] },
): string {
    const failedCount = results.filter((r) => r.failed).length;
    const lines: string[] = [];
    lines.push('<?xml version="1.0" encoding="UTF-8"?>');
    lines.push("<testsuites>");
    lines.push(`  <testsuite name="GameTest" tests="${results.length}" failures="${failedCount}">`);
    for (const r of results) {
        const name = escapeXml(r.name);
        const cls = escapeXml(r.classname);
        lines.push(`    <testcase name="${name}" classname="${cls}" time="${r.time}">`);
        if (r.failed) {
            const msg = escapeXml(r.message);
            lines.push(`      <failure message="${msg}"/>`);
        }
        lines.push("    </testcase>");
    }
    lines.push("  </testsuite>");

    // 重跑结果：单独一个 testsuite 记录重跑轮次结果
    if (rerunInfo.rerunPassed.length > 0 || rerunInfo.rerunFailed.length > 0) {
        lines.push(`  <testsuite name="GameTestRerun" tests="${rerunInfo.rerunPassed.length + rerunInfo.rerunFailed.length}" failures="${rerunInfo.rerunFailed.length}">`);
        for (const name of rerunInfo.rerunPassed) {
            lines.push(`    <testcase name="${escapeXml(name)}" classname="rerun-passed" time="0"/>`);
        }
        for (const name of rerunInfo.rerunFailed) {
            lines.push(`    <testcase name="${escapeXml(name)}" classname="rerun-failed" time="0">`);
            lines.push(`      <failure message="failed in rerun"/>`);
            lines.push(`    </testcase>`);
        }
        lines.push("  </testsuite>");
    }

    lines.push("</testsuites>");
    return lines.join("\n") + "\n";
}

// ============================================================================
// Server 进程启动
// ============================================================================

/**
 * 启动一个 minecraft-server --gametest 进程并等待完成。
 *
 * @param serverPath minecraft-server 二进制路径
 * @param opts 启动选项
 * @returns 进程退出码
 */
function runServer(
    serverPath: string,
    opts: { filter?: string; reportPath: string; worldName: string },
): Promise<number> {
    return new Promise((resolve, reject) => {
        const args: string[] = ["--gametest"];
        if (opts.filter !== undefined && opts.filter !== "") {
            args.push(`--gametest-tests=${opts.filter}`);
        }
        args.push(`--gametest-report=${opts.reportPath}`);
        // 每进程独立世界名，避免并行进程并发写同一世界目录
        args.push(`--gametest-world=${opts.worldName}`);

        console.log(`[gametest-runner] spawning: ${serverPath} ${args.join(" ")}`);
        const child = spawn(serverPath, args, { stdio: "inherit" });
        child.on("error", reject);
        child.on("close", (code) => {
            resolve(code ?? -1);
        });
    });
}

// ============================================================================
// 主流程
// ============================================================================

async function main(): Promise<void> {
    const args = parseArgs();
    const shards = Number.parseInt(args["shards"] ?? "1", 10);
    const filter = args["filter"] ?? "";
    const serverPath = args["server"] ?? path.join(REPO_ROOT, "build", "bin", "RelWithDebInfo", "minecraft-server.exe");
    const outDir = args["out-dir"] ?? path.join(REPO_ROOT, "build", "gametest-reports");
    const dryRun = args["dry-run"] === "true";

    if (!fs.existsSync(serverPath)) {
        console.error(`[gametest-runner] server binary not found: ${serverPath}`);
        process.exit(2);
    }

    fs.mkdirSync(outDir, { recursive: true });
    const timestamp = Date.now();

    // === 第 1 轮：全量跑（或按 filter）===
    const round1Report = path.join(outDir, `gametest-round1-${timestamp}.xml`);
    console.log(`[gametest-runner] === Round 1: full run ===`);
    console.log(`[gametest-runner] shards=${shards}, filter='${filter}'`);

    if (shards <= 1) {
        const exitCode = dryRun ? 0 : await runServer(serverPath, {
            filter,
            reportPath: round1Report,
            worldName: "gametest",
        });
        console.log(`[gametest-runner] Round 1 exit code: ${exitCode}`);
    } else {
        // 多进程并行分片
        /** @type {Promise<number>[]} */
        const promises: Promise<number>[] = [];
        for (let i = 0; i < shards; ++i) {
            const reportPath = path.join(outDir, `gametest-round1-${timestamp}-shard${i}.xml`);
            const p = dryRun ? Promise.resolve(0) : runServer(serverPath, {
                filter,
                reportPath: reportPath,
                worldName: `gametest-shard-${i}`,
            });
            promises.push(p);
        }
        const exitCodes = await Promise.all(promises);
        console.log(`[gametest-runner] Round 1 shard exit codes: ${exitCodes.join(", ")}`);
    }

    // === 解析第 1 轮结果，收集失败测试列表 ===
    /** @type {TestcaseResult[]} */
    let allResults: TestcaseResult[];
    if (shards <= 1) {
        if (!fs.existsSync(round1Report)) {
            console.error(`[gametest-runner] Round 1 report not found: ${round1Report}`);
            process.exit(2);
        }
        allResults = parseJunitXml(fs.readFileSync(round1Report, "utf-8"));
    } else {
        allResults = [];
        for (let i = 0; i < shards; ++i) {
            const reportPath = path.join(outDir, `gametest-round1-${timestamp}-shard${i}.xml`);
            if (!fs.existsSync(reportPath)) {
                console.warn(`[gametest-runner] shard ${i} report missing, skip: ${reportPath}`);
                continue;
            }
            allResults.push(...parseJunitXml(fs.readFileSync(reportPath, "utf-8")));
        }
    }

    const failedTests = allResults.filter((r) => r.failed);
    console.log(`[gametest-runner] Round 1: total=${allResults.length}, failed=${failedTests.length}`);

    if (failedTests.length === 0) {
        // 无失败：直接输出最终报告 + 退出码 0
        const finalReport = path.join(outDir, `gametest-final-${timestamp}.xml`);
        fs.writeFileSync(finalReport, generateJunitXml(allResults, { rerunPassed: [], rerunFailed: [] }));
        console.log(`[gametest-runner] All tests passed. Final report: ${finalReport}`);
        process.exit(0);
    }

    // === 第 2 轮：失败测试隔离重跑（每个失败测试独占进程）===
    console.log(`[gametest-runner] === Round 2: rerun ${failedTests.length} failed test(s) in isolation ===`);
    /** @type {string[]} */
    const rerunPassed: string[] = [];
    /** @type {string[]} */
    const rerunFailed: string[] = [];

    for (const failed of failedTests) {
        const rerunReport = path.join(outDir, `gametest-rerun-${timestamp}-${failed.name}.xml`);
        console.log(`[gametest-runner] rerunning failed test: ${failed.name}`);
        const exitCode = dryRun ? 0 : await runServer(serverPath, {
            filter: failed.name,
            reportPath: rerunReport,
            worldName: `gametest-rerun-${failed.name}`,
        });
        if (exitCode === 0) {
            rerunPassed.push(failed.name);
            console.log(`[gametest-runner]   rerun PASSED: ${failed.name}`);
        } else {
            rerunFailed.push(failed.name);
            console.log(`[gametest-runner]   rerun FAILED: ${failed.name}`);
        }
    }

    // === 聚合最终结果 ===
    // 重跑通过的测试，最终判定为通过（从失败集合中移除）
    const rerunPassedSet = new Set(rerunPassed);
    const finalResults = allResults.map((r) => {
        if (r.failed && rerunPassedSet.has(r.name)) {
            return { ...r, failed: false, message: `passed in rerun (first-run failure: ${r.message})` };
        }
        return r;
    });

    const finalFailedCount = finalResults.filter((r) => r.failed).length;
    const finalReport = path.join(outDir, `gametest-final-${timestamp}.xml`);
    fs.writeFileSync(finalReport, generateJunitXml(finalResults, { rerunPassed, rerunFailed }));

    console.log(`[gametest-runner] Final: total=${finalResults.length}, failed=${finalFailedCount}`);
    console.log(`[gametest-runner] Final report: ${finalReport}`);
    process.exit(finalFailedCount > 0 ? 1 : 0);
}

main().catch((err) => {
    console.error("[gametest-runner] fatal:", err);
    process.exit(2);
});
