/*
 * crafting.ts — 场景 E：合成。
 *
 * 合成是「服务端算结果、客户端只显示」的典型链路：客户端把材料摆进网格，服务端匹配配方、
 * 把产物写进结果槽并同步回来。这条链路此前完全没有自动化覆盖，而它有几处容易静默失效：
 *
 *   1. 结果槽的产物依赖服务端推送（客户端不会自己算），服务端不推就是「摆了没反应」；
 *   2. 空网格必须不产出任何东西——曾有一条配料全部未注册的配方匹配到空网格，结果槽凭空
 *      出现产物（见下方 empty_grid_produces_nothing 用例）；
 *   3. 结果槽的槽位号在 wire 上是有约定的（工作台菜单里结果槽排在网格之前），错位会让
 *      第三方客户端的每一次点击都落到相邻槽位且不报错。
 *
 * 材料统一用橡木木板（4 个 → 1 个工作台），它是原版配方、两侧服务端都有。
 */

import type { Bot } from "mineflayer";
import type { CaseDefinition } from "../case.ts";
import { expectEq, expectTrue } from "../assert/expect.ts";
import { vec3 } from "../assert/surface.ts";
import { delay, waitForCondition } from "../bot/wait.ts";

/**
 * 玩家背包窗口（containerId=0）的槽位号，即 InventoryMenu 布局：
 * 0=合成结果, 1-4=2x2 合成格, 5-8=护甲, 9-35=主背包, 36-44=快捷栏, 45=副手。
 */
const INV_RESULT_SLOT = 0;
const INV_GRID_SLOT_START = 1;
const INV_HOTBAR_SLOT_0 = 36;

/**
 * 工作台窗口的槽位号，即 CraftingMenu 布局：
 * 0=合成结果, 1-9=3x3 合成格, 10-36=主背包, 37-45=快捷栏。
 *
 * 结果槽排在最前是 wire 约定的一部分：客户端点 slot 0 就是「取走合成产物」。
 */
const TABLE_RESULT_SLOT = 0;
const TABLE_GRID_SLOT_START = 1;
const TABLE_MAIN_SLOT_START = 10;
const TABLE_HOTBAR_SLOT_0 = 37;

/** 合成 4 个橡木木板所需的数量（原版 2x2 配方）。 */
const PLANKS_PER_CRAFT = 4;

/**
 * 数一遍客户端收到的「容器同步」包。
 *
 * 与 inventory.ts 同一判据，理由见那里的说明：mineflayer 在 clickWindow /
 * creative.setInventorySlot 上都会本地预测、不等服务端回包，只有服务端回包本身能证明
 * 「它确实处理了这个上行包」。合成结果的推送只可能来自服务端，故这里额外要求同步包数增加，
 * 排除「客户端自己预测出一个产物」的假通过。
 */
function containerSyncCount(trace: { count(name: string): number }): number {
    return trace.count("window_items") + trace.count("set_slot") + trace.count("container_set_slot");
}

/** 从 bot 的 registry 里取物品定义。 */
function itemDef(bot: Bot, itemName: string): { id: number } | undefined {
    const typed = bot as unknown as { registry: { itemsByName: Record<string, { id: number }> } };
    return typed.registry.itemsByName[itemName];
}

/** 读某个窗口槽位当前物品的名称（空格返回 null）。 */
function slotItemName(window: unknown, slot: number): string | null {
    const slots = (window as { slots: ({ name?: string } | null)[] }).slots;
    const item = slots[slot];
    return item === null || item === undefined ? null : (item.name ?? null);
}

/** 经创造模式把物品放进玩家背包的指定菜单槽位。 */
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

/**
 * 在指定坐标旁放一个工作台并打开它，返回窗口句柄。
 *
 * 用 `bot.openBlock` 而非 `bot.openContainer`：后者的窗口类型白名单里没有 crafting_table，
 * 会直接抛「containerToOpen is neither a block nor an entity」。openBlock 只等 windowOpen，
 * 而窗口内容由服务端补发的全量包填满。
 */
