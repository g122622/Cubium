/*
 * protocol.ts — 场景 K：协议层的存活与容错。
 *
 * 既有用例只覆盖「服务端应当处理的包」；这里覆盖两类"不处理也不能出事"的包：
 *
 *   1. **keep_alive 往返**：心跳是连接存活的基础协议。项目里没有任何单元测试能覆盖它
 *      （unit 走 LocalTransport 不走 socket），而漏答心跳会让真客户端在 30 秒后被踢掉——
 *      表现为"玩一会儿就掉线"，没有 e2e 就根本发现不了。
 *   2. **未实现的上行包**：mineflayer 会发一批服务端尚未实现的消息（如聊天）。
 *      服务端必须**静默忽略并保持连接**，而不是把它当协议错误断开。
 */

import type { CaseDefinition } from "../case.ts";
import { expectEq, expectTrue } from "../assert/expect.ts";
import { blockNameAt } from "../assert/surface.ts";
import { delay, waitForCondition } from "../bot/wait.ts";

/**
 * 心跳往返的等待上限。
 *
 * vanilla 的心跳间隔是 15 秒（Cubium 同量级），故这里必须给足——取 25 秒：
 * 用例开始时服务端可能刚发过一次心跳，最多要等一整个间隔。
 */
const KEEP_ALIVE_TIMEOUT_MS = 25_000;

/** 发送聊天后观察连接健康的时间。 */
const CHAT_OBSERVE_MS = 1_500;

export const protocolCases: readonly CaseDefinition[] = [
    {
        id: "protocol/keep_alive_round_trip",
        title: "服务端心跳被客户端应答（连接不会因漏答被踢）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: false,
        skipReason: null,
        async run({ bot, trace }): Promise<Record<string, unknown>> {
            // 两个方向都要观察：服务端主动发 keep_alive，客户端必须回一条 keep_alive。
            // nmp 自动应答心跳；服务端把入站包静默丢弃（未登记/未处理）时，回包计数恒为 0，
            // 连接的存活就只能靠"服务端宽容"维持——这是必须钉住的行为。
            await waitForCondition(() => trace.count("keep_alive") > 0, {
                timeoutMs: KEEP_ALIVE_TIMEOUT_MS,
                pollMs: 200,
                what: "收到服务端心跳 keep_alive",
                describe: () => "服务端未在超时内下发任何心跳（KEEP_ALIVE_INTERVAL 配置异常？）",
            });

            const clientSentKeepAlive = (): boolean =>
                trace.all.some((entry) => entry.direction === "C2S" && entry.name === "keep_alive");
            await waitForCondition(clientSentKeepAlive, {
                timeoutMs: 15_000,
                pollMs: 100,
                what: "客户端回发 keep_alive",
                describe: () => "客户端未应答心跳（连接可能在下一轮心跳超时后被断开）",
            });

            const clientState = (bot._client as unknown as { state?: string }).state;
            expectEq(clientState, "play", "心跳往返后客户端应仍处于 play 阶段");
            return { serverSentKeepAlive: true, clientAnsweredKeepAlive: true };
        },
    },

    {
        id: "protocol/chat_message_keeps_connection_healthy",
        title: "发送聊天消息后连接保持健康（未实现的上行包须静默忽略）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: false,
        skipReason: null,
        async run({ bot, trace, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            const sentBefore = trace.count("chat_message") + trace.count("chat_command");
            bot.chat("e2e protocol probe");
            await delay(CHAT_OBSERVE_MS);

            // 判据只取"连接是否被搞坏"：服务端实现了聊天就回广播，未实现就静默忽略，
            // 两种都不允许断开连接或把客户端踢出。
            const clientState = (bot._client as unknown as { state?: string }).state;
            expectEq(clientState, "play", "发送聊天后客户端应仍处于 play 阶段");
            expectTrue(
                trace.count("chat_message") + trace.count("chat_command") > sentBefore,
                "聊天消息未真正发出（用例没有测到任何东西）",
            );
            expectTrue(
                blockNameAt(bot, spawnX, surfaceY, spawnZ) !== undefined,
                "发送聊天后仍应能读方块——连接异常中断会使区块数据不可用",
            );

            return {
                connectionHealthyAfterChat: true,
                chatEchoObserved:
                    trace.count("player_chat") + trace.count("system_chat") + trace.count("chat") > 0,
            };
        },
    },
];
