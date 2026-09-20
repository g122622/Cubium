/*
 * furnace.ts — 场景 F：熔炉。
 *
 * 熔炉是「方块实体 + 持续 tick 的容器」：它比箱子多两样东西，两样都只能靠真客户端往返验证。
 *
 *   1. **进度同步**：燃烧剩余时间与熔炼进度经 container_set_data 下推。不下推时服务端照常烧炼，
 *      客户端只是永远显示进度条为 0——纯表现层故障，不看包根本发现不了。
 *   2. **方块状态翻转**：点火后 LIT 属性要写回世界（亮度和贴图依此变化）。写不回时熔炉在
 *      客户端看起来始终是熄灭的。
 *
 * 材料用圆石（烧成石头）与煤炭（燃料），两者都是原版配方且本项目已注册。
 */

import type { Bot } from "mineflayer";
import type { CaseDefinition } from "../case.ts";
import { expectEq, expectTrue } from "../assert/expect.ts";
import { vec3 } from "../assert/surface.ts";
import { delay, waitForCondition } from "../bot/wait.ts";

/** 玩家背包窗口（containerId=0）的快捷栏首槽。 */
const INV_HOTBAR_SLOT_0 = 36;

/**
 * 熔炉窗口的槽位号，即 1.21.11 FurnaceMenu 布局：
 * 0=输入, 1=燃料, 2=产物, 3-29=主背包, 30-38=快捷栏。
 */
const FURNACE_INPUT_SLOT = 0;
const FURNACE_FUEL_SLOT = 1;
const FURNACE_RESULT_SLOT = 2;
const FURNACE_HOTBAR_SLOT_0 = 30;
const FURNACE_TOTAL_SLOTS = 39;

/** 圆石烧成石头是 200 tick（10 秒）的原版配方，留出宽裕余量。 */
const SMELT_TIMEOUT_MS = 30_000;

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
 * 在指定坐标旁放一个熔炉并打开它。
 *
 * 用 `bot.openBlock` 而非 `bot.openContainer`：后者的窗口类型白名单里没有 furnace。
 */
async function placeAndOpenFurnace(
    bot: Bot,
    x: number,
    surfaceY: number,
    z: number,
): Promise<{ slots: ({ name?: string } | null)[] }> {
    const given = await setSlot(bot, INV_HOTBAR_SLOT_0, "furnace", 1);
    expectTrue(given, "无法给 bot 发放熔炉（registry 中找不到 furnace）");
    await delay(150);

    const refBlock = bot.blockAt(vec3(x, surfaceY, z));
    expectTrue(refBlock !== null, `参考方块 (${x},${surfaceY},${z}) 不可读`);
    await bot.placeBlock(refBlock as never, vec3(0, 1, 0));
    await delay(200);

    const furnaceBlock = bot.blockAt(vec3(x, surfaceY + 1, z));
    expectTrue(furnaceBlock !== null, "放置后的熔炉不可读");

    const typed = bot as unknown as { openBlock(block: unknown): Promise<unknown> };
    const window = await typed.openBlock(furnaceBlock);
    expectTrue(window !== undefined && window !== null, "打开熔炉失败");
    return window as { slots: ({ name?: string } | null)[] };
}

/** 读熔炉方块的 LIT 属性（方块未加载时返回 null）。 */
function furnaceLit(bot: Bot, x: number, y: number, z: number): boolean | null {
    const block = bot.blockAt(vec3(x, y, z));
    if (block === null) {
        return null;
    }
    const props = (block as unknown as { getProperties(): Record<string, unknown> }).getProperties();
    const lit = props["lit"];
    return typeof lit === "boolean" ? lit : null;
}

/**
 * 把玩家背包里的物品经光标搬进熔炉的某个槽位。
 *
 * 不能直接用创造模式改槽：`set_creative_mode_slot` 的目标始终是玩家自己的背包菜单
 * （原版如此），写不进已打开的方块容器。故走「拾取到光标 → 放进目标槽」。
 */
async function moveIntoFurnaceSlot(bot: Bot, sourceWindowSlot: number, targetWindowSlot: number): Promise<void> {
    await bot.clickWindow(sourceWindowSlot, 0, 0); // 整叠拾取到光标
    await bot.clickWindow(targetWindowSlot, 0, 0); // 放进目标槽
}

