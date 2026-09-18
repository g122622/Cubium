/*
 * surface.ts — 地表基准探测与 Y 轴归一化。
 *
 * **本文件是「Cubium 与 vanilla 地形基准不同」这一必然差异的唯一收敛点。**
 *
 * 背景：Cubium 的超平坦层从 Y=0 起（FlatChunkGenerator::getMinY() 返回 0），地表在 Y=3；
 * vanilla 的超平坦层从维度 minY=-64 起，地表在 Y=-61。两侧绝对 Y 差 64 格。
 * 任何跨服务端的断言若直接比较绝对 Y 必然假失败，故一律先经本模块换算成「相对地表」。
 *
 * 探测而非硬编码：不写死 3 / -61。若将来 Cubium 修好 getMinY 或改用别的层配置，
 * 探测式会自动跟随，硬编码则会假失败。
 */

import type { Bot } from "mineflayer";

// vec3 是 mineflayer 的依赖，随其一起安装。类型定义缺失，故按构造签名断言。
import Vec3Module from "vec3";

type Vec3Ctor = new (x: number, y: number, z: number) => never;
const Vec3 = Vec3Module as unknown as Vec3Ctor;

/** 空气方块的名称集合（mineflayer 的 block.name 不带命名空间前缀）。 */
const AIR_NAMES = new Set(["air", "cave_air", "void_air"]);

/**
 * 构造 Vec3。
 *
 * 必须用 Vec3 实例而非普通 `{x,y,z}` 对象：mineflayer 的 blockAt / dig / placeBlock
 * 内部会调用 `pos.floored()` 等方法，普通对象会抛 "pos.floored is not a function"。
 */
export function vec3(x: number, y: number, z: number): never {
    return new Vec3(x, y, z) as never;
}

/** 判定一个方块名是否算空气。 */
export function isAir(blockName: string | undefined): boolean {
    return blockName === undefined || AIR_NAMES.has(blockName);
}

/**
 * 读某个坐标的方块对象；区块未加载时返回 null。
 *
 * 必须传 Vec3 实例：mineflayer 的 blockAt 内部会调用 `pos.floored()`，
 * 传普通 `{x,y,z}` 对象会抛 "pos.floored is not a function"。
 */
export function blockAt(bot: Bot, x: number, y: number, z: number): unknown {
    return bot.blockAt(new Vec3(x, y, z) as never);
}

/** 读某个坐标的方块名；区块未加载时返回 undefined。 */
export function blockNameAt(bot: Bot, x: number, y: number, z: number): string | undefined {
    const block = blockAt(bot, x, y, z) as { name: string } | null;
    return block === null ? undefined : block.name;
}

/**
 * 读某个坐标方块的生物群系 id；不可用时返回 undefined。
 *
 * 注意：`block.biome` 是 prismarine-block 新建的**占位 Biome 对象**——prismarine-chunk 的
 * `Block.fromStateId(stateId, biomeId)` 只带 id、不注入数据，故其 `name`/`color`/`temperature`
 * 恒为默认值（空串 / 0）。要拿名字必须经 registry 反查（见 biomeNameOfId）。
 * 因此跨端可比的锚点是 **id**，不是 name。
 */
export function biomeIdAt(bot: Bot, x: number, y: number, z: number): number | undefined {
    const block = blockAt(bot, x, y, z) as { biome?: { id?: number } } | null;
    return block?.biome?.id;
}

/** 经客户端 registry 把 biome id 反查为名字（name 不带命名空间前缀）。 */
export function biomeNameOfId(bot: Bot, biomeId: number): string | undefined {
    const registry = bot.registry as unknown as { biomes?: Record<number, { name?: string }> };
    return registry.biomes?.[biomeId]?.name;
}

/** 探测参数。 */
export interface DetectSurfaceOptions {
    /** 扫描的 Y 上界（含）。 */
    readonly scanTop: number;
    /** 扫描的 Y 下界（含）。 */
    readonly scanBottom: number;
    /** 总超时；超时仍未探到地表则抛异常。 */
    readonly timeoutMs: number;
    /** 轮询间隔。 */
    readonly pollMs: number;
}

/**
 * 探测某列的地表 Y（自上而下第一个非空气方块）。
 *
 * 内部轮询而非依赖 bot.waitForChunksToLoad()：后者要等满 viewDistance 覆盖的全部列
 * （viewDistance=4 时 81 列），在测试中过慢且可能长时间挂起；出生点所在列通常先到。
 *
 * @returns 地表方块的 Y 坐标。
 */
export async function detectSurfaceY(
    bot: Bot,
    x: number,
    z: number,
    options: DetectSurfaceOptions,
): Promise<number> {
    const deadline = Date.now() + options.timeoutMs;
    while (true) {
        for (let y = options.scanTop; y >= options.scanBottom; y -= 1) {
            const name = blockNameAt(bot, x, y, z);
            if (name !== undefined && !isAir(name)) {
                return y;
            }
        }
        if (Date.now() >= deadline) {
            throw new Error(
                `探测地表超时（列 ${x},${z}，扫描 ${options.scanTop}..${options.scanBottom}，` +
                    `等待 ${options.timeoutMs}ms）——该列可能尚未收到区块数据`,
            );
        }
        await new Promise((resolve) => setTimeout(resolve, options.pollMs));
    }
}

/**
 * 等待以 (centerX, centerZ) 为中心、半径 radius 的正方形区域内所有列的方块可读。
 *
 * 必要性：区块是服务端逐 tick 推送的，spawn 事件到达时视距内的列**未必已全部送达**。
 * 若用例在此时就读取邻域方块，会得到大量"未加载"的假失败。
 *
 * @param y 用于探针的 Y 坐标（取地表高度即可——非空气方块一定可读）。
 */
export async function waitForAreaLoaded(
    bot: Bot,
    centerX: number,
    centerZ: number,
    y: number,
    radius: number,
    options: { readonly timeoutMs: number; readonly pollMs: number },
): Promise<void> {
    const deadline = Date.now() + options.timeoutMs;
    while (true) {
        let missing = 0;
        for (let dx = -radius; dx <= radius; dx += 1) {
            for (let dz = -radius; dz <= radius; dz += 1) {
                if (blockNameAt(bot, centerX + dx, y, centerZ + dz) === undefined) {
                    missing += 1;
                }
            }
        }
        if (missing === 0) {
            return;
        }
        if (Date.now() >= deadline) {
            throw new Error(
                `等待区域加载超时：以 (${centerX},${centerZ}) 为中心、半径 ${radius} 的区域内仍有 ` +
                    `${missing} 列未加载（等待 ${options.timeoutMs}ms）`,
            );
        }
        await new Promise((resolve) => setTimeout(resolve, options.pollMs));
    }
}

/** 采集某列自地表向下 N 层的方块名（含地表本身）。 */
export function layersBelowSurface(bot: Bot, x: number, z: number, surfaceY: number, count: number): string[] {
    const layers: string[] = [];
    for (let i = 0; i < count; i += 1) {
        layers.push(blockNameAt(bot, x, surfaceY - i, z) ?? "<未加载>");
    }
    return layers;
}

/** 采集地表上方 N 层的方块名。 */
export function layersAboveSurface(bot: Bot, x: number, z: number, surfaceY: number, count: number): string[] {
    const layers: string[] = [];
    for (let i = 1; i <= count; i += 1) {
        layers.push(blockNameAt(bot, x, surfaceY + i, z) ?? "<未加载>");
    }
    return layers;
}
