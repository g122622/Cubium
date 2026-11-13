/*
 * bot-view.ts — bot 从自己收到的包看到的世界状态。
 *
 * 服务端自省探针（`/execute if block`、`/data get`）证明的是「服务端认为世界是什么」，
 * 客户端视角证明的是「这些改动有没有同步给玩家」。两者都失败/一个成功一个失败，
 * 指向的是完全不同的缺陷，所以这里只做**独立观测**，不复用任何服务端查询指令。
 *
 * 观测值必须只含稳定字段：方块名与状态属性、实体类型、物品名与数量、天气标记。
 * 坐标、实体 id、UUID、槽位号一律不进结果——它们两侧天然不同，会淹没真正的同步差异。
 */

import type { Bot } from "mineflayer";

import type { BoardOrigin, ClientViewSpec } from "../case.ts";
import { substituteCoordinates } from "../case.ts";
import { delay } from "../../../shared/bot/wait.ts";

/** 观测结果。 */
export interface ClientViewOutcome {
    readonly key: string;
    readonly kind: string;
    readonly text: string;
}

/** entity 观测在未显式给半径时使用的半径（格）。 */
const DEFAULT_ENTITY_RADIUS = 8;

/** 采集前等待网络包落地的时长。 */
const SYNC_SETTLE_MS = 150;

/**
 * 按顺序采集客户端视角。
 *
 * @param bot 已就绪的 bot。
 * @param specs 观测定义。
 * @param origin 本用例的棋盘原点。
 * @returns 观测结果，顺序与输入一致。
 */
export async function captureClientViews(
    bot: Bot,
    specs: readonly ClientViewSpec[],
    origin: BoardOrigin,
): Promise<readonly ClientViewOutcome[]> {
    if (specs.length === 0) {
        return [];
    }
    // 让最后一条指令产生的方块/物品/实体包先到达客户端，再读快照。
    await delay(SYNC_SETTLE_MS);

    const outcomes: ClientViewOutcome[] = [];
    for (const spec of specs) {
        outcomes.push({ key: spec.key, kind: spec.kind, text: readView(bot, spec, origin) });
    }
    return outcomes;
}

/** 读取单条观测。 */
function readView(bot: Bot, spec: ClientViewSpec, origin: BoardOrigin): string {
    switch (spec.kind) {
    case "block":
        return readBlock(bot, spec, origin);
    case "entity":
        return readEntities(bot, spec, origin);
    case "inventory":
        return readInventory(bot);
    case "time":
        return readTime(bot);
    }
}

/** 方块名 + 排好序的状态属性。 */
function readBlock(bot: Bot, spec: ClientViewSpec, origin: BoardOrigin): string {
    const [x, y, z] = parsePosition(spec, origin);
    const block = bot.blockAt(offsetFromSelf(bot, x, y, z));
    if (block === null) {
        return "<区块未加载>";
    }
    const properties = Object.entries(block.getProperties() ?? {})
        .map(([key, value]) => `${key}=${String(value)}`)
        .sort()
        .join(",");
    return properties.length === 0 ? block.name : `${block.name}[${properties}]`;
}

/** 半径内的实体类型计数（排除 bot 自己）。 */
function readEntities(bot: Bot, spec: ClientViewSpec, origin: BoardOrigin): string {
    const [x, y, z] = parsePosition(spec, origin);
    const radius = spec.radius === undefined ? DEFAULT_ENTITY_RADIUS : spec.radius;
    const counts = new Map<string, number>();
    for (const entity of Object.values(bot.entities)) {
        if (entity.id === bot.entity.id) {
            continue;
        }
        const dx = entity.position.x - x;
        const dy = entity.position.y - y;
        const dz = entity.position.z - z;
        if (dx * dx + dy * dy + dz * dz > radius * radius) {
            continue;
        }
        counts.set(entity.name, (counts.get(entity.name) ?? 0) + 1);
    }
    if (counts.size === 0) {
        return "<无实体>";
    }
    return [...counts.entries()]
        .sort((left, right) => (left[0] < right[0] ? -1 : left[0] > right[0] ? 1 : 0))
        .map(([name, count]) => `${name} x${count}`)
        .join(", ");
}

/** 物品栏内容（名字 + 数量，不含槽位）。 */
function readInventory(bot: Bot): string {
    const items = bot.inventory
        .items()
        .map((item) => `${item.name} x${item.count}`)
        .sort();
    return items.length === 0 ? "<空>" : items.join(", ");
}

/** 天气标记与天数（不含会随 tick 漂移的 timeOfDay）。 */
function readTime(bot: Bot): string {
    const raining = bot.isRaining ? "raining" : "clear";
    const thunder = bot.thunderState > 0 ? "thunder" : "no_thunder";
    return `${raining},${thunder},day=${bot.time.day}`;
}

/** 解析观测位置。 */
function parsePosition(spec: ClientViewSpec, origin: BoardOrigin): [number, number, number] {
    if (spec.position === undefined) {
        throw new Error(`clientView.${spec.key}: ${spec.kind} 观测缺少 position`);
    }
    const parts = substituteCoordinates(spec.position, origin)
        .trim()
        .split(/\s+/)
        .map((part) => Number.parseInt(part, 10));
    if (parts.length !== 3 || parts.some((value) => !Number.isFinite(value))) {
        throw new Error(`clientView.${spec.key}: position 必须是三个整数，实际为 "${spec.position}"`);
    }
    return [parts[0]!, parts[1]!, parts[2]!];
}

/**
 * 构造目标坐标的 Vec3。
 *
 * 不直接 import vec3：它只是 mineflayer 的传递依赖，借 bot 自己的 position 实例
 * 构造可以避免在 harness 里额外声明一个依赖。
 */
function offsetFromSelf(bot: Bot, x: number, y: number, z: number) {
    const base = bot.entity.position;
    return base.offset(x - base.x, y - base.y, z - base.z);
}
