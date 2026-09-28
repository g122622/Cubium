/*
 * entities.ts — 场景 I：掉落物实体的完整生命周期。
 *
 * 覆盖「丢弃 → 世界里出现实体 → 玩家拾取 → 物品回到物品栏」这条往返。此前 e2e 只用掉落物
 * 作为「错开实体 id 序列」的工具（handshake/login_entity_id_is_entity），从未验证它本身。
 *
 * 两处判据上的讲究：
 *   1. **丢弃**走 `bot.tossStack`：它发 `window_click`（先左键拾起，再点击窗口外的 -999 丢弃），
 *      依赖服务端存在 containerId=0 的常驻背包菜单——这正是容器链路修复的一项，回归价值高。
 *   2. **拾取**不能让客户端本地预测冒充结果。`tossStack` 会把本地槽位清空，因此「物品栏里
 *      又出现了这块石头」只可能来自服务端（拾取同步），是可靠判据。
 *
 * 拾取靠 `/tp` 把玩家送到达掉落物所在位置：靠按键走路到精确落点既慢又不稳（掉落物有初速度），
 * 而拾取半径只有约 1 格。传送是服务端行为，不影响被考察的拾取逻辑。
 */

import type { Bot } from "mineflayer";
import type { CaseDefinition } from "../case.ts";
import { expectTrue } from "../assert/expect.ts";
import { delay, waitForCondition } from "../bot/wait.ts";
import { INV_HOTBAR_SLOT_0, entityIds, newEntityIds, runCommand, setSlot, waitForSlot } from "./shared.ts";

/** 掉落物实体出现/消失的等待上限。 */
const ENTITY_TIMEOUT_MS = 10_000;
/** 拾取等待上限（含掉落物的拾取延迟）。 */
const PICKUP_TIMEOUT_MS = 20_000;
/** 把玩家送到掉落物的尝试次数与间隔。 */
const TELEPORT_ATTEMPTS = 6;
const TELEPORT_INTERVAL_MS = 700;

/** 一次实体快照：id → 实体对象。 */
type EntityTable = Record<number, { id: number; position: { x: number; y: number; z: number } }>;

function entityTable(bot: Bot): EntityTable {
    return bot.entities as unknown as EntityTable;
}

/** 物品栏里是否有指定物品。 */
function inventoryHas(bot: Bot, itemName: string): boolean {
    const slots = (bot.inventory as unknown as { slots: ({ name?: string } | null)[] }).slots;
    return slots.some((item) => item?.name === itemName);
}

export const entityCases: readonly CaseDefinition[] = [
    {
        id: "entities/toss_creates_dropped_item_entity",
        title: "丢弃物品在世界里生成掉落物实体（window_click 路径被受理）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: false,
        skipReason: null,
        async run({ bot }): Promise<Record<string, unknown>> {
            const given = await setSlot(bot, INV_HOTBAR_SLOT_0, "stone", 1);
            expectTrue(given, "无法给 bot 发放石头");
            await waitForSlot(bot, INV_HOTBAR_SLOT_0, "stone", "石头就位");

            const before = Object.keys(entityTable(bot)).map((key) => Number(key));
            const item = (bot.inventory as unknown as { slots: ({ slot?: number } | null)[] }).slots[INV_HOTBAR_SLOT_0];
            expectTrue(item !== null && item !== undefined, "快捷栏首槽应有石头");

            const typed = bot as unknown as { tossStack(item: unknown): Promise<void> };
            await typed.tossStack(item);

            // 服务端为掉落物分配实体 id 后会下发 add_entity（也可能是 spawn_entity，
            // nmp 会归一化为同一个 'entity' 事件）。
            await waitForCondition(() => newEntityIds(bot, before).length > 0, {
                timeoutMs: ENTITY_TIMEOUT_MS,
                pollMs: 50,
                what: "世界里出现新的掉落物实体",
                describe: () => `实体总数仍为 ${Object.keys(entityTable(bot)).length}（丢弃包可能被丢弃）`,
            });

            // 本地槽位被 tossStack 清空；服务端是否同步另说，这里只记录丢弃确实发生了。
            expectTrue(!inventoryHas(bot, "stone"), "丢弃后本地物品栏不应还有石头");
            return { droppedEntityCreated: true, droppedStackRemovedFromInventory: true };
        },
    },

    {
        id: "entities/dropped_item_is_picked_up",
        title: "玩家靠近后掉落物被拾取并回到物品栏（服务端拾取链路）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: true,
        skipReason: null,
        async run({ bot, trace }): Promise<Record<string, unknown>> {
            const given = await setSlot(bot, INV_HOTBAR_SLOT_0, "stone", 1);
            expectTrue(given, "无法给 bot 发放石头");
            await waitForSlot(bot, INV_HOTBAR_SLOT_0, "stone", "石头就位");

            const before = Object.keys(entityTable(bot)).map((key) => Number(key));
            const item = (bot.inventory as unknown as { slots: ({ slot?: number } | null)[] }).slots[INV_HOTBAR_SLOT_0];
            const typed = bot as unknown as { tossStack(item: unknown): Promise<void> };
            await typed.tossStack(item);

            let droppedId = -1;
            await waitForCondition(
                () => {
                    const ids = newEntityIds(bot, before);
                    if (ids.length === 0) {
                        return false;
                    }
                    droppedId = ids[0];
                    return true;
                },
                {
                    timeoutMs: ENTITY_TIMEOUT_MS,
                    pollMs: 50,
                    what: "掉落物实体出现（用于定位它的坐标）",
                },
            );

            // 掉落物有初速度，落点会漂移；按当前坐标反复把自己送到它身上，直到被拾取。
            // 判据只能是服务端行为：tossStack 已把本地槽位清空，物品栏里重新出现石头
            // 只可能来自拾取同步（客户端无法凭空预测出拾取）。
            let pickedUp = false;
            for (let attempt = 0; attempt < TELEPORT_ATTEMPTS && !pickedUp; attempt++) {
                const entity = entityTable(bot)[droppedId];
                if (entity === undefined) {
                    break;
                }
                const { x, y, z } = entity.position;
                await runCommand(bot, `/tp @s ${x} ${y} ${z}`, 3_000);
                const deadline = Date.now() + TELEPORT_INTERVAL_MS;
                while (Date.now() < deadline) {
                    await delay(100);
                    if (inventoryHas(bot, "stone") && entityTable(bot)[droppedId] === undefined) {
                        pickedUp = true;
                        break;
                    }
                }
            }

            expectTrue(
                entityTable(bot)[droppedId] === undefined,
                "掉落物实体仍在世界里——玩家靠近后未被拾取（服务端拾取未生效）",
            );
            expectTrue(inventoryHas(bot, "stone"), "拾取后物品栏里没有石头——拾取未把物品交还玩家");
            return {
                droppedEntityPickedUp: true,
                nearbyEntityPickupSynchronized: true,
                // 拾取动画包的名字两侧可能不同（Prismarine 别名陷阱），只记录"见到过"，
                // 不记录次数——次数会随帧同步次数抖动，入基线会造成假失败。
                sawPickupAnimationPacket: trace.count("take_item_entity") + trace.count("collect") > 0,
            };
        },
    },
];
