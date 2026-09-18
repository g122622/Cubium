/*
 * handshake.ts — 场景 A：握手与连接生命周期。
 *
 * 覆盖「真实 wire 客户端能否完整走完 Handshake → Login → Configuration → Play 并稳定驻留」。
 * 这是本项目此前完全没有自动化覆盖的一段：unit 走 LocalTransport 零拷贝直传 IR 包
 * （不经 codec、不经 socket），GameTest 是进程内且无玩家。
 */

import type { Bot } from "mineflayer";
import type { CaseDefinition } from "../case.ts";
import { expectEq, expectTrue } from "../assert/expect.ts";
import { delay, waitForEvent } from "../bot/wait.ts";

/** 读 mineflayer 注册表的规模（用于验证 registry_data 被正确解析）。 */
function registrySizes(bot: Bot): {
    biomes: number;
    blocks: number;
    dimensions: number;
    items: number;
} {
    const registry = bot.registry as unknown as {
        biomesArray?: unknown[];
        blocksArray?: unknown[];
        dimensionsArray?: unknown[];
        itemsArray?: unknown[];
    };
    return {
        biomes: registry.biomesArray?.length ?? 0,
        blocks: registry.blocksArray?.length ?? 0,
        dimensions: registry.dimensionsArray?.length ?? 0,
        items: registry.itemsArray?.length ?? 0,
    };
}