async function placeAndOpenCraftingTable(
    bot: Bot,
    x: number,
    surfaceY: number,
    z: number,
): Promise<{ slots: ({ name?: string } | null)[] }> {
    const given = await setSlot(bot, INV_HOTBAR_SLOT_0, "crafting_table", 1);
    expectTrue(given, "无法给 bot 发放工作台");
    await delay(150);

    const refBlock = bot.blockAt(vec3(x, surfaceY, z));
    expectTrue(refBlock !== null, `参考方块 (${x},${surfaceY},${z}) 不可读`);
    await bot.placeBlock(refBlock as never, vec3(0, 1, 0));
    await delay(200);

    const tableBlock = bot.blockAt(vec3(x, surfaceY + 1, z));
    expectTrue(tableBlock !== null, "放置后的工作台不可读");

    const typed = bot as unknown as { openBlock(block: unknown): Promise<unknown> };
    const window = await typed.openBlock(tableBlock);
    expectTrue(window !== undefined && window !== null, "打开工作台失败");
    return window as { slots: ({ name?: string } | null)[] };
}

/** 把快捷栏里的木板经光标逐个摆进工作台网格的 2x2 区域。 */
async function fillTableGridWithPlanks(bot: Bot, planksSlot: number): Promise<void> {
    // 先整叠拾取到光标，再用右键逐个放进网格。左键在空格上会放整叠，故必须用右键（button=1）。
    await bot.clickWindow(planksSlot, 0, 0);
    // 3x3 网格行优先排列，左上 2x2 区域即网格槽 0、1、3、4。
    for (const offset of [0, 1, 3, 4]) {
        await bot.clickWindow(TABLE_GRID_SLOT_START + offset, 1, 0);
    }
}

