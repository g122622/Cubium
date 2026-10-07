/*
 * player-visuals.ts — 场景 N：玩家视觉状态同步（装备/手持物）。
 *
 * 覆盖「玩家 A 改变手持物 → 服务端广播 → 玩家 B 看到 A 手里拿着该物品」这条链路。
 * 这是多人游戏最基本的可见性之一，此前完全没有自动化覆盖，且它是典型的「服务端自己有、
 * 别人看不见」：A 本地手持物正确（客户端预测 + 创造改槽），但 B 侧看不到任何变化。
 *
 * 判据取**旁观者 B**：B 世界里 A 的实体（prismarine-entity）的 `heldItem` 来自
 * `entity_equipment`(cb 100) 包，只可能由服务端广播。A 自己改槽不会让 B 侧镜像变化。
 */

import type { Bot } from "mineflayer";
import type { CaseDefinition } from "../case.ts";
import { expectTrue } from "../assert/expect.ts";
import { waitForCondition } from "../bot/wait.ts";
import { INV_HOTBAR_SLOT_0, setSlot } from "./shared.ts";

/** 等待 B 看到 A 的手持物更新的上限。 */
const VISUAL_SYNC_TIMEOUT_MS = 10_000;

/** 在旁观者 bot 的实体表里找「另一个玩家」实体。 */
function findOtherPlayerEntity(bot: Bot): { heldItem?: { name?: string } } | undefined {
    const selfId = bot.entity?.id ?? -1;
    const entities = bot.entities as unknown as Record<number, { type?: string; heldItem?: { name?: string } }>;
    for (const [key, entity] of Object.entries(entities)) {
        if (Number(key) !== selfId && entity.type === "player") {
            return entity;
        }
    }
    return undefined;
}

export const playerVisualCases: readonly CaseDefinition[] = [
    {
        id: "player-visuals/held_item_syncs_to_others",
        title: "玩家改变手持物后旁观者看到该物品（entity_equipment 广播）",
        servers: ["cubium", "vanilla"],
        botCount: 2,
        opPlayers: false,
        skipReason: null,
        async run({ bots }): Promise<Record<string, unknown>> {
            const [holder, observer] = bots;

            // 等旁观者世界里出现持有者的玩家实体。
            await waitForCondition(() => findOtherPlayerEntity(observer) !== undefined, {
                timeoutMs: VISUAL_SYNC_TIMEOUT_MS,
                pollMs: 100,
                what: "旁观者世界里出现持有者的玩家实体",
            });

            // 把持有者当前选中的快捷栏槽位改成钻石剑——这正是「手持物」。
            const selectedSlot = (holder as unknown as { quickBarSlot: number }).quickBarSlot + INV_HOTBAR_SLOT_0;
            const written = await setSlot(holder, selectedSlot, "diamond_sword", 1);
            expectTrue(written, "无法给持有者写入手持物（registry 中找不到 diamond_sword）");

            // 旁观者应经 entity_equipment 看到持有者的手持物变为钻石剑。
            await waitForCondition(
                () => findOtherPlayerEntity(observer)?.heldItem?.name === "diamond_sword",
                {
                    timeoutMs: VISUAL_SYNC_TIMEOUT_MS,
                    pollMs: 100,
                    what: "旁观者看到持有者的手持物为钻石剑",
                    describe: () =>
                        `旁观者看到的持有者手持物：` +
                        `${String(findOtherPlayerEntity(observer)?.heldItem?.name)}` +
                        "（为空即服务端未广播 entity_equipment）",
                },
            );

            return { heldItemVisibleToObserver: true };
        },
    },
];
