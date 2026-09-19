/*
 * inventory.ts — 场景 D：物品栏与容器交互。
 *
 * 覆盖「客户端改动物品栏 → 服务端处理 → 回包 → 客户端看到结果」的完整往返。这条链路
 * 此前完全没有自动化覆盖，而它是最容易静默失效的一段——服务端收下包但不回，客户端
 * 只会停在自己预测的状态上，不报错也不超时。
 *
 * 三条关键通路：
 *   1. 创造模式改槽（set_creative_mode_slot）→ 服务端回 container_set_content；
 *   2. 玩家背包屏的点击：客户端以固定的 containerId=0 上报，服务端须有常驻的背包菜单
 *      才能处理——否则这些点击全被「没有打开的容器」拒绝，物品栏 UI 完全不可交互；
 *   3. 打开方块容器：OpenScreen 只建出空窗口，内容要靠随后补发的 container_set_content，
 *      缺了它客户端会停在「窗口已开但内容未到」。
 */

import type { Bot } from "mineflayer";
import type { CaseDefinition } from "../case.ts";
import { expectEq, expectTrue } from "../assert/expect.ts";
import { vec3 } from "../assert/surface.ts";
import { delay, waitForCondition } from "../bot/wait.ts";

/** 玩家背包窗口里快捷栏首槽的槽位号（0-8 为合成/护甲区，36-44 才是快捷栏）。 */
const HOTBAR_SLOT_0 = 36;

/** 玩家背包窗口里主背包首槽的槽位号。 */
const MAIN_SLOT_0 = 9;

/**
 * 数一遍客户端收到的「容器同步」包。
 *
 * **这是本文件判断「服务端到底有没有处理这个上行包」的唯一可靠判据。**
 *
 * 为什么不能用客户端状态：mineflayer 在 `creative.setInventorySlot` 与 `clickWindow`
 * 上都会**立刻**把结果写进本地镜像、不等服务端回包（creative.js 的本地写入、
 * prismarine-windows `Window#acceptClick` 在发包前就改好了槽位与光标）。于是
 * 「等本地槽位变成某物品」这类断言，在服务端把包整个丢掉时照样通过——用例是绿的，
 * 却什么都没验证。
 *
 * 而服务端回包是它确实受理了的直接证据：本地预测伪造不出来。两条通路都算——
 * 原版只回变化槽位的 `set_slot`，Cubium 回整份物品栏的 `window_items`，这里不预设
 * 是哪种，只要求「有回包」。
 */
function containerSyncCount(trace: { count(name: string): number }): number {
    // 单槽更新在 Prismarine 生态里有新旧两种命名，两侧都算。
    return trace.count("window_items") + trace.count("set_slot") + trace.count("container_set_slot");
}

/** 从 bot 的 registry 里取物品定义。 */
function itemDef(bot: Bot, itemName: string): { id: number } | undefined {
    const typed = bot as unknown as { registry: { itemsByName: Record<string, { id: number }> } };
    return typed.registry.itemsByName[itemName];
}

/** 读某个背包槽位当前物品的名称（空格返回 null）。 */
function slotItemName(bot: Bot, slot: number): string | null {
    const slots = (bot.inventory as unknown as { slots: ({ name?: string } | null)[] }).slots;
    const item = slots[slot];
    return item === null || item === undefined ? null : (item.name ?? null);
}

/**
 * 经创造模式把某个物品放进指定背包槽位。
 *
 * 走 mineflayer 的 creative 插件（内部发 set_creative_mode_slot），因此这条路径同时
 * 检验了服务端对该上行包的处理与回包。
 */
async function setSlot(bot: Bot, slot: number, itemName: string, count: number): Promise<boolean> {
    const def = itemDef(bot, itemName);
    if (def === undefined) {
        return false;
    }
    const typed = bot as unknown as {
        creative: { setInventorySlot(slot: number, item: unknown): Promise<void> };
        version: string;
    };
    type ItemCtor = new (id: number, count: number) => unknown;
    const ItemModule = (await import("prismarine-item")).default as unknown as (version: string) => ItemCtor;
    const Item = ItemModule(typed.version);
    await typed.creative.setInventorySlot(slot, new Item(def.id, count));
    return true;
}

/** 等某个背包槽位变成指定物品。 */
async function waitForSlot(bot: Bot, slot: number, itemName: string, what: string): Promise<void> {
    await waitForCondition(() => slotItemName(bot, slot) === itemName, {
        timeoutMs: 10_000,
        pollMs: 50,
        what,
        describe: () => `槽位 ${slot} 当前为 ${String(slotItemName(bot, slot))}`,
    });
}