export const furnaceCases: readonly CaseDefinition[] = [
    {
        id: "furnace/open_delivers_content",
        title: "打开熔炉后收到窗口内容（39 槽且三格皆空）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 熔炉菜单的槽位布局是「输入 / 燃料 / 产物 + 36 格玩家背包」共 39 格。数量或排列
            // 不对时第三方客户端会把物品放进错误的格子里，且不报任何错。
            const window = await placeAndOpenFurnace(bot, spawnX + 3, surfaceY, spawnZ + 3);

            const slotCount = (window as unknown as { slots: unknown[] }).slots.length;
            expectEq(slotCount, FURNACE_TOTAL_SLOTS, "熔炉窗口槽位数应为 39（3 熔炉格 + 36 玩家格）");
            expectEq(slotItemName(window, FURNACE_INPUT_SLOT), null, "刚打开的熔炉输入槽应为空");
            expectEq(slotItemName(window, FURNACE_FUEL_SLOT), null, "刚打开的熔炉燃料槽应为空");
            expectEq(slotItemName(window, FURNACE_RESULT_SLOT), null, "刚打开的熔炉产物槽应为空");

            await bot.closeWindow(window as never);
            return { furnaceSlotCount: slotCount };
        },
    },

    {
        id: "furnace/smelts_input_to_result",
        title: "熔炉烧炼：进度数据下推 + 产物槽产出 + LIT 翻转",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, trace, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            const x = spawnX - 3;
            const z = spawnZ + 3;
            const window = await placeAndOpenFurnace(bot, x, surfaceY, z);

            // 材料放进第二、三个快捷栏槽位（熔炉窗口里快捷栏在最后 9 格）。
            const cobbleGiven = await setSlot(bot, INV_HOTBAR_SLOT_0 + 1, "cobblestone", 1);
            const coalGiven = await setSlot(bot, INV_HOTBAR_SLOT_0 + 2, "coal", 1);
            expectTrue(cobbleGiven && coalGiven, "无法给 bot 发放圆石/煤炭（registry 中找不到）");
            await delay(200);

            const cobbleWindowSlot = FURNACE_HOTBAR_SLOT_0 + 1;
            const coalWindowSlot = FURNACE_HOTBAR_SLOT_0 + 2;
            await moveIntoFurnaceSlot(bot, cobbleWindowSlot, FURNACE_INPUT_SLOT);
            await moveIntoFurnaceSlot(bot, coalWindowSlot, FURNACE_FUEL_SLOT);

            // 进度数据（燃烧剩余 / 熔炼进度）经 container_set_data 下推。Prismarine 生态沿用
            // 1.16 旧别名 craft_progress_bar 指代同一个包，两侧名字都算，不把别名当唯一真相。
            const progressBefore = trace.count("craft_progress_bar") + trace.count("container_set_data");

            // LIT 是方块状态属性，须在烧炼期间观察——它会在燃料耗尽后翻回 false，事后判断不了。
            let litObserved = false;
            const observeLit = (): void => {
                if (furnaceLit(bot, x, surfaceY + 1, z) === true) {
                    litObserved = true;
                }
            };

            await waitForCondition(
                () => {
                    observeLit();
                    return slotItemName(window, FURNACE_RESULT_SLOT) === "stone";
                },
                {
                    timeoutMs: SMELT_TIMEOUT_MS,
                    pollMs: 100,
                    what: "熔炉产出石头（圆石烧炼完成）",
                    describe: () => `产物槽当前为 ${String(slotItemName(window, FURNACE_RESULT_SLOT))}`,
                },
            );

            expectTrue(
                trace.count("craft_progress_bar") + trace.count("container_set_data") > progressBefore,
                "未收到任何熔炼进度数据（container_set_data）——服务端烧炼了但客户端看不到进度",
            );
            expectTrue(litObserved, "烧炼期间熔炉方块的 lit 属性始终为 false——点燃状态未写回世界");

            await bot.closeWindow(window as never);
            return { smeltedToStone: true, progressDataReceived: true, litFlipped: true };
        },
    },
];
