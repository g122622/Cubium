/*
 * block-interaction.ts — 场景 C：方块交互（挖掘 / 放置）。
 *
 * 验证玩家动作的**闭合回路**：客户端发 player_action / use_item_on → 服务端改世界并回
 * block_update(cb 8) → 客户端据此判定动作完成。mineflayer 的 dig() 依赖 block_change
 * 的 `newBlock.type===0` 才 resolve，placeBlock() 若 5 秒内收不到 block update 会直接抛
 * "Server refused to place ..."——两条路径都能把服务端不回包暴露成用例失败。
 *
 * 用例统一在 creative 模式下运行：挖掘瞬破，从而不受 update_attributes(cb 129) 尚未实现
 * （该包提供 block_break_speed 属性）的影响。
 *
 * 坐标约定：所有交互点限制在出生点 ±2 格内。mineflayer 与服务端都要求目标处于 bot 的
 * 触及范围（约 4.5 格）；超出时服务端拒绝放置，用例会以「与协议无关的原因」失败。
 * 各用例取不同的 (dx,dz) 错开，避免相互干扰（虽然每用例是独立进程与新世界）。
 */

import type { CaseDefinition } from "../case.ts";
import { expectEq, expectTrue } from "../assert/expect.ts";
import { blockNameAt, vec3 } from "../assert/surface.ts";
import { delay } from "../bot/wait.ts";

/** 给 bot 的快捷栏发放方块（creative 模式）。 */
async function giveBlockToBot(bot: never, itemName: string, count: number): Promise<boolean> {
    const typedBot = bot as unknown as {
        creative: { setInventorySlot(slot: number, item: unknown): Promise<void> };
        registry: { itemsByName: Record<string, { id: number }> };
        version: string;
    };
    const itemDef = typedBot.registry.itemsByName[itemName];
    if (itemDef === undefined) {
        return false;
    }
    const ItemModule = (await import("prismarine-item")).default as unknown as (
        version: string,
    ) => new (id: number, count: number) => unknown;
    const Item = ItemModule(typedBot.version);
    await typedBot.creative.setInventorySlot(36, new Item(itemDef.id, count));
    return true;
}

