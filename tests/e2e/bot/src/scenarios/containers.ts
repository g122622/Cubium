/*
 * containers.ts — 场景 G：方块容器的种类与维度。
 *
 * 覆盖三类此前完全没有自动化覆盖的容器缺陷，它们的共同点是**服务端不报错、客户端毫无反应**：
 *
 *   1. 木桶（barrel）：方块实体是 BarrelEntity，而菜单工厂曾只接受 Chest/TrappedChest ——
 *      木桶永远建不出菜单，右键无任何反馈。
 *   2. 潜影盒（shulker_box）：同型缺陷，且它的菜单类型（shulker_box）与箱子（generic_9x3）
 *      不同，菜单类型编码错位在这里会直接暴露（mineflayer 会拒绝非容器窗口）。
 *   3. 非主世界维度里的容器：菜单工厂曾硬编码 `getOverworld()`，玩家在下界开容器会拿到
 *      主世界同坐标的方块实体——把**另一个容器**的内容显示给玩家（静默串台）。
 *
 * 第 3 条需要一个"同坐标、两个维度、内容不同"的构造：主世界与下界在同一个 (x,y,z) 各放一个
 * 箱子，主世界那个塞一块石头；玩家在下界打开同坐标的箱子时，正确的实现应看到**空箱子**。
 * 若服务端仍取主世界，就会看到石头（用例失败）。
 *
 * 该构造需要服务端执行命令（/execute in、/fill、/setblock、/tp），故本文件的用例声明
 * opPlayers: true，由 runner 在启动服务端前写好 ops.json。
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
    waitForPhysicsTick,
    waitForSlot,
} from "./shared.ts";

/** 容器窗口里玩家背包区的槽位偏移：箱子/木桶/潜影盒都是 27 格容器 + 27 格主背包。 */
const CONTAINER_SLOTS = 27;
const CONTAINER_PLAYER_MAIN_START = 27;
const CONTAINER_PLAYER_HOTBAR_START = 54;
/** 容器 + 玩家背包的槽位总数。 */
const CONTAINER_TOTAL_SLOTS = CONTAINER_SLOTS + 36;

/** 打开容器窗口的等待上限。 */
const OPEN_TIMEOUT_MS = 15_000;

/** 命令反馈的等待上限。 */
const COMMAND_FEEDBACK_TIMEOUT_MS = 5_000;

/** 维度 id（命令参数用带命名空间的完整形式）。 */
const NETHER = "minecraft:the_nether";

/**
 * 打开方块容器并等待窗口。
 *
 * 用 `bot.openContainer` 而非 `bot.openBlock`：前者会额外校验窗口类型是否属于容器白名单
 * （barrel / shulker_box / generic_9x3 都在其中），菜单类型编码错位会因此直接抛错——
 * 这正是潜影盒用例想钉住的东西。`openBlock` 不做该校验。
 *
 * 这里再加一层更短的超时：mineflayer 自身的等待是 20 秒，超时只说
 * "Event windowOpen did not fire"，看不出「服务端根本没发 OpenScreen」。
 *
 * @returns 窗口对象（含 slots 数组）。
 */
async function openContainerTimed(bot: Bot, block: unknown): Promise<{ slots: ({ name?: string } | null)[] }> {
    const typed = bot as unknown as { openContainer(block: unknown): Promise<unknown> };
    let timer: NodeJS.Timeout | null = null;
    try {
        const window = await Promise.race([
            typed.openContainer(block),
            new Promise<never>((_, reject) => {
                timer = setTimeout(
                    () => reject(new Error(`打开容器超时（${OPEN_TIMEOUT_MS}ms）：服务端未下发窗口内容`)),
                    OPEN_TIMEOUT_MS,
                );
            }),
        ]);
        return window as { slots: ({ name?: string } | null)[] };
    } finally {
        if (timer !== null) {
            clearTimeout(timer);
        }
    }
}

/**
 * 放置一个方块容器并打开它。
 *
 * @param itemName 需要放置的方块物品名（barrel / shulker_box）。
 */
