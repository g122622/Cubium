/*
 * chat.ts — 场景 M：聊天广播。
 *
 * 覆盖「玩家 A 发聊天 → 服务端广播 → 玩家 B 的聊天框收到」这条链路。既有 protocol.ts 只验证
 * 「服务端不因聊天而断连」，从未验证聊天**是否真的被广播**——而「静默丢弃」与「正常广播」
 * 在单 bot 视角下无法区分（发出去没报错，看起来就成功了）。
 *
 * 判据必须落在**另一个玩家**身上：B 收到的聊天内容只能是服务端转发的（B 自己没发），
 * 因此 B 侧观察到的聊天包是服务端确实广播了的直接证据。
 *
 * 聊天包名在 1.21.11 有多个候选（`player_chat` / `system_chat` / `profileless_chat` /
 * 旧名 `chat`），服务端用哪一种实现不影响判据——只要 B 收到了其中之一即算广播成功。
 */

import type { Bot } from "mineflayer";
import type { CaseDefinition } from "../case.ts";
import { expectTrue } from "../assert/expect.ts";
import { waitForCondition } from "../bot/wait.ts";

/** 等待 B 收到广播的上限。 */
const BROADCAST_TIMEOUT_MS = 10_000;

/** 让 bot 记录收到的聊天纯文本（服务端可能经 messagestr 送达）。 */
function recordChatText(bot: Bot): { text: string[] } {
    const sink: { text: string[] } = { text: [] };
    const typed = bot as unknown as { on(event: string, listener: (...args: unknown[]) => void): void };
    typed.on("messagestr", (...args: unknown[]) => {
        const first = args[0];
        if (typeof first === "string") {
            sink.text.push(first);
        }
    });
    return sink;
}

export const chatCases: readonly CaseDefinition[] = [
    {
        id: "chat/message_broadcast_to_other_player",
        title: "一个玩家的聊天被广播给另一个玩家（服务端转发而非静默丢弃）",
        servers: ["cubium", "vanilla"],
        // 预连一个，再追加连接第二个：必须保证「先加入者的监听已就绪」之后才有聊天发生。
        botCount: 1,
        opPlayers: false,
        skipReason: null,
        async run({ bot, connectBot }): Promise<Record<string, unknown>> {
            const second = await connectBot();
            // 在第二个 bot 上装监听器，收集它收到的所有聊天文本。
            const received = recordChatText(second);

            // 等第二个 bot 的实体在第一个 bot 的世界里建立——说明它已完整进入同一世界。
            await waitForCondition(
                () => Object.keys(bot.entities).some((key) => {
                    const entity = (bot.entities as unknown as Record<string, { type?: string }>)[key];
                    return entity?.type === "player" && Number(key) !== bot.entity?.id;
                }),
                {
                    timeoutMs: BROADCAST_TIMEOUT_MS,
                    pollMs: 100,
                    what: "第二个 bot 的玩家实体出现在第一个 bot 的世界中",
                },
            );

            const probe = "e2e-chat-broadcast-probe";
            bot.chat(probe);

            await waitForCondition(() => received.text.some((text) => text.includes(probe)), {
                timeoutMs: BROADCAST_TIMEOUT_MS,
                pollMs: 100,
                what: "第二个玩家收到第一个玩家发出的聊天内容",
                describe: () =>
                    `第二个 bot 已收到的聊天文本：${JSON.stringify(received.text)}` +
                    "（不含探针内容即服务端未广播）",
            });

            return { chatBroadcastObserved: true };
        },
    },
];