export const craftingCases: readonly CaseDefinition[] = [
    {
        id: "crafting/empty_grid_produces_nothing",
        title: "空合成网格不产出任何结果（结果槽恒空）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 回归锚点：曾有一条配料全部未注册的配方（其配料物品在项目里未实现，解析时被降级成
            // 空配料）匹配到完全空的网格，结果槽凭空出现产物并随全量同步下发到客户端。空网格
            // 永不产出结果，这里把它钉死。
            const window = await placeAndOpenCraftingTable(bot, spawnX + 3, surfaceY, spawnZ + 3);

            expectEq(
                slotItemName(window, TABLE_RESULT_SLOT),
                null,
                "刚打开的工作台结果槽应为空（空网格不产出任何东西）",
            );

            // 再等一段：若结果槽是被「稍后才推送的脏数据」填上的，只在打开瞬间断言会漏掉。
            await delay(600);
            expectEq(slotItemName(window, TABLE_RESULT_SLOT), null, "静置后工作台结果槽仍应为空");

            await bot.closeWindow(window as never);
            return { emptyGridResultIsNull: true };
        },
    },

    {
        id: "crafting/table_3x3_crafts_from_cursor",
        title: "工作台 3x3：光标摆放材料 → 结果槽出现产物 → 取走",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, trace, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            const window = await placeAndOpenCraftingTable(bot, spawnX - 3, surfaceY, spawnZ - 3);

            // 材料放进第二个快捷栏槽位（工作台窗口里快捷栏在最后 9 格）。
            const planksInvSlot = INV_HOTBAR_SLOT_0 + 1;
            const given = await setSlot(bot, planksInvSlot, "oak_planks", PLANKS_PER_CRAFT);
            expectTrue(given, "无法给 bot 发放木板");
            await delay(200);

            const planksWindowSlot = TABLE_HOTBAR_SLOT_0 + 1;

            // 判据取服务端回包：mineflayer 的 clickWindow 会本地预测槽位变化，只看客户端状态
            // 无法区分「服务端受理了」与「客户端自己演了一遍」。
            const before = containerSyncCount(trace);
            await fillTableGridWithPlanks(bot, planksWindowSlot);

            await waitForCondition(() => containerSyncCount(trace) > before, {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "服务端受理合成网格点击并回包同步",
                describe: () => `收到的容器同步包数未增加（仍为 ${before}）——点击可能被拒绝`,
            });

            await waitForCondition(() => slotItemName(window, TABLE_RESULT_SLOT) === "crafting_table", {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "工作台结果槽出现产物（服务端匹配到配方）",
                describe: () => `结果槽当前为 ${String(slotItemName(window, TABLE_RESULT_SLOT))}`,
            });

            // 取走产物：左键点结果槽，产物进光标，再把光标上的物品放进主背包空槽。
            await bot.clickWindow(TABLE_RESULT_SLOT, 0, 0);
            const emptyMainSlot = TABLE_MAIN_SLOT_START;
            await bot.clickWindow(emptyMainSlot, 0, 0);

            await waitForCondition(() => slotItemName(window, emptyMainSlot) === "crafting_table", {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "产物经光标落到主背包",
                describe: () => `主背包首槽当前为 ${String(slotItemName(window, emptyMainSlot))}`,
            });

            await bot.closeWindow(window as never);
            return { craftedFromCursor: true, resultSlotIndex: TABLE_RESULT_SLOT };
        },
    },

    {
        id: "crafting/table_shift_click_result_to_inventory",
        title: "工作台结果槽 shift+点击直接把产物送进玩家背包",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, trace, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            const window = await placeAndOpenCraftingTable(bot, spawnX + 3, surfaceY, spawnZ - 3);

            const planksInvSlot = INV_HOTBAR_SLOT_0 + 1;
            const given = await setSlot(bot, planksInvSlot, "oak_planks", PLANKS_PER_CRAFT);
            expectTrue(given, "无法给 bot 发放木板");
            await delay(200);

            await fillTableGridWithPlanks(bot, TABLE_HOTBAR_SLOT_0 + 1);
            await waitForCondition(() => slotItemName(window, TABLE_RESULT_SLOT) === "crafting_table", {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "工作台结果槽出现产物",
                describe: () => `结果槽当前为 ${String(slotItemName(window, TABLE_RESULT_SLOT))}`,
            });

            // mode=1 即 shift+点击（快速移动）。它会绕过光标，直接把产物塞进玩家背包。
            // 这条路径与「经光标搬运」是两套不同的服务端逻辑，须分别覆盖。
            const before = containerSyncCount(trace);
            await bot.clickWindow(TABLE_RESULT_SLOT, 0, 1);

            await waitForCondition(() => containerSyncCount(trace) > before, {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "服务端受理 shift+点击并回包同步",
                describe: () => `收到的容器同步包数未增加（仍为 ${before}）`,
            });

            // 产物应落在玩家背包区（主背包 10-36 或快捷栏 37-45），而不是留在结果槽或光标上。
            await waitForCondition(
                () => {
                    const range = [...Array(TABLE_HOTBAR_SLOT_0 + 9 - TABLE_MAIN_SLOT_START).keys()].map(
                        (i) => TABLE_MAIN_SLOT_START + i,
                    );
                    return range.some((slot) => slotItemName(window, slot) === "crafting_table");
                },
                {
                    timeoutMs: 10_000,
                    pollMs: 50,
                    what: "产物被送进玩家背包",
                    describe: () => "玩家背包区没有任何槽位拿到工作台——shift+点击未生效",
                },
            );

            await bot.closeWindow(window as never);
            return { shiftClickMovedResult: true };
        },
    },

    {
        id: "crafting/inventory_2x2_grid_crafts",
        title: "玩家背包 2x2 合成格产出结果并同步到客户端",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, trace }): Promise<Record<string, unknown>> {
            // 背包合成格在 wire 上是玩家背包窗口（containerId=0）的槽位 1-4。服务端若把它们
            // 当成「PlayerInventory 里没有的槽位」忽略掉，创造模式改槽写进去的物品会消失，
            // 合成格永远摆不上东西。
            const before = containerSyncCount(trace);
            for (let i = 0; i < PLANKS_PER_CRAFT; ++i) {
                const ok = await setSlot(bot, INV_GRID_SLOT_START + i, "oak_planks", 1);
                expectTrue(ok, "无法把木板写进背包合成格");
            }

            await waitForCondition(() => containerSyncCount(trace) > before, {
                timeoutMs: 10_000,
                pollMs: 50,
                what: "服务端受理背包合成格的改槽并回包同步",
                describe: () => `收到的容器同步包数未增加（仍为 ${before}）`,
            });

            await waitForCondition(
                () => slotItemName(bot.inventory, INV_RESULT_SLOT) === "crafting_table",
                {
                    timeoutMs: 10_000,
                    pollMs: 50,
                    what: "背包合成结果槽出现产物",
                    describe: () => `结果槽当前为 ${String(slotItemName(bot.inventory, INV_RESULT_SLOT))}`,
                },
            );

            return { inventoryGridCrafted: true };
        },
    },
];
