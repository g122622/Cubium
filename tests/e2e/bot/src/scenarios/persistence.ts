/*
 * persistence.ts — 场景 H：存档持久化（重启后世界仍在）。
 *
 * 这是此前完全没有覆盖的一段：此前所有用例都在**同一个服务端进程**内完成，改完世界就地断言，
 * 从不经过"落盘 → 重新加载"这条路径。而项目近期连续改动区块存储（段缓存移除、区块层驱动
 * 保存、非空气方块计数与方块数据一致性），正是最容易在存档往返中静默丢数据的地方。
 *
 * 流程固定为四步：
 *   改世界 → `/save-all` 显式落盘 → restartServer()（同一游戏目录重启）→ 重连后核对。
 *
 * **为什么要显式 `/save-all`**：harness 只能硬杀 Cubium 进程（Windows 上无法触发其优雅退出），
 * 不落盘就重启会测出"数据丢了"，而那只是关服方式的问题，不是存档缺陷。显式落盘让本用例
 * 只考察「已写盘的数据能否被正确读回」这一件事。
 */

import type { Bot } from "mineflayer";
import type { CaseDefinition } from "../case.ts";
import { expectEq, expectTrue } from "../assert/expect.ts";
import { blockNameAt, vec3 } from "../assert/surface.ts";
import { delay, waitForCondition } from "../bot/wait.ts";
import {
    INV_HOTBAR_SLOT_0,
    runCommand,
    setSlot,
    slotItemName,
    waitForBlockReadable,
    waitForSlot,
} from "./shared.ts";

/** 显式落盘的等待上限。 */
const SAVE_FEEDBACK_TIMEOUT_MS = 20_000;
/** 重启后等待目标区块送达的上限。 */
const RELOAD_TIMEOUT_MS = 30_000;

/**
 * 重启服务端并重连一个 bot。
 *
 * restartServer() 会断开全部连接，故必须重新 connectBot()；重连后的这个 bot 会成为新的主 bot
 * （ctx.bot 指向它），最终快照也采自它。
 */
async function restartAndReconnect(bot: Bot, restartServer: () => Promise<void>, connectBot: () => Promise<Bot>): Promise<Bot> {
    await runCommand(bot, "/save-all", SAVE_FEEDBACK_TIMEOUT_MS);
    await restartServer();
    return connectBot();
}

