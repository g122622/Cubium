import assert from "node:assert/strict";
import type { ScenarioContext } from "../context.ts";

/** 有效旋转、挥手、着地状态包均匀混发，覆盖解码、派发和玩家状态更新。 */
export async function runPackets(context: ScenarioContext): Promise<void> {
    const { control, options } = context;
    let sent = 0;
    let credit = 0;
    // 快捷栏回包会触发 mineflayer 自动回写，高频异步切槽会形成回声风暴，破坏可控速率。
    const names = ["look", "arm_animation", "flying"];
    const counts: Record<string, number> = Object.fromEntries(names.map((name) => [name, 0]));
    try {
        await context.measure("flood", async () => {
            await context.paced(options.packetSeconds, 25, (_elapsed, delta) => {
                // 最多累积 100ms 配额，避免调度迟滞后形成无限补发风暴。
                credit += options.packetsPerSecond * Math.min(delta, 0.1);
                const batch = Math.floor(credit);
                credit -= batch;
                for (let index = 0; index < batch; index += 1) {
                    const name = names[sent % names.length];
                    const params = name === "look"
                        ? { yaw: sent % 360, pitch: 0, flags: { onGround: false, hasHorizontalCollision: false } }
                        : name === "arm_animation" ? { hand: 0 }
                        : { flags: { onGround: false, hasHorizontalCollision: false } };
                    control.write(name, params);
                    counts[name] += 1;
                    sent += 1;
                }
            });
        });
        await context.measure("recovery", () => context.wait(options.recoverySeconds));
        await context.measure("verification", async () => {
            context.workload.drainPongMs = await control.barrier(options.spawnTimeoutSeconds * 1000);
        });
    } finally {
        Object.assign(context.workload, {
            requestedPacketsPerSecond: options.packetsPerSecond,
            sentPackets: sent, counts, achievedPacketsPerSecond: sent / options.packetSeconds,
        });
    }
    const expected = options.packetsPerSecond * options.packetSeconds;
    assert(sent >= expected * 0.9, `Load generator sent only ${sent} / ${expected} packets`);
}