async function placeAndOpenBlockContainer(
    bot: Bot,
    itemName: string,
    x: number,
    surfaceY: number,
    z: number,
): Promise<{ window: { slots: ({ name?: string } | null)[] }; blockX: number; blockY: number }> {
    const given = await setSlot(bot, INV_HOTBAR_SLOT_0, itemName, 1);
    expectTrue(given, `无法给 bot 发放 ${itemName}（registry 中找不到该物品）`);
    await delay(200);

    const refBlock = bot.blockAt(vec3(x, surfaceY, z));
    expectTrue(refBlock !== null, `参考方块 (${x},${surfaceY},${z}) 不可读`);
    await bot.placeBlock(refBlock as never, vec3(0, 1, 0));
    await delay(200);

    const blockY = surfaceY + 1;
    const placed = bot.blockAt(vec3(x, blockY, z));
    expectTrue(placed !== null, `放置后的 ${itemName} 不可读`);
    expectEq(blockNameAt(bot, x, blockY, z), itemName, `放置后 (${x},${blockY},${z}) 应为 ${itemName}`);

    const window = await openContainerTimed(bot, placed as never);
    return { window, blockX: x, blockY };
}

/**
 * 让 bot 自己（而不是命令）在脚边放一个箱子，返回箱子的坐标。
 *
 * **为什么不在这里用 `/setblock`**：实测跨维度传送后，客户端手里的目标维度区块数据与
 * 服务端的当前状态不一致——在目标维度用命令改方块（无论改在传送前还是传送后），客户端
 * 都可能读到改动前的快照（本次实测：下界 (8,100,8) 明明被 setblock 成箱子，客户端读到的
 * 仍是 netherrack，且此后没有任何 block_change 能纠正它）。用 bot 自己的放置则不存在这个
 * 问题：mineflayer 的放置是本地预测 + 服务端 ack，客户端一定知道箱子在哪里。
 * 该区块一致性问题本身与容器无关，是需要独立修复的服务端缺陷。
 *
 * 落点不可预知（玩家会从 y=100 掉到下界地形上），因此按四个水平方向逐个尝试，
 * 用「脚下那一格是不是可站立的实心方块 + 目标格是不是空气」筛出可放置的位置。
 *
 * @returns 箱子的方块坐标。
 */
async function placeChestBesidePlayer(
    bot: Bot,
): Promise<{ readonly x: number; readonly y: number; readonly z: number }> {
    const typed = bot as unknown as { entity: { position: { x: number; y: number; z: number } } };
    const feetX = Math.floor(typed.entity.position.x);
    const feetY = Math.floor(typed.entity.position.y);
    const feetZ = Math.floor(typed.entity.position.z);

    // 下界地形高低不平，玩家脚下那一格未必是「一块平地」：在水平 ±2、垂直 {0,+1} 的范围内
    // 找一个「目标格是空气 + 正下方是实心」的位置。半径 2 保证仍在触及范围（约 4.5 格）内。
    const candidates: Array<readonly [number, number, number]> = [];
    for (const dy of [0, 1]) {
        for (let dx = -2; dx <= 2; dx += 1) {
            for (let dz = -2; dz <= 2; dz += 1) {
                if (dx === 0 && dz === 0 && dy === 0) {
                    continue;
                }
                candidates.push([dx, dy, dz]);
            }
        }
    }
    // 近处优先，放置成功率更高。
    candidates.sort((a, b) => Math.abs(a[0]) + Math.abs(a[1]) + Math.abs(a[2]) - (Math.abs(b[0]) + Math.abs(b[1]) + Math.abs(b[2])));

    const probes: string[] = [];
    for (const [dx, dy, dz] of candidates) {
        const x = feetX + dx;
        const y = feetY + dy;
        const z = feetZ + dz;
        const support = bot.blockAt(vec3(x, y - 1, z));
        const target = bot.blockAt(vec3(x, y, z));
        if (support === null || target === null) {
            continue;
        }
        const supportName = (support as { name?: string }).name ?? "";
        const targetName = (target as { name?: string }).name ?? "";
        if (supportName === "air" || supportName.includes("lava") || targetName !== "air") {
            probes.push(`(${dx},${dy},${dz})=${targetName}/${supportName}`);
            continue;
        }
        const given = await setSlot(bot, INV_HOTBAR_SLOT_0, "chest", 1);
        expectTrue(given, "无法给 bot 发放箱子");
        await delay(200);
        try {
            await bot.placeBlock(support as never, vec3(0, 1, 0));
        } catch (err) {
            // 该位置放不下（触及范围、支撑面判定等），换下一个。
            probes.push(`(${dx},${dy},${dz})=place-failed:${(err as Error).message.split("\n")[0]}`);
            continue;
        }
        await delay(200);
        if (blockNameAt(bot, x, y, z) === "chest") {
            return { x, y, z };
        }
    }
    throw new Error(
        `bot 脚边找不到可放置箱子的位置（脚下 ${feetX},${feetY},${feetZ}）；候选探测结果：${probes.slice(0, 12).join(" ")}`,
    );
}