export const inventoryCases: readonly CaseDefinition[] = [
    {
        id: "inventory/display_name_resolves",
        title: "玩家列表条目的 displayName 可解析（player_info_update 的文本组件）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot }): Promise<Record<string, unknown>> {
            // player_info_update 的 displayName 是一个自定界的文本组件 NBT。写成空、写成
            // 不合法的 NBT、或该 action 位没置位，客户端都不会报错——它只会渲染不出名字，
            // 或直接回退到 profile 名。故必须显式断言解析结果。
            const players = bot.players as unknown as Record<string, { displayName?: { toString(): string } }>;
            const self = players[bot.username];
            expectTrue(self !== undefined, `玩家列表中没有自己 '${bot.username}'`);

            const displayName = self.displayName;
            expectTrue(displayName !== undefined && displayName !== null, "自身条目的 displayName 缺失");
            expectEq(displayName.toString(), bot.username, "displayName 的纯文本应等于用户名");
            return { displayNameResolved: true };
        },
    },

    {
        id: "inventory/creative_slot_write_visible",
        title: "创造模式改槽后客户端看到该物品（上行处理 + 回包闭环）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, trace }): Promise<Record<string, unknown>> {
            // set_creative_mode_slot 是上行包；服务端必须处理并把结果同步回来。
            // 判据取「服务端回了容器同步包」——不能只看客户端槽位，因为 mineflayer 在
            // setInventorySlot 上会本地预测、不等回包（见 containerSyncCount 的说明）。
            const before = containerSyncCount(trace);
            const placed = await setSlot(bot, HOTBAR_SLOT_0, "stone", 1);
            expectTrue(placed, "无法经创造模式写入槽位（registry 中找不到 stone）");

            await waitForCondition(() => containerSyncCount(trace) > before, {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "服务端受理创造改槽并回包同步",
                describe: () => `收到的容器同步包数未增加（仍为 ${before}）——上行包可能被丢弃`,
            });
            return { creativeSlotVisible: true, serverSyncedBack: true };
        },
    },

    {
        id: "inventory/held_slot_change_reflected",
        title: "改变手持槽内容后 bot.heldItem 随之变化",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot }): Promise<Record<string, unknown>> {
            // 手持槽（当前选中快捷栏槽）是物品栏里唯一被渲染到屏幕上的部分，客户端对它
            // 有一份独立的镜像。服务端改槽后若不把该槽同步回来，玩家的手持物会与实际不符。
            const typed = bot as unknown as { quickBarSlot: number };
            const selectedSlot = typed.quickBarSlot + HOTBAR_SLOT_0;

            const first = await setSlot(bot, selectedSlot, "stone", 1);
            expectTrue(first, "无法写入手持槽（registry 中找不到 stone）");
            await waitForSlot(bot, selectedSlot, "stone", "手持槽变为石头");
            expectEq(bot.heldItem?.name ?? null, "stone", "bot.heldItem 应反映石头");

            // 换成另一种物品，确认跟的是「当前内容」而不是首次赋值的残留。
            const second = await setSlot(bot, selectedSlot, "diamond", 1);
            expectTrue(second, "无法写入手持槽（registry 中找不到 diamond）");
            await waitForSlot(bot, selectedSlot, "diamond", "手持槽变为钻石");
            expectEq(bot.heldItem?.name ?? null, "diamond", "bot.heldItem 应反映钻石");
            return { heldItemTracksSlot: true };
        },
    },

    {
        id: "inventory/player_inventory_click_applies",
        title: "玩家背包屏的点击被服务端接受（containerId=0 的常驻菜单）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, trace }): Promise<Record<string, unknown>> {
            // 客户端背包屏的点击一律以固定的 containerId=0 上报。服务端若没有对应的常驻
            // 菜单，这些点击会被当成「没有打开的容器」直接拒绝——不报错、不回包，表现为
            // 物品栏 UI 完全没反应。移动一次物品即可证伪。
            const given = await setSlot(bot, HOTBAR_SLOT_0, "stone", 1);
            expectTrue(given, "无法给 bot 发放石头");
            await waitForSlot(bot, HOTBAR_SLOT_0, "stone", "石头已放入快捷栏首槽");

            const mainSlot = MAIN_SLOT_0;
            expectEq(slotItemName(bot, mainSlot), null, "目标主背包槽位应为空");

            // 左键拾取到光标，再左键放进主背包首个空槽——两次 clickWindow 都以 containerId=0 上行。
            //
            // 本用例存在的意义就是「服务端受理了 containerId=0 的点击」，而客户端状态不能
            // 作为判据：clickWindow 会本地预测（Window#acceptClick 在发包前就改好了本地镜像），
            // 服务端把点击丢掉时客户端照样显示物品已移动。故判据取服务端的回包。
            const before = containerSyncCount(trace);
            await bot.clickWindow(HOTBAR_SLOT_0, 0, 0);
            await bot.clickWindow(mainSlot, 0, 0);

            await waitForCondition(() => containerSyncCount(trace) > before, {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "服务端受理背包屏点击（containerId=0）并回包同步",
                describe: () => `收到的容器同步包数未增加（仍为 ${before}）——点击可能被拒绝`,
            });
            await waitForSlot(bot, mainSlot, "stone", "物品经背包屏点击移动到主背包");

            return {
                clickAccepted: true,
                clickPacketsSent: trace.count("window_click") + trace.count("container_click"),
            };
        },
    },

    {
        id: "inventory/container_open_delivers_content",
        title: "打开方块容器后收到窗口内容（OpenScreen 只是空壳）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 服务端在打开容器时先发 OpenScreen，再补一次全量内容。只有前者时客户端会停在
            // 「窗口已开、内容未到」——mineflayer 的 openContainer 正是等窗口内容才 resolve，
            // 缺了它这里会直接超时。
            const given = await setSlot(bot, HOTBAR_SLOT_0, "chest", 1);
            expectTrue(given, "无法给 bot 发放箱子");

            const x = spawnX + 3;
            const z = spawnZ + 3;
            const refBlock = bot.blockAt(vec3(x, surfaceY, z));
            expectTrue(refBlock !== null, `参考方块 (${x},${surfaceY},${z}) 不可读`);

            await bot.placeBlock(refBlock as never, vec3(0, 1, 0));
            await delay(200);

            const chestBlock = bot.blockAt(vec3(x, surfaceY + 1, z));
            expectTrue(chestBlock !== null, "放置后的箱子不可读");

            // openContainer 内部会等 windowOpen，而 windowOpen 要等窗口内容到达才触发。
            const chest = await bot.openContainer(chestBlock as never);
            expectTrue(chest !== undefined && chest !== null, "打开容器失败");

            // 箱子方块容器应有 27 个箱子槽 + 36 个玩家背包槽。
            const slotCount = (chest as unknown as { slots: unknown[] }).slots.length;
            expectTrue(slotCount >= 27 + 36, `容器槽位数不足：${slotCount}（期望至少 63）`);

            await bot.closeWindow(chest as never);
            return { containerContentDelivered: true, containerSlotCount: slotCount };
        },
    },

    {
        id: "inventory/container_close_returns_cursor_item",
        title: "关闭容器时服务端归还光标上的物品",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, trace, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 玩家可以把物品「拿在光标上」直接关掉容器。此时服务端必须把该物品塞回玩家
            // 物品栏——否则它既不在容器里也不在背包里，凭空消失。
            //
            // 槽位号在两种窗口下不是同一套：玩家背包窗口里快捷栏是 36-44，而在箱子窗口里
            // 要先排 27 个箱子槽、再排 27 个主背包槽，快捷栏落在最后 9 格。本用例内部统一
            // 用「当前打开窗口」的坐标，切换窗口时重新换算。
            const given = await setSlot(bot, HOTBAR_SLOT_0, "chest", 1);
            expectTrue(given, "无法给 bot 发放箱子");
            await delay(150);

            const x = spawnX - 3;
            const z = spawnZ - 3;
            const refBlock = bot.blockAt(vec3(x, surfaceY, z));
            expectTrue(refBlock !== null, "参考方块不可读");
            await bot.placeBlock(refBlock as never, vec3(0, 1, 0));
            await delay(200);

            const chestBlock = bot.blockAt(vec3(x, surfaceY + 1, z));
            expectTrue(chestBlock !== null, "放置后的箱子不可读");
            const chest = await bot.openContainer(chestBlock as never);

            // 往背包里放一块石头（此时箱子已打开，但它改的是玩家背包那一份数据）。
            const stoneGiven = await setSlot(bot, HOTBAR_SLOT_0, "stone", 1);
            expectTrue(stoneGiven, "无法发放石头");
            await waitForSlot(bot, HOTBAR_SLOT_0, "stone", "石头就位");

            // 箱子窗口里玩家快捷栏落在最后 9 格（前面是 27 个箱子槽 + 27 个主背包槽）。
            // 断言只读 bot.inventory：set_creative_slot 触发的同步以 containerId=0 下发，
            // 客户端只更新常驻的玩家背包窗口，箱子窗口那份镜像要等它自己收到全量才刷新。
            const chestSlots = (chest as unknown as { slots: unknown[] }).slots;
            const hotbarInChest = chestSlots.length - 9;

            // 拾取到光标，然后直接关窗口。服务端必须把光标上的物品塞回玩家物品栏。
            //
            // 不对「光标上有石头」做中间断言：光标由客户端本地预测维持（Window#acceptClick
            // 在发包前就设好了 selectedItem），断言它等于什么都没验证。改为验证两件服务端
            // 独有的事实——关窗后它回了同步包，且物品栏里确实出现了这块石头。
            const before = containerSyncCount(trace);
            await bot.clickWindow(hotbarInChest, 0, 0);
            await bot.closeWindow(chest as never);

            await waitForCondition(() => containerSyncCount(trace) > before, {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "服务端受理关窗并回包同步",
                describe: () => `收到的容器同步包数未增加（仍为 ${before}）`,
            });
            await waitForCondition(
                () => {
                    const slots = (bot.inventory as unknown as { slots: ({ name?: string } | null)[] }).slots;
                    return slots.some((item) => item?.name === "stone");
                },
                {
                    timeoutMs: 10_000,
                    pollMs: 50,
                    what: "关闭容器后石头回到玩家物品栏（服务端归还光标物品）",
                    describe: () => "物品栏里没有石头——关闭时光标物品被丢弃了",
                },
            );
            return { cursorItemReturned: true };
        },
    },
];
