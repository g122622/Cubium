import path from "node:path";
import { CUBIUM_SERVER_EXE, REPO_ROOT } from "../../e2e/bot/src/config.ts";

export type ScenarioId = "movement" | "packets" | "joins";

/** 所有缺省选择只在 CLI 边界确定，之后逐层显式传递。 */
export interface Options {
    cases: ScenarioId[];
    server: string;
    output: string;
    world: "default" | "flat";
    seed: string;
    viewDistance: number;
    simulationDistance: number;
    warmupSeconds: number;
    baselineSeconds: number;
    moveSeconds: number;
    recoverySeconds: number;
    speeds: number[];
    packetSeconds: number;
    packetsPerSecond: number;
    players: number;
    joinRate: number;
    holdSeconds: number;
    spawnTimeoutSeconds: number;
    strictRecovery: boolean;
    skipMovementSpeedCheck: boolean;
    memoryRatio: number;
    tickLimitMs: number;
}

/** 解析选项并拒绝无效负载，避免空跑被报告为成功。 */
export function parseOptions(args: readonly string[]): Options | null {
    const options: Options = {
        cases: ["movement", "packets", "joins"],
        server: CUBIUM_SERVER_EXE,
        output: path.join(REPO_ROOT, "build", "stress"),
        world: "default",
        seed: "0",
        viewDistance: 8,
        simulationDistance: 8,
        warmupSeconds: 10,
        baselineSeconds: 10,
        moveSeconds: 60,
        recoverySeconds: 15,
        speeds: [5.6, 16, 32, 64, 128, 256],
        packetSeconds: 60,
        packetsPerSecond: 1000,
        players: 100,
        joinRate: 10,
        holdSeconds: 60,
        spawnTimeoutSeconds: 120,
        strictRecovery: false,
        skipMovementSpeedCheck: false,
        memoryRatio: 1.2,
        tickLimitMs: 50,
    };
    const numericOptions = new Map<string, keyof Options>([
        ["view-distance", "viewDistance"], ["simulation-distance", "simulationDistance"],
        ["warmup-seconds", "warmupSeconds"], ["baseline-seconds", "baselineSeconds"],
        ["move-seconds", "moveSeconds"], ["recovery-seconds", "recoverySeconds"],
        ["packet-seconds", "packetSeconds"],
        ["packets-per-second", "packetsPerSecond"], ["players", "players"],
        ["join-rate", "joinRate"], ["hold-seconds", "holdSeconds"],
        ["spawn-timeout-seconds", "spawnTimeoutSeconds"],
        ["memory-ratio", "memoryRatio"], ["tick-limit-ms", "tickLimitMs"],
    ]);
    for (const arg of args) {
        if (arg === "--help" || arg === "-h") return null;
        if (arg === "--strict-recovery") {
            options.strictRecovery = true;
            continue;
        }
        if (arg === "--skip-movement-speed-check") {
            options.skipMovementSpeedCheck = true;
            continue;
        }
        const match = /^--([^=]+)=(.+)$/.exec(arg);
        if (match === null) throw new Error(`Invalid option: ${arg}; use --help`);
        const [, name, value] = match;
        const key = numericOptions.get(name);
        if (key !== undefined) {
            const number = Number(value);
            const allowsZero = name === "join-rate" || name === "warmup-seconds";
            if (!Number.isFinite(number) || number < 0 || (!allowsZero && number === 0)) {
                throw new Error(`Invalid value for --${name}: ${value}`);
            }
            Object.assign(options, { [key]: number });
        } else if (name === "case") {
            const selected = value === "all" ? ["movement", "packets", "joins"] : value.split(",");
            if (!selected.every((id) => ["movement", "packets", "joins"].includes(id))) {
                throw new Error(`Unknown scenario: ${value}`);
            }
            options.cases = [...new Set(selected)] as ScenarioId[];
        } else if (name === "speeds") {
            const speeds = value.split(",").map(Number);
            if (speeds.some((speed) => !Number.isFinite(speed) || speed <= 0)) {
                throw new Error(`Invalid speeds: ${value}`);
            }
            options.speeds = [...new Set(speeds)];
        } else if (name === "server" || name === "output") {
            options[name] = path.resolve(value);
        } else if (name === "seed") {
            options.seed = value;
        } else if (name === "world" && (value === "default" || value === "flat")) {
            options.world = value;
        } else {
            throw new Error(`Unknown option: ${arg}; use --help`);
        }
    }
    for (const key of ["players", "packetsPerSecond", "viewDistance", "simulationDistance"] as const) {
        if (!Number.isSafeInteger(options[key])) throw new Error(`${key} must be an integer`);
    }
    if (options.viewDistance < 2 || options.viewDistance > 32 || options.simulationDistance < 2 || options.simulationDistance > 32) {
        throw new Error("View and simulation distances must be in [2, 32]");
    }
    if (options.players > 9999) throw new Error("Player count must be <= 9999");
    return options;
}

/** 打印可独立使用的命令行说明。 */
export function printHelp(): void {
    console.log(`Usage: node run.ts [--name=value]
  --case=all|movement|packets|joins  Comma-separated scenarios (default all)
  --server=<path>                  Server executable
  --output=<path>                  Artifacts root (default build/stress)
  --world=default|flat --seed=0     Fresh isolated world for every scenario
  --view-distance=8 --simulation-distance=8
  --warmup-seconds=10 --baseline-seconds=10
  --move-seconds=60                Creative flight along +X
  --speeds=5.6,16,32,64,128,256     Blocks/second; each speed runs serially in a fresh world
  --skip-movement-speed-check      Explicitly bypass server speed validation for movement jobs
  --recovery-seconds=15            Idle after movement / packet flood / logout
  --packets-per-second=1000 --packet-seconds=60
  --players=100 --join-rate=10      Players/second; 0 starts all connections at once
  --hold-seconds=60                Hold all joined players online
  --spawn-timeout-seconds=120      Timeout for each full spawn
  --strict-recovery               Fail when recovery exceeds the limits below
  --memory-ratio=1.2 --tick-limit-ms=50
Exit codes: 0 passed; 1 scenario failed; 2 invalid arguments or environment error.`);
}