export const blockInteractionCases: readonly CaseDefinition[] = [
    {
        id: "block-interaction/dig_grass_block",
        title: "挖掘地表草方块：客户端完成判定 + 世界实际改变",
        servers: ["cubium", "vanilla"],
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            const x = spawnX + 2;
            const z = spawnZ;
            expectEq(
                blockNameAt(bot, x, surfaceY, z),
                "grass_block",
                `目标点 (${x},${surfaceY},${z}) 应为草方块`,
            );

            const target = bot.blockAt(vec3(x, surfaceY, z));
            expectTrue(target !== null, "目标方块不可读（区块未加载）");
            // dig 的 promise 只有在客户端收到「新方块为空气」的 block_change 后才 resolve，
            // 故它 resolve 本身即证明服务端回了包。
            await bot.dig(target as never, true);

            await delay(200);
            expectEq(blockNameAt(bot, x, surfaceY, z), "air", `挖掉 (${x},${surfaceY},${z}) 后应为空气`);
            return { digResolved: true, becameAir: true };
        },
    },

    {
        id: "block-interaction/dig_emits_update_and_ack",
        title: "挖掘触发 block_update 与 block_changed_ack 下发",
        servers: ["cubium", "vanilla"],
        async run({ bot, trace, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            const x = spawnX + 2;
            const z = spawnZ + 2;
            const updateBefore = trace.count("block_change");
            // cb 4 在 1.21.11 的官方名是 block_changed_ack，但 Prismarine 生态沿用 1.16 旧别名
            // acknowledge_player_digging 指代同一个包（项目 docs/未实现的数据包.md:104
            // 也记录了这个别名陷阱）。两侧名字都接受，不把别名当唯一真相。
            const ackNames = ["acknowledge_player_digging", "block_changed_ack"];
            const ackBefore = ackNames.reduce((sum, name) => sum + trace.count(name), 0);

            const target = bot.blockAt(vec3(x, surfaceY, z));
            expectTrue(target !== null, "目标方块不可读");
            await bot.dig(target as never, true);
            await delay(300);

            expectTrue(
                trace.count("block_change") > updateBefore,
                "挖掘后未收到 block_update(cb 8)——客户端将永远无法判定挖掘完成",
            );
            const ackAfter = ackNames.reduce((sum, name) => sum + trace.count(name), 0);
            expectTrue(
                ackAfter > ackBefore,
                `挖掘后未收到 block_changed_ack(cb 4，Prismarine 名 ${ackNames.join("/")})——` +
                    `服务端未确认客户端方块预测序列号`,
            );
            return { gotBlockUpdate: true, gotBlockChangedAck: true };
        },
    },

    {
        id: "block-interaction/place_stone_block",
        title: "放置方块：客户端不超时且世界实际改变",
        servers: ["cubium", "vanilla"],
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            const gave = await giveBlockToBot(bot as never, "stone", 64);
            expectTrue(gave, "无法给 bot 发放石头（itemsByName 中找不到 stone）");

            const x = spawnX + 2;
            const z = spawnZ - 2;
            const refBlock = bot.blockAt(vec3(x, surfaceY, z));
            expectTrue(refBlock !== null, "参考方块不可读");

            // 在参考方块的顶面放置。mineflayer 若收不到 block update 会抛
            // "Server refused to place ..."——该异常即服务端未回包的信号。
            await bot.placeBlock(refBlock as never, vec3(0, 1, 0));

            await delay(200);
            expectEq(
                blockNameAt(bot, x, surfaceY + 1, z),
                "stone",
                `在 (${x},${surfaceY + 1},${z}) 放置后应为石头`,
            );
            return { placeResolved: true, placedStone: true };
        },
    },

    {
        id: "block-interaction/repeated_place_is_stable",
        title: "对同一位置重复放置不使客户端挂起（失败路径仍需应答）",
        servers: ["cubium", "vanilla"],
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 服务端的 PlacementBlockUpdateGuard 是 RAII：无论放置成功、失败还是提前 return，
            // 析构时都会下发命中的方块更新。本用例走「第二次放置必然失败」的路径，
            // 验证失败路径同样有应答——否则客户端会超时抛错。
            const gave = await giveBlockToBot(bot as never, "stone", 64);
            expectTrue(gave, "无法给 bot 发放石头");

            const x = spawnX - 2;
            const z = spawnZ + 2;
            const refBlock = bot.blockAt(vec3(x, surfaceY, z));
            expectTrue(refBlock !== null, "参考方块不可读");

            await bot.placeBlock(refBlock as never, vec3(0, 1, 0));
            await delay(200);
            expectEq(blockNameAt(bot, x, surfaceY + 1, z), "stone", "首次放置应成功");

            // 第二次：目标位置已被占用。无论成功与否，**不能**让客户端因服务端不回包而超时。
            let secondThrew = false;
            try {
                const occupied = bot.blockAt(vec3(x, surfaceY + 1, z));
                await bot.placeBlock(occupied as never, vec3(0, 1, 0));
            } catch (err) {
                secondThrew = true;
                const message = (err as Error).message;
                expectTrue(
                    !message.includes("was still air") && !message.includes("did not answer"),
                    `重复放置时服务端未回方块更新（回包缺失的表现）：${message}`,
                );
            }

            // 关键断言：连接与区块读取必须仍然正常。
            await delay(200);
            expectTrue(
                blockNameAt(bot, x, surfaceY + 1, z) !== undefined,
                "重复放置后区块不可读——连接可能已异常",
            );
            return {
                firstPlaceSucceeded: true,
                secondAttemptResolved: true,
                secondThrew,
            };
        },
    },

    {
        id: "block-interaction/dig_place_dig_roundtrip",
        title: "挖→放→再挖的完整往返",
        servers: ["cubium", "vanilla"],
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            const gave = await giveBlockToBot(bot as never, "stone", 64);
            expectTrue(gave, "无法给 bot 发放石头");

            const x = spawnX + 3;
            const z = spawnZ - 2;

            // 1. 先挖掉地表草方块
            const grass = bot.blockAt(vec3(x, surfaceY, z));
            expectTrue(grass !== null, "目标草方块不可读");
            await bot.dig(grass as never, true);
            await delay(200);
            expectEq(blockNameAt(bot, x, surfaceY, z), "air", "第一步：挖后应为空气");

            // 2. 在挖出的空位放石头（以下方的泥土为参考面）
            const refBlock = bot.blockAt(vec3(x, surfaceY - 1, z));
            expectTrue(refBlock !== null, "参考方块（地表下方）不可读");
            // 服务端在 heldItem 为空时直接 return、不进入放置路径。挖掘动作发生在发放物品
            // 之后数百毫秒，此处重新确认手持状态，排除「手持物品在挖掘过程中失效」这一可能。
            const stillHolding = await giveBlockToBot(bot as never, "stone", 64);
            expectTrue(stillHolding, "重新发放石头失败");
            await bot.placeBlock(refBlock as never, vec3(0, 1, 0));
            await delay(200);
            expectEq(blockNameAt(bot, x, surfaceY, z), "stone", "第二步：放置后应为石头");

            // 3. 再挖掉
            const stone = bot.blockAt(vec3(x, surfaceY, z));
            expectTrue(stone !== null, "放置的石头不可读");
            await bot.dig(stone as never, true);
            await delay(200);
            expectEq(blockNameAt(bot, x, surfaceY, z), "air", "第三步：再挖后应为空气");

            return { roundtripComplete: true, finalState: "air" };
        },
    },

    {
        id: "block-interaction/connection_stays_healthy",
        title: "交互全程未因未登记的上行包断开连接",
        servers: ["cubium", "vanilla"],
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // mineflayer 会发送一批服务端尚未登记的上行包（player_loaded / swing /
            // client_tick_end 等）。服务端对未登记包的处理是静默丢弃——既不断连也不报错。
            // 本用例是那个行为的回归保护：若将来有人误把未登记包当成协议错误处理，这里会红。
            // 这些包的 id 与已登记集合零交集，不会误解析成别的包。
            const x = spawnX - 2;
            const z = spawnZ;
            const target = bot.blockAt(vec3(x, surfaceY, z));
            expectTrue(target !== null, "目标方块不可读");
            await bot.dig(target as never, true);
            await delay(500);

            const clientState = (bot._client as unknown as { state?: string }).state;
            expectEq(clientState, "play", "交互后客户端应仍处于 play 阶段");
            expectTrue(
                blockNameAt(bot, x, surfaceY - 1, z) !== undefined,
                "交互后仍应能读取方块——连接异常中断会使区块数据不可用",
            );
            return { stillInPlayState: true };
        },
    },
];
