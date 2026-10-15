import { performance } from "node:perf_hooks";
import { setTimeout as delay } from "node:timers/promises";
import type { Options } from "./config.ts";
import type { ClientPool, StressClient } from "./clients.ts";
import type { ServerProcess } from "./server.ts";

export interface Phase {
    name: string;
    startMs: number;
    endMs: number;
}

export interface GeneratorSample {
    timeMs: number;
    phase: string;
    stats: object;
}

/** 用例共享阶段时钟与健康检查；时长按单调时钟计算。 */
export class ScenarioContext {
    readonly options: Options;
    readonly pool: ClientPool;
    readonly server: ServerProcess;
    readonly control: StressClient;
    readonly phases: Phase[] = [];
    readonly generator: GeneratorSample[] = [];
    readonly workload: Record<string, unknown> = {};
    private currentPhase = "setup";
    private nextSampleMs = 0;

    /** 连接用例内资源，不拥有服务端与机器人清理流程。 */
    constructor(options: Options, pool: ClientPool, server: ServerProcess, control: StressClient) {
        this.options = options;
        this.pool = pool;
        this.server = server;
        this.control = control;
    }

    /** 同时检查服务端和机器人，避免空载等待完整个用例。 */
    check(): void {
        if (this.server.state !== "ready") throw new Error(`Server is ${this.server.state}`);
        this.pool.assertHealthy();
        if (performance.now() >= this.nextSampleMs) {
            this.generator.push({ timeMs: Date.now(), phase: this.currentPhase, stats: this.pool.snapshot() });
            this.nextSampleMs = performance.now() + 1000;
        }
    }

    /** 记录阶段边界，即使阶段失败也保留已完成部分。 */
    async measure(name: string, action: () => Promise<void>): Promise<void> {
        const phase: Phase = { name, startMs: Date.now(), endMs: Date.now() };
        this.phases.push(phase);
        this.currentPhase = name;
        console.log(`  ${name}`);
        try {
            await action();
        } finally {
            phase.endMs = Date.now();
            this.generator.push({ timeMs: Date.now(), phase: name, stats: this.pool.snapshot() });
        }
    }

    /** 等待期间持续检查，不依赖可能随低 TPS 停滞的 physicsTick 计数。 */
    async wait(seconds: number): Promise<void> {
        const deadline = performance.now() + seconds * 1000;
        while (performance.now() < deadline) {
            this.check();
            await delay(Math.min(50, Math.max(1, deadline - performance.now())));
        }
        this.check();
    }

    /** 按固定节奏施压；延迟时不补发无限突发负载。 */
    async paced(seconds: number, periodMs: number, action: (elapsedSeconds: number, deltaSeconds: number) => void): Promise<void> {
        const started = performance.now();
        const deadline = started + seconds * 1000;
        let previous = started;
        let next = started + periodMs;
        while (next <= deadline) {
            await delay(Math.max(1, next - performance.now()));
            const now = Math.min(performance.now(), deadline);
            this.check();
            action((now - started) / 1000, (now - previous) / 1000);
            previous = now;
            next = Math.max(next + periodMs, performance.now() + 1);
        }
        if (performance.now() < deadline) await this.wait((deadline - performance.now()) / 1000);
    }
}
