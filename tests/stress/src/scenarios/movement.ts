import assert from "node:assert/strict";
import type { ScenarioContext } from "../context.ts";

/** 每档单独计时、单独运行；停止后用服务端权威位置验证实际位移。 */
export async function runMovement(context: ScenarioContext, speed: number): Promise<void> {
    const { control, options } = context;
    const start = control.bot.entity.position.clone();
    const initialChunks = control.chunksReceived;
    const initialCorrections = control.corrections;
    let sentDistance = 0;
    await context.measure("moving", async () => {
        await context.paced(options.moveSeconds, 50, (_elapsed, delta) => {
            // 发生器停顿时限制单包步长，缺失负载由位移达成检查明确报告。
            sentDistance += speed * Math.min(delta, 0.1);
            control.bot.entity.position.set(start.x + sentDistance, start.y, start.z);
            control.write("position", {
                x: start.x + sentDistance, y: start.y, z: start.z,
                flags: { onGround: false, hasHorizontalCollision: false },
            });
        });
    });
    // 原地观察完整时长，保留连接、视距和区块订阅。
    await context.measure("recovery", () => context.wait(options.recoverySeconds));
    const correctionsDuringLoad = control.corrections - initialCorrections;
    await context.measure("verification", async () => {
        await control.barrier(options.spawnTimeoutSeconds * 1000);
        // 目标玩家也是自己，目的地取自服务端玩家数据，返回实际接受的位置。
        await control.teleport(`/tp ${control.username} ${control.username}`, options.spawnTimeoutSeconds * 1000);
    });
    const end = control.bot.entity.position;
    const distance = end.x - start.x;
    const expected = speed * options.moveSeconds;
    const chunksReceived = control.chunksReceived - initialChunks;
    Object.assign(context.workload, {
        speed, expectedDistance: expected, sentDistance, serverConfirmedDistance: distance,
        correctionsDuringLoad, chunksReceived, chunksUnloaded: control.chunksUnloaded,
        start: { x: start.x, y: start.y, z: start.z }, end: { x: end.x, y: end.y, z: end.z },
    });
    assert(sentDistance >= expected * 0.9, `Load generator reached only ${sentDistance.toFixed(1)} / ${expected.toFixed(1)} blocks`);
    assert(distance >= expected * 0.9, `Server accepted only ${distance.toFixed(1)} / ${expected.toFixed(1)} blocks`);
    assert(Math.abs(end.z - start.z) < 1 && Math.abs(end.y - start.y) < 1, "Movement left the prescribed straight line");
    assert(chunksReceived > 0, "No new chunks were received during movement");
}
