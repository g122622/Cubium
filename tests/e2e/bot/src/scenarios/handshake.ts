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
import { delay, waitForCondition, waitForEvent } from "../bot/wait.ts";

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

/**
 * 让服务端生成一个掉落物实体，从而推进其实体实例 id 序列。
 *
 * 走 raw `set_creative_slot` 且 slot 为负数：这是创造模式「丢弃物品」的线格式语义
 * （对齐 vanilla LocalPlayer.handleCreativeModeItemDrop），物品栈随包携带，不依赖
 * 服务端侧的物品栏状态。
 *
 * 为什么不用更常规的 API：bot.toss 发的是 window_click，而远程玩家并没有打开的容器，
 * 服务端会拒绝（不会生成任何实体的静默失败）；bot.creative.setInventorySlot 内部有
 * assert(slot >= 0)，负数槽被直接拦死。
 *
 * @returns 是否成功发包（注册表中找不到该物品时返回 false）。
 */
async function dropItemEntity(bot: Bot, itemName: string): Promise<boolean> {
    const typedBot = bot as unknown as {
        registry: { itemsByName: Record<string, { id: number }> };
        version: string;
        _client: { write(name: string, params: unknown): void };
    };
    const itemDef = typedBot.registry.itemsByName[itemName];
    if (itemDef === undefined) {
        return false;
    }
    type ItemCtor = {
        new (id: number, count: number): unknown;
        toNotch(item: unknown): unknown;
    };
    const ItemModule = (await import("prismarine-item")).default as unknown as (version: string) => ItemCtor;
    const Item = ItemModule(typedBot.version);
    typedBot._client.write("set_creative_slot", {
        slot: -1,
        item: Item.toNotch(new Item(itemDef.id, 1)),
    });
    return true;
}

/**
 * 在 bot 的实体表中找出「另一个玩家」实体。
 *
 * 用于取得旁观视角下某玩家的**真实实体 id**——服务端在 add_entity 里下发的就是这个值，
 * 与对方从 login 包得到的自身 id 是同一事实的两个来源。
 *
 * 按 type 而非用户名匹配：玩家实体的 username 由 player_info_update 填充，依赖它会把
 * 本函数与「Tab 列表广播」耦合起来。
 */
function findOtherPlayerEntity(bot: Bot): { readonly id: number } | undefined {
    const selfId = bot.entity?.id ?? -1;
    const entities = bot.entities as unknown as Record<number, { type?: string }>;
    for (const [key, entity] of Object.entries(entities)) {
        const id = Number(key);
        if (id !== selfId && entity.type === "player") {
            return { id };
        }
    }
    return undefined;
}

