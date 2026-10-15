import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { monitorEventLoopDelay } from "node:perf_hooks";
import { makeRunId } from "../../e2e/bot/src/servers/workspace.ts";
import { waitForPortReleased } from "../../e2e/bot/src/servers/port.ts";
import { ClientPool, type StressClient } from "./clients.ts";
import { ScenarioContext } from "./context.ts";
import { prepareServer } from "./server.ts";
import { readMetrics, recoverySummary, summarizePhase, validateMetrics, type TickSample } from "./metrics.ts";
import { writeIndex, writeReport } from "./report.ts";
import { runMovement } from "./scenarios/movement.ts";
import { runPackets } from "./scenarios/packets.ts";
import { runJoins } from "./scenarios/joins.ts";
import type { Options, ScenarioId } from "./config.ts";

interface Job {
    id: string;
    scenario: ScenarioId;
    speed: number | null;
}

/** 结束时保留控制连接执行 /stop，使内存线程与指标文件正常收尾。 */
async function stopServer(context: ScenarioContext | null, control: StressClient | null): Promise<void> {
    if (context === null || control === null || control.ended || context.server.state !== "ready") return;
    // /stop 引发的全部掉线属于预期关闭，不能计作负载期间失败。
    for (const client of context.pool.clients) client.expectedEnd = true;
    control.bot.chat("/stop");
    const deadline = Date.now() + 30_000;
    while (context.server.state === "ready" && Date.now() < deadline) {
        await new Promise((resolve) => setTimeout(resolve, 100));
    }
    assert(context.server.state !== "ready", "Graceful server shutdown timed out");
    assert.equal(context.server.exitInfo?.code, 0, "Server exited unsuccessfully during shutdown");
}

/** 串行运行整个任务列表，任何档位都不会与下一档共享世界或进程。 */
export async function runStressTests(options: Options): Promise<number> {
    assert(fs.existsSync(options.server), `Server executable not found: ${options.server}`);
    const root = path.join(options.output, makeRunId());
    fs.mkdirSync(root, { recursive: true });
    fs.writeFileSync(path.join(root, "options.json"), JSON.stringify(options, null, 2));
    const jobs = options.cases.flatMap<Job>((scenario) => scenario === "movement"
        ? options.speeds.map((speed) => ({ id: `movement-${speed}${options.skipMovementSpeedCheck ? "-unchecked" : ""}`, scenario, speed }))
        : [{ id: scenario, scenario, speed: null }]);
    const results: Array<{ id: string; passed: boolean; errors: string[] }> = [];
    console.log(`Artifacts: ${root}`);
    for (const job of jobs) {
        console.log(`[${results.length + 1}/${jobs.length}] ${job.id}`);
        const directory = path.join(root, job.id);
        const { server, port } = await prepareServer(options, directory, job.scenario === "movement" && options.skipMovementSpeedCheck);
        const pool = new ClientPool(port, options);
        const errors: string[] = [];
        const lag = monitorEventLoopDelay({ resolution: 20 });
        lag.enable();
        let context: ScenarioContext | null = null;
        let control: StressClient | null = null;
        try {
            await server.start();
            control = await pool.connect("StressControl");
            context = new ScenarioContext(options, pool, server, control);
            assert.equal(control.bot.game.gameMode, "creative", "Stress flight requires creative mode");
            control.bot.physicsEnabled = false;
            // mineflayer 的类型声明尚未公开维度高度，运行时来自已完成的维度注册表握手。
            const dimension = control.bot.game as typeof control.bot.game & { minY: number; height: number };
            const altitude = dimension.minY + dimension.height + 16;
            assert(Number.isFinite(altitude), "Dimension height was not received");
            const spawn = control.bot.entity.position;
            await control.teleport(`/tp ${spawn.x} ${altitude} ${spawn.z}`, options.spawnTimeoutSeconds * 1000);
            control.write("abilities", { flags: 2 });
            await context.measure("warmup", () => context!.wait(options.warmupSeconds));
            await control.barrier(options.spawnTimeoutSeconds * 1000);
            await context.measure("baseline", () => context!.wait(options.baselineSeconds));
            if (job.scenario === "movement") await runMovement(context, job.speed!);
            else if (job.scenario === "packets") await runPackets(context);
            else await runJoins(context);
            context.check();
        } catch (error) {
            errors.push(error instanceof Error ? error.message : String(error));
        } finally {
            try { await stopServer(context, control); }
            catch (error) { errors.push(`Graceful shutdown failed: ${String(error)}`); }
            await pool.closeAll();
            await server.stop();
            lag.disable();
        }
        const phases = context?.phases ?? [];
        let samples: TickSample[] = [];
        try {
            samples = readMetrics(path.join(directory, "server-metrics.csv"));
            validateMetrics(phases, samples);
            assert(samples.length > 0, "Server emitted no tick metrics");
            if (job.scenario === "joins" && errors.length === 0) {
                const holding = phases.find((phase) => phase.name === "holding")!;
                const held = samples.filter((sample) => sample.timeMs >= holding.startMs && sample.timeMs <= holding.endMs);
                assert(held.every((sample) => sample.players === options.players + 1), "Server player count disagrees with spawned clients");
            }
        } catch (error) {
            errors.push(`Metrics: ${error instanceof Error ? error.message : String(error)}`);
        }
        const recovery = recoverySummary(phases, samples, options.memoryRatio, options.tickLimitMs);
        if (options.strictRecovery && (recovery === null || recovery.rssRecovered !== true || recovery.commitRecovered !== true || recovery.tickRecovered !== true)) {
            errors.push("Recovery exceeded configured memory / tick limits or lacks valid samples");
        }
        if (server.stopTimedOutFlag) errors.push("Server process did not exit; remaining jobs cannot start safely");
        const result = { id: job.id, passed: errors.length === 0, errors };
        results.push(result);
        const summary = {
            ...result, options, workload: context?.workload ?? {}, recovery,
            phases: phases.map((phase) => summarizePhase(phase, samples)),
            generator: { ...pool.snapshot(), eventLoopP95Ms: lag.percentile(95) / 1e6, eventLoopMaxMs: lag.max / 1e6 },
            serverExit: server.exitInfo,
            memorySource: "ProfilerManager dedicated sampling thread (1000 Hz), latest sample exported per tick",
            commitMeaning: process.platform === "win32" ? "Private commit (PagefileUsage)" : "Virtual address space (VmSize / virtual_size)",
            profilerEnabled: false,
        };
        fs.writeFileSync(path.join(directory, "summary.json"), JSON.stringify(summary, null, 2));
        fs.writeFileSync(path.join(directory, "phases.json"), JSON.stringify(phases, null, 2));
        fs.writeFileSync(path.join(directory, "events.jsonl"), pool.events.map((event) => JSON.stringify(event)).join("\n") + "\n");
        fs.writeFileSync(path.join(directory, "generator.json"), JSON.stringify(context?.generator ?? [], null, 2));
        writeReport(directory, job.id, summary, phases, summary.phases, samples);
        writeIndex(root, results);
        fs.writeFileSync(path.join(root, "summary.json"), JSON.stringify(results, null, 2));
        console.log(`  ${result.passed ? "PASS" : "FAIL"}: ${errors.join("; ") || "completed"}`);
        if (server.stopTimedOutFlag || !(await waitForPortReleased(port, 5000, 200))) {
            console.error("Server port did not release; stopping the remaining jobs");
            return 2;
        }
    }
    console.log(`Report: ${path.join(root, "index.html")}`);
    return results.every((result) => result.passed) ? 0 : 1;
}
