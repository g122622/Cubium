// 红石电路测试共用工具；所有坐标均为 GameTest 相对坐标。
import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import type { Vector3 } from "@minecraft/server";

export const HORIZONTAL_INPUTS = [
    { name: "north", x: 0, y: 0, z: -1 },
    { name: "south", x: 0, y: 0, z: 1 },
    { name: "east", x: 1, y: 0, z: 0 },
    { name: "west", x: -1, y: 0, z: 0 },
] as const;

export function offset(pos: Vector3, direction: Vector3, distance: number): Vector3 {
    return { x: pos.x + direction.x * distance, y: pos.y + direction.y * distance, z: pos.z + direction.z * distance };
}

export function setCircuitBlock(test: Test, type: string, pos: Vector3, states: string): void {
    (test as Test & { setBlockWithStates(type: string, pos: Vector3, states: string): void })
        .setBlockWithStates(`minecraft:${type}`, pos, states);
}

export function assertState(test: Test, pos: Vector3, property: string, expected: string | number | boolean): void {
    const block = test.getBlock(pos);
    test.assert(block !== undefined, `Missing block at ${JSON.stringify(pos)}`);
    const actual = block!.permutation.getState(property as any);
    test.assert(actual === expected, `${block!.typeId} at ${JSON.stringify(pos)}: ${property} expected ${expected}, got ${actual}`);
}

export function assertType(test: Test, pos: Vector3, expected: string): void {
    const actual = test.getBlock(pos)?.typeId;
    test.assert(actual === `minecraft:${expected}`, `Block at ${JSON.stringify(pos)}: expected ${expected}, got ${actual}`);
}

export function wireLine(test: Test, start: Vector3, direction: Vector3, length: number): void {
    for (let i = 0; i < length; i++) test.setBlockType("minecraft:redstone_wire", offset(start, direction, i));
}

export function registerCircuitTest(name: string, callback: (test: Test) => void, maxTicks: number): void {
    GameTest.register("BlockBehaviorTests", `redstone_deep_${name}`, callback)
        .structureName("gametests:redstone_lab")
        .batch(`redstone_deep_${name.split("_")[0]}`)
        .maxTicks(maxTicks);
}

let switchId = 0;

// 通过真实玩家交互切换拉杆，保证附着方块的邻居通知链也被验证。
export function createLeverSwitch(test: Test, pos: Vector3, states: string): () => void {
    setCircuitBlock(test, "lever", pos, states);
    const player = test.spawnSimulatedPlayer({ x: pos.x + 2, y: pos.y, z: pos.z + 2 }, `redstone_${switchId++}`);
    return () => test.assert(player.interactWithBlock(pos), `Lever interaction failed at ${JSON.stringify(pos)}`);
}