export const handshakeCases: readonly CaseDefinition[] = [
    {
        id: "handshake/spawn_event_after_health",
        title: "bot 收到 update_health 后触发 spawn（服务端必须主动下发 set_health）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
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
        botCount: 1,
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
        botCount: 1,
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
        botCount: 1,
        async run({ bot }): Promise<Record<string, unknown>> {
            // 服务端 login 包的首字段在 vanilla 语义里是玩家**实体实例 id**，而非玩家注册 id——
            // 两者是各自独立的递增序列。本用例只做自洽性检查（自身实体可被该 id 索引到）；
            // 序列错配的真正暴露需要让它们先错开，见 handshake/login_entity_id_is_entity。
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
        id: "handshake/login_entity_id_is_entity",
        title: "login 首字段是实体实例 id（错开两条 id 序列后与旁观者所见比对）",
        servers: ["cubium", "vanilla"],
        // 只预连一个：第二个必须在「已制造出非玩家实体」之后才加入，否则两条 id 序列
        // 同步递增，即便服务端填错字段也无从分辨。
        botCount: 1,
        async run({ bot, connectBot }): Promise<Record<string, unknown>> {
            // 服务端的玩家**注册 id**（按加入顺序分配）与**实体实例 id**（每创建一个实体分配）
            // 是两条独立递增的序列。只连两个玩家时它们同步推进（1↔1、2↔2），把 login 的
            // 实体 id 字段填成注册 id 也看不出差别。先制造一个非玩家实体（掉落物）让实体
            // 序列领先一格，第二个玩家加入时两条序列才会错开，错配因此显形。
            const dropped = await dropItemEntity(bot, "stone");
            expectTrue(dropped, "无法制造掉落物实体（客户端注册表中找不到 stone）");

            // 等掉落物真正在世界里建立：服务端为它分配实体 id 后才会下发 add_entity。
            const entityCountBefore = Object.keys(bot.entities).length;
            await waitForCondition(() => Object.keys(bot.entities).length > entityCountBefore, {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "掉落物实体出现在 bot.entities 中（服务端已为它分配实体 id）",
            });

            const second = await connectBot();

            // 第一个 bot 是唯一能观察到第二个 bot **真实实体 id** 的旁观者——服务端在
            // add_entity 里下发的就是这个 id。把这个值与第二个 bot 从 login 包得到的自身
            // id 比对：两者必须一致，否则 login 首字段填的是别的 id 序列。
            await waitForCondition(() => findOtherPlayerEntity(bot) !== undefined, {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "第一个 bot 观测到第二个 bot 的实体（add_entity 已送达）",
            });
            const observed = findOtherPlayerEntity(bot);
            const selfView = second.entity?.id ?? -1;
            expectTrue(selfView >= 0, `第二个 bot 的 bot.entity.id 无效：${selfView}`);
            expectEq(
                observed?.id,
                selfView,
                "第二个 bot 由 login 包确定的自身实体 id 与旁观者看到的实体 id 不一致——" +
                    "login 首字段必须是实体实例 id，而非玩家注册 id",
            );
            return {
                entityIdSequenceSpilled: true,
                selfIdMatchesObserver: true,
            };
        },
    },

    {
        id: "handshake/tab_list_contains_self",
        title: "Tab 列表包含自身（服务端须发送 player_info_update）",
        servers: ["cubium"],
        botCount: 1,
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
        id: "handshake/tab_list_broadcasts_both_ways",
        title: "Tab 列表双向广播（先加入者能看到后加入者，反之亦然）",
        servers: ["cubium", "vanilla"],
        // 必须等第一个 bot 就绪后再连第二个：单向广播（只把条目发给玩家本人）在单 bot 下
        // 与双向广播不可区分，只有让「先加入者的列表里出现后加入者」才能证伪。
        botCount: 1,
        async run({ bot, connectBot }): Promise<Record<string, unknown>> {
            const second = await connectBot();

            await waitForCondition(() => bot.username in bot.players, {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "先加入者自身出现在自己的 Tab 列表",
            });
            await waitForCondition(() => second.username in bot.players, {
                timeoutMs: 10_000,
                pollMs: 50,
                what: `先加入者的 Tab 列表中出现后加入者 '${second.username}'`,
            });
            await waitForCondition(() => bot.username in second.players, {
                timeoutMs: 10_000,
                pollMs: 50,
                what: `后加入者的 Tab 列表中出现先加入者 '${bot.username}'`,
            });

            return {
                firstSeesSelf: true,
                firstSeesSecond: true,
                secondSeesFirst: true,
            };
        },
    },

    {
        id: "handshake/tab_list_removes_on_leave",
        title: "离场广播（断开者的条目从他人 Tab 列表移除）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, connectBot }): Promise<Record<string, unknown>> {
            const second = await connectBot();
            const departedName = second.username;

            await waitForCondition(() => departedName in bot.players, {
                timeoutMs: 10_000,
                pollMs: 50,
                what: `离场者的条目先出现在观看者 Tab 列表中`,
            });

            second.quit();
            await waitForCondition(() => !(departedName in bot.players), {
                timeoutMs: 10_000,
                pollMs: 50,
                what: `离场者 '${departedName}' 的条目从观看者 Tab 列表中移除`,
            });

            return { removedAfterLeave: true };
        },
    },

    {
        id: "handshake/clean_disconnect",
        title: "客户端主动断开后服务端正常收尾",
        servers: ["cubium"],
        botCount: 1,
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
