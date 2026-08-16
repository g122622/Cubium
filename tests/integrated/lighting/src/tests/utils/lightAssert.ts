// 光照集成测试断言辅助（lighting 包专有，依赖 Cubium 扩展的 Block 光照只读属性）。
//
// 背景：Cubium 在官方 @minecraft/server Block 之上扩展了 blockLight/skyLight/brightness/canSeeSky
// 四个只读属性（Cubium 专有，官方基岩 BDS 无）。本模块封装光照数值断言，供各测试文件复用。
//
// 关键约束：方块变更后光照重算是异步的。setBlock 后光照变更入队 m_lightQueue，需若干世界 tick
// 由 ServerWorld::tick 批量重算（BlockLightEngine/SkyLightEngine 传播）。故断言前必须等待光照
// 稳定——用 runAtTickTime 延迟若干 tick，或用 pollUntilSucceed 轮询直到光照达预期。

import type { Test } from "@minecraft/server-gametest";

/**
 * 读取结构相对坐标处方块的方块光等级（0-15）。
 * 经 test.getBlock 拿 Block JS 对象（Cubium 扩展属性 blockLight）。
 */
export function getBlockLight(test: Test, x: number, y: number, z: number): number {
    const block = test.getBlock({ x, y, z });
    // blockLight 是 Cubium 专有扩展属性；基岩 BDS 的 Block 无此属性返回 undefined。
    // 此处用 as unknown as number 兼容类型增强声明（cubium-gametest-augment.d.ts）。
    return (block as unknown as { blockLight?: number }).blockLight ?? -1;
}

/**
 * 读取结构相对坐标处方块的天空光等级（0-15）。
 */
export function getSkyLight(test: Test, x: number, y: number, z: number): number {
    const block = test.getBlock({ x, y, z });
    return (block as unknown as { skyLight?: number }).skyLight ?? -1;
}

/**
 * 读取结构相对坐标处方块的综合亮度（0-15，含天空减暗）。
 */
export function getBrightness(test: Test, x: number, y: number, z: number): number {
    const block = test.getBlock({ x, y, z });
    return (block as unknown as { brightness?: number }).brightness ?? -1;
}

/**
 * 读取结构相对坐标处方块是否露天。
 */
export function getCanSeeSky(test: Test, x: number, y: number, z: number): boolean {
    const block = test.getBlock({ x, y, z });
    return (block as unknown as { canSeeSky?: boolean }).canSeeSky ?? false;
}

/**
 * 断言方块光等级等于期望值。失败抛带坐标与实际值的错误信息。
 */
export function assertBlockLight(test: Test, x: number, y: number, z: number, expected: number, context = ""): void {
    const actual = getBlockLight(test, x, y, z);
    test.assert(
        actual === expected,
        `expected blockLight=${expected} at (${x},${y},${z}) but got ${actual}${context ? " (" + context + ")" : ""}`,
    );
}

/**
 * 断言天空光等级等于期望值。
 */
export function assertSkyLight(test: Test, x: number, y: number, z: number, expected: number, context = ""): void {
    const actual = getSkyLight(test, x, y, z);
    test.assert(
        actual === expected,
        `expected skyLight=${expected} at (${x},${y},${z}) but got ${actual}${context ? " (" + context + ")" : ""}`,
    );
}

/**
 * 断言是否露天。
 */
export function assertCanSeeSky(test: Test, x: number, y: number, z: number, expected: boolean, context = ""): void {
    const actual = getCanSeeSky(test, x, y, z);
    test.assert(
        actual === expected,
        `expected canSeeSky=${expected} at (${x},${y},${z}) but got ${actual}${context ? " (" + context + ")" : ""}`,
    );
}
