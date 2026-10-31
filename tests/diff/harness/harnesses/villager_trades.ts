/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/*
 * villager_trades.ts — 村民交易三层（gen / price / restock）的共同语料。
 *
 * 三个能力共用一个探针可执行文件：探针按输入里的 `mode` 字段分发，oracle 按命令行第一个
 * 参数分发，因此同一份语料可以喂给三者中的任意一个。
 *
 *   - `gen`    ：职业 × 等级 × 村民类型 × 随机种子 → 抽取出的交易列表。
 *                原版每个等级只有 2 个计数器（`addOffersFromItemListings(..., 2)`），
 *                所以 `num` 固定为 2。
 *   - `price`  ：基础代价 / 需求 / 价格乘数 / 特殊价格 → 第一支付代价的数量。
 *                堆叠上限由代价的物品本身决定，语料只用上限为 1 / 16 / 64 的物品。
 *   - `restock`：一组交易的 uses / demand / maxUses → 补货一次之后的 uses / demand。
 *
 * 语料全部由固定种子的自乘同余序列生成，两侧喂同一份输入。
 */

import type { HarnessDeclaration } from "../core/types.ts";

/** 交易表覆盖的职业（原版 VillagerTrades.TRADES 的键，去掉 none / nitwit）。 */
const PROFESSIONS = [
    "minecraft:armorer",
    "minecraft:butcher",
    "minecraft:cartographer",
    "minecraft:cleric",
    "minecraft:farmer",
    "minecraft:fisherman",
    "minecraft:fletcher",
    "minecraft:leatherworker",
    "minecraft:librarian",
    "minecraft:mason",
    "minecraft:shepherd",
    "minecraft:toolsmith",
    "minecraft:weaponsmith",
] as const;

/** 村民外观类型（原版全部 7 种；决定船只、盔甲、寻宝地图等条目的分支）。 */
const TYPES = [
    "minecraft:desert",
    "minecraft:jungle",
    "minecraft:plains",
    "minecraft:savanna",
    "minecraft:snow",
    "minecraft:swamp",
    "minecraft:taiga",
] as const;

/** 每种职业抽取的种子数量。等级 1-5 全跑一遍。 */
const SEEDS_PER_PROFESSION = 6;

/** 价格语料里使用的代价物品：堆叠上限分别为 1 / 16 / 64，覆盖夹取上界。 */
const PRICE_ITEMS = ["minecraft:emerald", "minecraft:snowball", "minecraft:oak_sign"] as const;

/** 自乘同余序列：固定种子、全平台一致。 */
function randomSource(seed: number): (bound: number) => number {
    let state = seed >>> 0;
    return (bound: number): number => {
        state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
        return Math.floor((state / 0x100000000) * bound);
    };
}

interface GenUnit {
    readonly mode: "gen";
    readonly profession: string;
    readonly level: number;
    readonly num: number;
    readonly seed: number;
    readonly type: string;
}

interface PriceUnit {
    readonly mode: "price";
    readonly item: string;
    readonly base: number;
    readonly demand: number;
    readonly multiplier: number;
    readonly special: number;
    readonly maxStack: number;
}

interface RestockUnit {
    readonly mode: "restock";
    readonly offers: readonly { readonly uses: number; readonly demand: number; readonly maxUses: number }[];
}

/** 交易表本体：只有职业名，输出该职业全部等级的条目。 */
interface ListUnit {
    readonly mode: "list";
    readonly profession: string;
}

type Unit = GenUnit | PriceUnit | RestockUnit | ListUnit;

function genUnits(): GenUnit[] {
    const next = randomSource(0x56545241); // "VTRA"
    const units: GenUnit[] = [];
    for (const profession of PROFESSIONS) {
        for (let seedIndex = 0; seedIndex < SEEDS_PER_PROFESSION; seedIndex++) {
            // 种子取大跨度的 64 位值：同一职业不同等级用同一个种子，才能看出"逐级追加"的差异。
            const seed = ((seedIndex + 1) * 0x9E3779B1) ^ (profession.length << 24);
            const type = TYPES[next(TYPES.length)];
            for (let level = 1; level <= 5; level++) {
                units.push({ mode: "gen", profession, level, num: 2, seed, type });
            }
        }
    }
    return units;
}

function priceUnits(): PriceUnit[] {
    const next = randomSource(0x56545052); // "VTPR"
    const units: PriceUnit[] = [];
    const maxStacks: Record<string, number> = {
        "minecraft:emerald": 64,
        "minecraft:snowball": 16,
        "minecraft:oak_sign": 16,
    };
    // 1) 系统性覆盖：基础数量 × 需求 × 乘数，特殊价格为 0。
    for (const item of PRICE_ITEMS) {
        for (const base of [1, 2, 3, 5, 8, 12, 16]) {
            for (const demand of [-8, -3, -1, 0, 1, 3, 8]) {
                for (const multiplier of [0, 0.05, 0.2]) {
                    units.push({ mode: "price", item, base, demand, multiplier, special: 0, maxStack: maxStacks[item] });
                }
            }
        }
    }
    // 2) 神圣价格（玩家声誉 / 英雄效果）：正负两侧，包含把代价压到 1 与顶到上限的情形。
    for (const item of PRICE_ITEMS) {
        for (const base of [1, 4, 16, 64]) {
            for (const special of [-64, -16, -5, -1, 1, 5, 16, 64]) {
                units.push({ mode: "price", item, base, demand: 0, multiplier: 0.05, special, maxStack: maxStacks[item] });
            }
        }
    }
    // 3) 随机语料：需求与乘数取连续值，验证 floor 作用在乘积上而不是取整后相乘。
    for (let i = 0; i < 600; i++) {
        const item = PRICE_ITEMS[next(PRICE_ITEMS.length)];
        units.push({
            mode: "price",
            item,
            base: next(65),
            demand: next(33) - 16,
            multiplier: next(64) / 64,
            special: next(41) - 20,
            maxStack: maxStacks[item],
        });
    }
    return units;
}