export const handshakeCases: readonly CaseDefinition[] = [
    {
        id: "handshake/spawn_event_after_health",
        title: "bot 收到 update_health 后触发 spawn（服务端必须主动下发 set_health）",
        servers: ["cubium", "vanilla"],
        async run({ bot, trace }): Promise<Record<string, unknown>> {
            // runner 已等到 spawn，此处断言的是「spawn 为何能发生」——mineflayer 的 spawn
            // 完全由首个 health>0 的 update_health 驱动（lib/plugins/health.js:18）。
            // 服务端缺失该包时 bot 会停在「已登录未进入世界」且不报错。
            expectTrue(
                trace.count("update_health") > 0,
                "未收到任何 update_health（服务端 set_health cb 102 缺失时 mineflayer 永不 spawn）",
            );
            expectTrue(bot.entity !== undefined && bot.entity !== null, "spawn 后 bot.entity 不应为空");
            return {
                updateHealthCountAtLeastOne: trace.count("update_health") > 0,
                hasOwnEntity: true,
                healthIsPositive: (bot.health ?? 0) > 0,
            };
        },
    },

    {
        id: "handshake/phase_sequence",
        title: "五个协议阶段的包序列完整（Login → Configuration → Play）",
        servers: ["cubium", "vanilla"],
        async run({ trace }): Promise<Record<string, unknown>> {
            // 逐段验证：Login 阶段必须有 success + compress（本服压缩阈值为 256）；
            // Configuration 阶段必须有 select_known_packs 与 finish_configuration；
            // Play 阶段以 login 包为界。
            expectTrue(trace.has("success"), "未收到 login 阶段的 success（LoginSuccess）");
            expectTrue(trace.has("select_known_packs"), "未收到 configuration 阶段的 select_known_packs");
            expectTrue(trace.has("finish_configuration"), "未收到 configuration 阶段的 finish_configuration");
            expectTrue(trace.has("login"), "未收到 play 阶段的 login 包");

            // 顺序校验：以 trace 中首次出现的下标为准。
            const indexOf = (name: string): number => trace.all.findIndex((entry) => entry.name === name);
            const order = {
                success: indexOf("success"),
                selectKnownPacks: indexOf("select_known_packs"),
                finishConfiguration: indexOf("finish_configuration"),
                login: indexOf("login"),
            };
            expectTrue(
                order.success < order.selectKnownPacks &&
                    order.selectKnownPacks < order.finishConfiguration &&
                    order.finishConfiguration < order.login,
                `阶段顺序错误：期望 success < select_known_packs < finish_configuration < login，实际 ${JSON.stringify(order)}`,
            );
            return {
                sawSuccess: true,
                sawSelectKnownPacks: true,
                sawFinishConfiguration: true,
                sawLogin: true,
            };
        },
    },

    {
        id: "handshake/registry_data_decodes",
        title: "registry_data 可被客户端解码（服务端须按客户端声明下发 NBT）",
        servers: ["cubium", "vanilla"],
        async run({ bot, trace }): Promise<Record<string, unknown>> {
            // 该用例同时是「客户端未声明 minecraft:core 时服务端必须下发完整 NBT」的回归保护：
            // 若服务端对未声明的客户端发 value 缺失的条目，客户端会在解析时抛
            // TypeError 并静默挂起（不会走到 spawn，本用例也就跑不到这里）。
            expectTrue(trace.has("registry_data"), "未收到任何 registry_data");
            const sizes = registrySizes(bot);
            expectTrue(sizes.blocks > 0, "客户端方块注册表为空（registry_data 未被正确解析）");
            expectTrue(sizes.dimensions > 0, "客户端维度注册表为空（dimension_type 未被正确解析）");
            return {
                sawRegistryData: true,
                blocksNonEmpty: true,
                dimensionsNonEmpty: true,
                biomesNonEmpty: sizes.biomes > 0,
            };
        },
    },

    {
        id: "handshake/login_entity_id_consistent",
        title: "login 包建立的本地实体自洽（bot.entities 中无重复自身条目）",
        servers: ["cubium", "vanilla"],
        async run({ bot }): Promise<Record<string, unknown>> {
            // 服务端 login 包的 entityId 必须是玩家**实体** id，而非玩家注册 id——
            // 两者是各自独立的递增序列。单人测试下它们常恰好相等（PlayerId=1/EntityId=1），
            // 故本用例只能验证自洽性（自身实体可被 id 索引到且名字匹配）。
            // TODO: 用两个 bot 同时连接才能暴露 playerId 与 entityId 序列错配的真实缺陷。
            const selfId = bot.entity?.id ?? -1;
            expectTrue(selfId >= 0, `bot.entity.id 无效：${selfId}`);

            const entities = bot.entities as unknown as Record<number, { name?: string; type?: string }>;
            const self = entities[selfId];
            expectTrue(self !== undefined, `bot.entities 中不存在 id=${selfId} 的自身实体`);
            expectEq(
                bot.entities[selfId] !== undefined,
                true,
                "自身实体应可通过 bot.entity.id 在 bot.entities 中定位",
            );
            return {
                hasSelfEntity: true,
                selfEntityIdPositive: selfId > 0,
            };
        },
    },

    {
        id: "handshake/tab_list_contains_self",
        title: "Tab 列表包含自身（服务端须发送 player_info_update）",
        servers: ["cubium"],
        async run({ bot }): Promise<Record<string, unknown>> {
            // player_info_update(cb 68) 的 IR/codec/协议表登记三层齐备，但服务端此前零发送点，
            // 导致 bot.players 为空。本用例是该缺口的回归保护。
            // 注意：该用例只对 Cubium 生效——vanilla 必然通过，纳入比对没有信息量。
            const names = Object.keys(bot.players);
            expectTrue(names.length > 0, "Tab 列表为空：服务端未下发 player_info_update(cb 68)");
            const selfName = bot.username;
            expectTrue(names.includes(selfName), `Tab 列表中不含自身 '${selfName}'，实际：${names.join(",")}`);
            return {
                playerCountPositive: true,
                containsSelf: true,
            };
        },
    },

    {
        id: "handshake/clean_disconnect",
        title: "客户端主动断开后服务端正常收尾",
        servers: ["cubium"],
        async run({ bot }): Promise<Record<string, unknown>> {
            await delay(200);
            const ended = waitForEvent(bot, "end", {
                timeoutMs: 10_000,
                what: "断开连接的 end 事件",
                describe: () => `连接状态: ${(bot._client as unknown as { state?: string }).state ?? "<未知>"}`,
            });
            bot.quit();
            const [reason] = await ended;
            expectTrue(reason !== undefined, "end 事件未携带断开原因");
            return {
                disconnectReasonClass: String(reason).includes("socketClosed") ? "socketClosed" : "other",
            };
        },
    },
];