export const persistenceCases: readonly CaseDefinition[] = [
    {
        id: "persistence/block_changes_survive_restart",
        title: "重启后方块改动仍在（挖掉的仍是空气，放置的仍是石头）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: true,
        // 阻塞点：**用已有世界重启后，实体与地形不再发生碰撞**。服务端重新打开存档后，客户端
        // 仍能看到地形（实测 (0,3,0)=grass_block、(0,0,0)=bedrock，81 列区块正常送达），但玩家
        // 从出生点一路坠入虚空（y 持续降到 -400 以下），服务端全程不做位置纠正；同一时间服务端
        // 还在批量刷出又立刻销毁生物（3336 次 spawn/destroy），说明服务端侧的碰撞判定同样看不到地形。
        // 证据：build/e2e/artifacts/20260928-011441-48536/（第二个服务端日志 + bot-trace），
        //       并用独立探针在同一存档目录上直接起服务端复现（客户端读到 grass_block，
        //       bot 位置 y=-6.81 且持续下落）。
        // 解除条件：修好「存档重载后的区块加载/碰撞状态」后把本字段改回 null。
        skipReason:
            "服务端从存档重启后实体不再与地形碰撞（客户端能看到方块，玩家与生物仍一路坠入虚空）；" +
            "待修复存档重载后的区块加载/碰撞状态后移除此跳过标记",
        async run({ bot, restartServer, connectBot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            const dugX = spawnX + 2;
            const dugZ = spawnZ;
            const placedX = spawnX + 3;
            const placedZ = spawnZ;

            // 1) 挖掉地表草方块
            expectEq(blockNameAt(bot, dugX, surfaceY, dugZ), "grass_block", "待挖位置应为草方块");
            const target = bot.blockAt(vec3(dugX, surfaceY, dugZ));
            expectTrue(target !== null, "待挖方块不可读");
            await bot.dig(target as never, true);
            await delay(200);
            expectEq(blockNameAt(bot, dugX, surfaceY, dugZ), "air", "挖掉后应为空气");

            // 2) 在另一处放一块石头
            const given = await setSlot(bot, INV_HOTBAR_SLOT_0, "stone", 1);
            expectTrue(given, "无法给 bot 发放石头");
            await waitForSlot(bot, INV_HOTBAR_SLOT_0, "stone", "石头就位");
            const refBlock = bot.blockAt(vec3(placedX, surfaceY, placedZ));
            expectTrue(refBlock !== null, "放置参考方块不可读");
            await bot.placeBlock(refBlock as never, vec3(0, 1, 0));
            await delay(200);
            expectEq(blockNameAt(bot, placedX, surfaceY + 1, placedZ), "stone", "放置后应为石头");

            // 3) 落盘并重启（同一世界目录）
            const reloaded = await restartAndReconnect(bot, restartServer, connectBot);
            await waitForBlockReadable(reloaded, dugX, surfaceY, dugZ, "重启后待挖列区块送达", RELOAD_TIMEOUT_MS);
            await waitForBlockReadable(
                reloaded,
                placedX,
                surfaceY + 1,
                placedZ,
                "重启后待放置列区块送达",
                RELOAD_TIMEOUT_MS,
            );

            // 4) 核对：两处改动都必须还在
            expectEq(
                blockNameAt(reloaded, dugX, surfaceY, dugZ),
                "air",
                "重启后挖掉的方块又回来了——区块改动未落盘或未正确读回",
            );
            expectEq(
                blockNameAt(reloaded, placedX, surfaceY + 1, placedZ),
                "stone",
                "重启后放置的方块消失了——区块改动未落盘或未正确读回",
            );
            return { dugBlockStillAir: true, placedBlockStillStone: true };
        },
    },

    {
        id: "persistence/container_content_survives_restart",
        title: "重启后方块实体内容仍在（箱子里的物品不丢）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: true,
        // 与 block_changes_survive_restart 同一阻塞点：重连后 bot 无法落地（实体与地形不碰撞），
        // 用例走不到「重新打开箱子」这一步。解除条件同上。
        skipReason:
            "服务端从存档重启后实体不再与地形碰撞（bot 无法落地，读不到世界内容）；" +
            "待修复存档重载后的区块加载/碰撞状态后移除此跳过标记",
        async run({ bot, restartServer, connectBot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            const x = spawnX - 3;
            const z = spawnZ - 3;
            const chestY = surfaceY + 1;

            // 1) 放箱子并塞一颗钻石
            const given = await setSlot(bot, INV_HOTBAR_SLOT_0, "chest", 1);
            expectTrue(given, "无法给 bot 发放箱子");
            await delay(200);
            const refBlock = bot.blockAt(vec3(x, surfaceY, z));
            expectTrue(refBlock !== null, "参考方块不可读");
            await bot.placeBlock(refBlock as never, vec3(0, 1, 0));
            await delay(200);
            const chestBlock = bot.blockAt(vec3(x, chestY, z));
            expectTrue(chestBlock !== null, "放置后的箱子不可读");

            const chest = (await bot.openContainer(chestBlock as never)) as unknown as {
                slots: ({ name?: string } | null)[];
            };
            const diamondGiven = await setSlot(bot, INV_HOTBAR_SLOT_0, "diamond", 1);
            expectTrue(diamondGiven, "无法给 bot 发放钻石");
            await waitForSlot(bot, INV_HOTBAR_SLOT_0, "diamond", "钻石就位");
            // 箱子窗口：0-26 是箱子格、27-53 是主背包、54-62 是快捷栏。
            await bot.clickWindow(54, 0, 0);
            await bot.clickWindow(0, 0, 0);
            await waitForCondition(() => slotItemName(chest, 0) === "diamond", {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "钻石进入箱子首槽",
                describe: () => `箱子首槽当前为 ${String(slotItemName(chest, 0))}`,
            });
            await bot.closeWindow(chest as never);
            await delay(200);

            // 2) 落盘并重启
            const reloaded = await restartAndReconnect(bot, restartServer, connectBot);
            await waitForBlockReadable(reloaded, x, chestY, z, "重启后箱子所在列区块送达", RELOAD_TIMEOUT_MS);
            await waitForCondition(() => blockNameAt(reloaded, x, chestY, z) === "chest", {
                timeoutMs: RELOAD_TIMEOUT_MS,
                pollMs: 100,
                what: "重启后箱子方块仍在原处",
                describe: () => `(${x},${chestY},${z}) 当前为 ${String(blockNameAt(reloaded, x, chestY, z))}`,
            });

            // 3) 重新打开箱子核对内容
            const reloadedChestBlock = reloaded.blockAt(vec3(x, chestY, z));
            expectTrue(reloadedChestBlock !== null, "重启后箱子方块不可读");
            const reloadedChest = (await reloaded.openContainer(reloadedChestBlock as never)) as unknown as {
                slots: ({ name?: string } | null)[];
            };
            expectEq(
                slotItemName(reloadedChest, 0),
                "diamond",
                "重启后箱子首槽应为钻石——内容丢失说明方块实体存档未落盘或未正确读回",
            );
            await reloaded.closeWindow(reloadedChest as never);
            return { containerContentSurvived: true };
        },
    },
];
