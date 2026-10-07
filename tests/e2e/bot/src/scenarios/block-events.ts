/*
 * block-events.ts — 场景 O：方块事件（BlockEvent）的广播。
 *
 * 覆盖「玩家开箱 → 服务端广播 BlockEvent(cb 7) → 客户端播放箱盖开合动画」这条链路。
 * 方块事件是「服务端主动通知客户端做一段本地动画」的通用机制（箱子/潜影盒开盖、末影箱、
 * 音符盒、装饰陶罐摇晃、刷怪笼等），它不改变世界状态，因此**不发也不会报错**——
 * 客户端只是「箱子开了，盖子纹丝不动」，是最容易长期潜伏的一类表现层缺陷。
 *
 * 判据取**旁观者**：开箱者本地会自己播放动画（客户端预测），只有旁观者收到的 BlockEvent
 * 才是服务端广播的证据。判据直接数旁观者的原始包（`block_action`），不经 mineflayer 的
 * 高层事件解析——后者只在方块名可解析时才 emit，会引入与「包发没发」无关的假失败。
 *
 * 实测（Cubium）：开箱与关箱各广播一次 `block_action`（byte1=1 开 / byte2 为打开者计数），
 * 旁观者均能收到。该用例因此是**回归保护**（防止容器打开/关闭链路将来丢掉 startOpen/stopOpen
 * 的广播），而非缺陷暴露。
 */

import type { Bot } from "mineflayer";
import type { CaseDefinition } from "../case.ts";
import { expectEq, expectTrue } from "../assert/expect.ts";
import { blockNameAt, vec3 } from "../assert/surface.ts";
import { delay, waitForCondition } from "../bot/wait.ts";
import { INV_HOTBAR_SLOT_0, setSlot } from "./shared.ts";

/** 箱子菜单的等待上限。 */
const OPEN_TIMEOUT_MS = 15_000;
/** 旁观者收到方块事件的等待上限。 */
const BLOCK_EVENT_TIMEOUT_MS = 10_000;

/**
 * 打开方块容器并等待窗口（同 containers.ts 的超时封装，避免 mineflayer 的 20 秒无信息超时）。
 */
async function openContainerTimed(bot: Bot, block: unknown): Promise<unknown> {
    const typed = bot as unknown as { openContainer(block: unknown): Promise<unknown> };
    let timer: NodeJS.Timeout | null = null;
    try {
        return await Promise.race([
            typed.openContainer(block),
            new Promise<never>((_, reject) => {
                timer = setTimeout(
                    () => reject(new Error(`打开容器超时（${OPEN_TIMEOUT_MS}ms）：服务端未下发窗口内容`)),
                    OPEN_TIMEOUT_MS,
                );
            }),
        ]);
    } finally {
        if (timer !== null) {
            clearTimeout(timer);
        }
    }
}

export const blockEventCases: readonly CaseDefinition[] = [
    {
        id: "block-events/chest_open_broadcasts_block_action",
        title: "开箱广播 BlockEvent（旁观者收到箱盖开合动画包）",
        servers: ["cubium", "vanilla"],
        botCount: 2,
        opPlayers: false,
        skipReason: null,
        async run({ bot, traces, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 旁观者的原始包记录（traces 与 bots 同序，[1] 即第二个 bot）。
            const observerTrace = traces[1];
            const before = observerTrace.count("block_action");

            // 开箱者在出生点旁放一个箱子。
            const x = spawnX + 2;
            const z = spawnZ + 2;
            const given = await setSlot(bot, INV_HOTBAR_SLOT_0, "chest", 1);
            expectTrue(given, "无法给 bot 发放箱子");
            await delay(200);
            const refBlock = bot.blockAt(vec3(x, surfaceY, z));
            expectTrue(refBlock !== null, `参考方块 (${x},${surfaceY},${z}) 不可读`);
            await bot.placeBlock(refBlock as never, vec3(0, 1, 0));
            await delay(200);
            expectEq(blockNameAt(bot, x, surfaceY + 1, z), "chest", "放置后应为箱子");

            // 打开箱子。服务端应在此刻把 openCount 置 1 并广播 BlockEvent(1, 1)。
            const chestBlock = bot.blockAt(vec3(x, surfaceY + 1, z));
            expectTrue(chestBlock !== null, "放置后的箱子不可读");
            const window = await openContainerTimed(bot, chestBlock as never);

            await waitForCondition(() => observerTrace.count("block_action") > before, {
                timeoutMs: BLOCK_EVENT_TIMEOUT_MS,
                pollMs: 100,
                what: "旁观者收到开箱的 BlockEvent（block_action cb 7）",
                describe: () =>
                    `旁观者收到的 block_action 计数未增加（仍为 ${before}）——服务端未广播箱盖动画`,
            });

            await bot.closeWindow(window as never);
            return {
                blockEventBroadcastToObserver: true,
                // 只记录「见过」而非次数：开箱/关箱各广播一次，但具体次数可能随实现抖动，
                // 入基线会造成假失败。
                sawBlockActionForChest: observerTrace.count("block_action") > before,
            };
        },
    },
];