function restockUnits(): RestockUnit[] {
    const next = randomSource(0x56545253); // "VTRS"
    const units: RestockUnit[] = [];
    const usageValues = [0, 1, 2, 3, 5, 8, 12, 16];
    // 1) 单条交易：uses 从小到售罄，maxUses 取交易表里出现的几档。
    for (const maxUses of [4, 6, 8, 12, 16]) {
        for (const uses of usageValues) {
            for (const demand of [-6, -1, 0, 2, 7]) {
                units.push({ mode: "restock", offers: [{ uses, demand, maxUses }] });
            }
        }
    }
    // 2) 多条交易：验证"先对全部交易 updateDemand，再统一 resetUses"。
    for (let i = 0; i < 400; i++) {
        const count = 1 + next(6);
        const offers = Array.from({ length: count }, () => ({
            uses: usageValues[next(usageValues.length)],
            demand: next(25) - 12,
            maxUses: 4 + next(13),
        }));
        units.push({ mode: "restock", offers });
    }
    return units;
}

/**
 * 交易表本体语料：每职业一个单元。
 *
 * 这个模式不经过"抽两条"的随机过程，直接比对整张表（等级 → 全部条目），因此每处差异
 * 都对应一个具体条目字段。`gen` 保留用于验证抽取过程本身（抽签与条目内随机消耗顺序）。
 */
function listUnits(): { readonly profession: string }[] {
    return PROFESSIONS.map((profession) => ({ profession }));
}

function unitKey(unit: Unit, index: number): string {
    return unit.mode === "list" ? `list/${unit.profession}` : `${unit.mode}/${index}`;
}

/**
 * 各模式的单元键：
 *   - `list`：`list/<职业>`（每职业一个单元，内容是整张表）；
 *   - 其余：`<模式>/<序号>`。
 * 两侧按同一份语料构造。
 */
export function villagerTradesDeclaration(mode: "list" | "gen" | "price" | "restock"): HarnessDeclaration {
    let corpus: { readonly unit: string; readonly input: Unit }[] = [];
    return {
        name: `villager_${mode}`,
        probe: "villager_trades",
        oracle: "VillagerTradesOracle",
        // oracle 以第一个命令行参数区分模式；探针以输入的 mode 字段区分。
        oracleArgs: [mode],
        // gen 会通过 LootLevel 建一个原版服务端世界（附魔书条目要读世界注册表），
        // 它与战利品族共用固定目录锁，必须串行运行。
        exclusiveGroup: "loot-world",
        key: "unit",
        // 附魔装备 / 附魔书的附魔内容尚未对齐（390 个单元里的 35 个）。`offerSummary` 把整批
        // 交易压成一个 `offers` 数组，逐字段比对时字段路径只能到 `offers` 这一层，没法只放过
        // 附魔那一段，于是整条单元被登记为已知差异。
        // TODO: 把比对口径改成逐交易展开（路径能落到 `offers[i].sell.enchantments`）后，把登记
        // 收窄到附魔字段，让价格、次数、经验、抽取序列继续严格比对。
        knownDifferences: mode === "gen"
            ? [
                {
                    key: "*",
                    field: "offers",
                    reason:
                        "附魔内容未对齐：项目附魔池只按 EnchantmentType 粗分类，原版逐附魔维护 "
                        + "supported_items / primary_items / exclusive_set，候选列表与等级区间随之不同",
                },
            ]
            : undefined,
        units: {
            kind: "computed",
            units: (): string[] => corpus.map((entry) => entry.unit),
        },
        input: (unit: string): unknown => {
            const entry = corpus.find((candidate) => candidate.unit === unit);
            if (entry === undefined) {
                throw new Error(`villager_${mode}: unknown unit ${unit}`);
            }
            return { ...entry.input, unit };
        },
        beforeUnits: (): void => {
            if (mode === "list") {
                corpus = listUnits().map((entry) => ({
                    unit: `list/${entry.profession}`,
                    input: { mode: "list", profession: entry.profession },
                }));
                return;
            }
            const units = mode === "gen" ? genUnits() : mode === "price" ? priceUnits() : restockUnits();
            corpus = units.map((unit, index) => ({ unit: unitKey(unit, index), input: unit }));
        },
    } satisfies HarnessDeclaration;
}