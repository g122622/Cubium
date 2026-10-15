import assert from "node:assert/strict";
import { performance } from "node:perf_hooks";
import type { StressClient } from "../clients.ts";
import type { ScenarioContext } from "../context.ts";

/** 全部机器人完成 spawn 后保持在线，再集中退出并观测回落。 */
export async function runJoins(context: ScenarioContext): Promise<void> {
    const { pool, options } = context;
    const joining: Array<Promise<StressClient | null>> = [];
    const failures: string[] = [];
    await context.measure("joining", async () => {
        const started = performance.now();
        for (let index = 0; index < options.players; index += 1) {
            if (options.joinRate > 0) {
                const due = started + index * 1000 / options.joinRate;
                if (due > performance.now()) await context.wait((due - performance.now()) / 1000);
            }
            joining.push(pool.connect(`Stress${String(index + 1).padStart(4, "0")}`).catch((error) => {
                failures.push(error instanceof Error ? error.message : String(error));
                return null;
            }));
        }
        await Promise.all(joining);
    });
    const players = (await Promise.all(joining)).filter((client): client is StressClient => client !== null);
    Object.assign(context.workload, {
        requestedPlayers: options.players, joinedPlayers: players.length,
        joinRate: options.joinRate, failures, spawnLatenciesMs: players.map((client) => client.spawnMs),
    });
    assert.equal(players.length, options.players, `Not all players spawned: ${failures.join("; ")}`);
    await context.measure("holding", () => context.wait(options.holdSeconds));
    await context.measure("leaving", async () => { await Promise.all(players.map((client) => client.close())); });
    await context.measure("recovery", () => context.wait(options.recoverySeconds));
    await context.measure("verification", async () => {
        context.workload.drainPongMs = await context.control.barrier(options.spawnTimeoutSeconds * 1000);
    });
}