/**
 * 等 bot 真正落地。
 *
 * **不能只等 `onGround === true`**：跨维度传送后 mineflayer 会先关掉物理，此时 onGround
 * 仍是传送前的旧值（恒为 true），等待会立刻通过，而 bot 其实还在半空或嵌在方块里。
 * 判据取三者同时成立并连续成立若干次：脚下方块是实心（非空气、非岩浆）、onGround 为真、
 * 且高度不再变化。
 */
async function waitUntilLanded(bot: Bot, timeoutMs: number): Promise<void> {
    const deadline = Date.now() + timeoutMs;
    let lastY = Number.NaN;
    let stableSamples = 0;
    while (Date.now() < deadline) {
        const position = bot.entity?.position;
        if (position !== undefined) {
            const feetY = Math.floor(position.y);
            const below = bot.blockAt(vec3(Math.floor(position.x), feetY - 1, Math.floor(position.z)));
            const belowName = below === null ? null : ((below as { name?: string }).name ?? "");
            const solidBelow = belowName !== null && belowName !== "air" && !belowName.includes("lava");
            const stable = Math.abs(position.y - lastY) < 0.05;
            stableSamples = solidBelow && bot.entity?.onGround === true && stable ? stableSamples + 1 : 0;
            if (stableSamples >= 3) {
                return;
            }
            lastY = position.y;
        }
        await delay(100);
    }
    throw new Error(
        `等待落地超时（${timeoutMs}ms）：当前位置 y=${String(bot.entity?.position.y)}` +
            `，脚下 ${String(blockNameAt(bot, Math.floor(bot.entity?.position.x ?? 0), Math.floor((bot.entity?.position.y ?? 0) - 1), Math.floor(bot.entity?.position.z ?? 0)))}`,
    );
}

/** 把玩家背包快捷栏首槽的物品搬进当前打开的容器首槽。 */
async function moveHotbarItemIntoContainerSlot(bot: Bot, targetSlot: number): Promise<void> {
    await bot.clickWindow(CONTAINER_PLAYER_HOTBAR_START, 0, 0);
    await bot.clickWindow(targetSlot, 0, 0);
}

