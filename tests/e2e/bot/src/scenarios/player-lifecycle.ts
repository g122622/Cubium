/*
 * player-lifecycle.ts — 场景 P：玩家实体在他人世界的生命周期。
 *
 * 覆盖「玩家 B 断开 → 服务端把 B 的实体从 A 的世界里移除 → A 侧不再看到 B」。既有
 * handshake.ts 只验证了 B 的 **Tab 列表条目**被移除（`player_info_update` 的离场广播），
 * 从未验证 B 的**玩家实体**是否也从 A 的世界里消失——两者是彼此独立的通路，Tab 列表干净
 * 不代表世界里没有一具「幽灵玩家」。
 *
 * 判据取旁观者 A：B 退出后 A 必须收到 `entity_destroy`(cb 75) 且 B 的实体不再出现在
 * A 的实体表中。A 侧实体表完全由服务端驱动，客户端无法凭空保留或删除。
 *
 * 实测（Cubium）：断连路径经 `world.destroyEntity` → `EntityTracker` 向追踪者广播
 * `RemoveEntities`，旁观者世界里 B 的实体被正确移除。该用例因此是**回归保护**，与
 * `handshake/tab_list_removes_on_leave`（Tab 条目移除，走 `player_info_update` 另一条通路）
 * 互为补充——两条通路互不相干，任一条断掉都不会被另一条发现。
 */

import type { Bot } from "mineflayer";
import type { CaseDefinition } from "../case.ts";
import { expectTrue } from "../assert/expect.ts";
import { waitForCondition } from "../bot/wait.ts";

/** 等待实体出现/消失的上限。 */
const LIFECYCLE_TIMEOUT_MS = 15_000;

/** 在观看者 bot 的实体表里找「另一个玩家」实体。 */
function findOtherPlayerEntity(bot: Bot): { id: number } | undefined {
    const selfId = bot.entity?.id ?? -1;
    const entities = bot.entities as unknown as Record<number, { type?: string }>;
    for (const [key, entity] of Object.entries(entities)) {
        if (Number(key) !== selfId && entity.type === "player") {
            return { id: Number(key) };
        }
    }
    return undefined;
}

export const playerLifecycleCases: readonly CaseDefinition[] = [
    {
        id: "player-lifecycle/leave_removes_entity",
        title: "玩家断开后其实体从他人世界移除（entity_destroy 广播）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: false,
        // 实测（Cubium）：离场玩家的实体确实从旁观者世界里移除，本用例为**回归保护**
        // （防止将来改动断连/实体追踪链路时丢掉该广播）。
        skipReason: null,
        async run({ bot, connectBot }): Promise<Record<string, unknown>> {
            const second = await connectBot();

            // 等旁观者世界里出现第二个玩家的实体。
            await waitForCondition(() => findOtherPlayerEntity(bot) !== undefined, {
                timeoutMs: LIFECYCLE_TIMEOUT_MS,
                pollMs: 100,
                what: "旁观者世界里出现第二个玩家的实体",
            });

            // 让第二个玩家断开。
            second.quit();

            // 旁观者应经 entity_destroy(cb 75) 把该实体从世界里移除。
            await waitForCondition(() => findOtherPlayerEntity(bot) === undefined, {
                timeoutMs: LIFECYCLE_TIMEOUT_MS,
                pollMs: 100,
                what: "第二个玩家的实体从旁观者世界移除",
                describe: () =>
                    `旁观者世界里仍存在离场玩家的实体（id=${String(findOtherPlayerEntity(bot)?.id)}）` +
                    "——服务端未广播 entity_destroy",
            });

            return { departedPlayerEntityRemoved: true };
        },
    },
];