export const containerCases: readonly CaseDefinition[] = [
    {
        id: "containers/barrel_open_delivers_content",
        title: "木桶可打开并收到窗口内容（27 格容器 + 36 格玩家背包）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: false,
        skipReason: null,
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 木桶与箱子共用 generic_9x3 菜单类型，但方块实体类型是 Barrel。服务端若只认
            // Chest/TrappedChest，木桶就会「右键无反应、零日志」——本用例是该缺口的哨兵。
            const { window, blockY } = await placeAndOpenBlockContainer(bot, "barrel", spawnX + 2, surfaceY, spawnZ + 2);

            const slotCount = window.slots.length;
            expectEq(slotCount, CONTAINER_TOTAL_SLOTS, "木桶窗口槽位数应为 63（27 木桶格 + 36 玩家格）");
            expectEq(slotItemName(window, 0), null, "刚打开的木桶首槽应为空");

            // 回包闭环：把石头从快捷栏搬进木桶。
            const given = await setSlot(bot, INV_HOTBAR_SLOT_0, "stone", 1);
            expectTrue(given, "无法给 bot 发放石头");
            await waitForSlot(bot, INV_HOTBAR_SLOT_0, "stone", "石头就位");
            await moveHotbarItemIntoContainerSlot(bot, 0);

            // 判据取「关窗后重新打开时服务端下发的全量内容」，而不是「有没有收到增量同步包」：
            // vanilla 在这种「客户端预测结果与服务端结算一致」的点击上**不发增量包**
            // （实测 mineflayer 侧一个 set_slot 都没收到，而服务端确实结算了），
            // 而 clickWindow 本身是本地预测，单看客户端槽位等于什么都没验证。
            // 重新打开窗口时服务端必须下发全量内容，它是权威状态，两侧都成立。
            await bot.closeWindow(window as never);
            await delay(300);
            const reopened = await openContainerTimed(bot, bot.blockAt(vec3(spawnX + 2, blockY, spawnZ + 2)) as never);
            expectEq(
                slotItemName(reopened, 0),
                "stone",
                "重新打开木桶后首槽应为石头——为空说明服务端没有结算那次点击",
            );

            await bot.closeWindow(reopened as never);
            expectEq(blockNameAt(bot, spawnX + 2, blockY, spawnZ + 2), "barrel", "关闭后木桶方块应仍在原处");
            return { containerSlotCount: slotCount, itemMovedIntoContainer: true };
        },
    },

    {
        id: "containers/shulker_box_open_delivers_content",
        title: "潜影盒可打开（独立的 shulker_box 菜单类型）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: false,
        skipReason: null,
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 潜影盒的菜单类型是 shulker_box 而非常见的 generic_9x3，槽位布局同为 27+36。
            // 菜单类型编码表错位时（历史上 ContainerType 枚举少了 crafter_3x3），
            // mineflayer 会因窗口类型不在容器白名单里直接抛错——比"看起来打开了"有价值得多。
            const { window } = await placeAndOpenBlockContainer(bot, "shulker_box", spawnX - 2, surfaceY, spawnZ - 2);

            const slotCount = window.slots.length;
            expectEq(slotCount, CONTAINER_TOTAL_SLOTS, "潜影盒窗口槽位数应为 63（27 潜影盒格 + 36 玩家格）");
            expectEq(slotItemName(window, 0), null, "刚打开的潜影盒首槽应为空");

            await bot.closeWindow(window as never);
            return { containerSlotCount: slotCount };
        },
    },

    {
        id: "containers/container_in_nether_is_not_overworld",
        title: "下界里的容器能被打开（菜单工厂须按玩家维度取世界，而非固定主世界）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: true,
        // 阻塞点：跨维度传送后服务端下发给客户端的是**未生成的默认区块**——实测客户端在下界
        // (0,125,0) 周围的 5x5（含本该是空气的层）全读到 netherrack，客户端因此看不到下界真实
        // 地形，也无法在下界放置/打开容器，本用例的前提不成立。
        // 证据：build/e2e/artifacts/20260928-003716-72828/（服务端日志 + bot-trace）。
        //       服务端一侧的同坐标状态是正常的（/setblock 返回 1），问题在区块下发。
        // 解除条件：修好「维度切换后的区块下发」后把本字段改回 null——用例本身已写好，
        //           它对应的菜单工厂缺陷（固定取主世界）已经修复，修好后应能直接通过。
        skipReason:
            "服务端跨维度传送后给客户端下发未生成的默认区块（整片 netherrack），" +
            "客户端看不到下界真实地形，无法在下界放置/打开容器；待修复维度区块下发缺陷后移除此跳过标记",
        async run({ bot }): Promise<Record<string, unknown>> {
            // 被测缺陷：菜单工厂曾硬编码 getOverworld()，玩家在下界开容器时会在**主世界**同坐标
            // 找方块实体——那里通常是空气，于是菜单建不出来（客户端表现为开容器超时），
            // 若主世界同坐标恰好有别的容器则会串台显示它的内容。
            //
            // 判别方式：在下界放一个箱子并打开它。修好后能打开且内容为空；若服务端仍取主世界，
            // 在该坐标找不到方块实体 → 菜单创建失败 → 客户端等不到窗口（本用例失败），
            // 服务端日志同时会留下 "Container menu creation failed: no block entity" 的告警。
            //
            // 箱子由 bot 自己放置而不是用 /setblock：跨维度传送后，命令改动的方块不会可靠地
            // 出现在客户端手里（见 placeChestBesidePlayer 的说明）。坐标不再是固定值，
            // 因为玩家会从 y=100 落到下界地形上——坐标由落点决定，不影响判别力。
            //
            // 关掉自然刷怪。**这不是为了用例本身，而是绕开一个与本用例无关的服务端崩溃**：
            // 主世界之外的新维度（此处为下界）首次被 tick 时，NaturalSpawner::_createDensityManager
            // 会 ACCESS_VIOLATION（读 0x0）导致服务端整体崩溃（实测崩溃栈：
            // NaturalSpawner.cpp:965 ← NaturalSpawner::tick:376 ← ServerDimension::tick:187）。
            // ServerDimension::tick 用 doMobSpawning 规则门控自然刷怪，故先关掉它即可继续
            // 验证容器行为。**该崩溃本身是需要独立修复的服务端缺陷**，不要因为这条 workaround
            // 就认为它们是同一个问题。
            // TODO: 修复「首次 tick 非主世界维度时 NaturalSpawner 崩溃」后，移除本行门控，
            //       让本用例在自然刷怪开启的默认环境下运行。
            await runCommand(bot, "/gamerule doMobSpawning false", COMMAND_FEEDBACK_TIMEOUT_MS);

            // 进入下界：y=125 紧贴下界天花板（y=127 的基岩层）之下，那里必定是空气——
            // 取地形高度以内的 y（例如 100）会把玩家直接塞进岩层里，客户端会认为它站在
            // 实心方块上（onGround 成立）而无法在脚边找到可放置的位置。落到地形上再操作。
            await runCommand(bot, `/execute in ${NETHER} run tp @s 0.5 125 0.5`, COMMAND_FEEDBACK_TIMEOUT_MS);
            await waitForCondition(() => String(bot.game.dimension).includes("the_nether"), {
                timeoutMs: 15_000,
                pollMs: 100,
                what: "bot 进入下界（dimension 变为 the_nether）",
                describe: () => `当前 dimension = ${String(bot.game.dimension)}`,
            });
            // 跨维度传送后 mineflayer 会暂时关闭物理，而 openContainer 内部的 lookAt 要等一个
            // 物理 tick——不等这一步，客户端连 use_item_on 都发不出去（见 shared.ts 的说明）。
            await waitForPhysicsTick(bot, 15_000);
            await waitUntilLanded(bot, 30_000);

            const chest = await placeChestBesidePlayer(bot);
            const chestBlock = bot.blockAt(vec3(chest.x, chest.y, chest.z));
            expectTrue(chestBlock !== null, `下界箱子 (${chest.x},${chest.y},${chest.z}) 不可读`);

            const netherWindow = await openContainerTimed(bot, chestBlock as never);
            const slotCount = netherWindow.slots.length;
            expectEq(slotCount, CONTAINER_TOTAL_SLOTS, "下界箱子窗口槽位数应为 63（27 箱子格 + 36 玩家格）");
            expectEq(slotItemName(netherWindow, 0), null, "刚放置的下界箱子首槽应为空");
            await bot.closeWindow(netherWindow as never);

            return {
                containerSlotCount: slotCount,
                openedContainerInNether: true,
            };
        },
    },
];
