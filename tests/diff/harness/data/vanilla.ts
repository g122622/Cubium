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
 * vanilla.ts — 参照实现（oracle）之一：minecraft-data 的方块表。
 *
 * minecraft-data 是 PrismarineJS 维护的、由官方数据包生成的对照数据，已经随
 * tests/e2e/bot 的依赖装在仓库里，离线可用，不需要 JDK 与服务端 jar。
 *
 * 它**不是**最终形态：它只给出属性与状态数，没有状态级 ID 的完整枚举，
 * 也没有 Java 侧的解析语义。等 text_encode 打通后，应改为链接 1.21.11 服务端
 * jar 的 Java oracle（放在 tools/diff-java/ 下，参照 mods/horizon 的做法），
 * 那才是逐字节可比的权威源。
 *
 * 放在这一版里的理由：让差分链路先跑通，验证 probe、编排层与比对语义正确，
 * 再换参照实现——换的时候只改本文件，probe 与测试脚本不动。
 */

import { createRequire } from "node:module";
import path from "node:path";
import { fileURLToPath } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));

/** 参照实现对齐的游戏版本。 */
export const VANILLA_VERSION = "1.21.11";

/** 仓库根（tests/diff/harness → tests/diff → tests → 根）。 */
export const REPO_ROOT = path.resolve(HERE, "../../../..");

/** minecraft-data 的加载器（复用 e2e bot 已安装的依赖，避免维护两份同版本数据）。 */
const E2E_MODULE_DIR = path.join(REPO_ROOT, "tests/e2e/bot");
const requireFromE2E = createRequire(path.join(E2E_MODULE_DIR, "package.json"));

interface MinecraftDataModule {
    (version: string): {
        blocksArray: RawBlock[];
        itemsArray: unknown[];
        entitiesArray: unknown[];
        enchantmentsArray: unknown[];
        foodsArray: unknown[];
    };
}

let cachedModule: MinecraftDataModule | undefined;

/** 从 e2e bot 的 node_modules 加载 minecraft-data（CommonJS 包，故走 createRequire）。 */
export function loadMinecraftData(): MinecraftDataModule {
    if (cachedModule === undefined) {
        try {
            cachedModule = requireFromE2E("minecraft-data") as MinecraftDataModule;
        } catch {
            throw new Error(
                `无法从 ${E2E_MODULE_DIR} 加载 minecraft-data。请先在 ${E2E_MODULE_DIR} 下执行 npm install。`,
            );
        }
    }
    return cachedModule;
}

export interface VanillaProperty {
    readonly name: string;
    /** 对应 IProperty::typeName()：EnumProperty / BooleanProperty / IntegerProperty。 */
    readonly type: string;
    /** 允许取值；为 null 表示参照数据没给出值域。 */
    readonly values: readonly string[] | null;
}

export interface VanillaBlock {
    readonly name: string;
    readonly stateCount: number;
    readonly properties: readonly VanillaProperty[];
}

/**
 * minecraft-data 的属性类型名 → IProperty::typeName() 的返回名。
 *
 * 只映射两侧都有明确对应物的类型。遇到未知类型直接抛错而不是猜——
 * 宁可让升级 minecraft-data 时暴露出来，也不要静默放过半个属性。
 * 1.21.11 实测只有 enum / bool / int 三种（845 / 1000 / 139 个属性实例）。
 */
const PROPERTY_TYPE_BY_MC_DATA: Readonly<Record<string, string>> = {
    enum: "EnumProperty",
    bool: "BooleanProperty",
    int: "IntegerProperty",
    integer: "IntegerProperty",
};

interface RawProperty {
    name: string;
    type?: string;
    num_values?: number;
    values?: Array<string | number | boolean>;
}

interface RawBlock {
    name: string;
    minStateId?: number;
    maxStateId?: number;
    states?: RawProperty[];
}

/** 把 minecraft-data 的单个属性转成与 probe 同构的摘要。 */
function toProperty(raw: RawProperty): VanillaProperty {
    const rawType = raw.type ?? "enum";
    const type = PROPERTY_TYPE_BY_MC_DATA[rawType];
    if (type === undefined) {
        throw new Error(
            `minecraft-data 出现未知属性类型 ${JSON.stringify(rawType)}（属性 ${JSON.stringify(raw.name)}）；` +
                "请在 vanilla.ts 的 PROPERTY_TYPE_BY_MC_DATA 中补上对应关系",
        );
    }
    // minecraft-data 对布尔属性不列 values，vanilla 的取值顺序恒为 false, true。
    if (rawType === "bool") {
        return { name: raw.name, type, values: ["false", "true"] };
    }
    if (raw.values === undefined) {
        return { name: raw.name, type, values: null };
    }
    return { name: raw.name, type, values: raw.values.map(String) };
}

/**
 * 读取方块表，返回按名字排序的摘要。
 *
 * 数据来自 e2e bot 已安装的 minecraft-data，只维护一处，避免两份数据各自漂移。
 */
export function loadBlocks(): VanillaBlock[] {
    const data = loadMinecraftData()(VANILLA_VERSION);
    const blocks: VanillaBlock[] = [];
    for (const raw of data.blocksArray) {
        if (raw.minStateId === undefined || raw.maxStateId === undefined) {
            continue;
        }
        blocks.push({
            name: `minecraft:${raw.name}`,
            stateCount: raw.maxStateId - raw.minStateId + 1,
            properties: (raw.states ?? []).map(toProperty),
        });
    }
    blocks.sort((lhs, rhs) => lhs.name.localeCompare(rhs.name));
    return blocks;
}

/**
 * 方块摘要，与 probe `run` 的输出同构。
 *
 * 属性按名字排序：vanilla 的 `StateHolder#toString` 输出、以及 probe 的编码都按
 * 名字字典序，比较的是这个语义顺序，不是任何一方注册时的插入顺序。
 * 取值也排序——minecraft-data 给的枚举顺序与 vanilla 的 `Property#getPossibleValues`
 * 不一定一致，而本能力关心的是「值域相同」，不是「枚举顺序相同」。
 */
export function describe(block: VanillaBlock): Record<string, unknown> {
    const properties = [...block.properties]
        .sort((lhs, rhs) => (lhs.name < rhs.name ? -1 : lhs.name > rhs.name ? 1 : 0))
        .map((prop) => ({
            name: prop.name,
            type: prop.type,
            values: prop.values === null ? null : [...prop.values].sort(),
        }));
    return { block: block.name, stateCount: block.stateCount, properties };
}

/**
 * 展开方块的全部状态文本，与 probe `list` 的输出同构。
 *
 * 属性按名字排序后做笛卡尔积，结果整体排序：双方都排序，
 * 枚举顺序的差异就不会被误报成内容差异。
 */
export function stateTexts(block: VanillaBlock): string[] {
    const ordered = [...block.properties].sort((lhs, rhs) => (lhs.name < rhs.name ? -1 : lhs.name > rhs.name ? 1 : 0));
    if (ordered.length === 0) {
        return [block.name];
    }

    let combinations: Array<Array<[string, string]>> = [[]];
    for (const prop of ordered) {
        if (prop.values === null) {
            throw new Error(`${block.name}: 属性 ${prop.name} 缺值列表，无法展开状态全集`);
        }
        const expanded: Array<Array<[string, string]>> = [];
        for (const prefix of combinations) {
            for (const value of prop.values) {
                expanded.push([...prefix, [prop.name, value]]);
            }
        }
        combinations = expanded;
    }

    return combinations
        .map((combination) => `${block.name}[${combination.map(([name, value]) => `${name}=${value}`).join(",")}]`)
        .sort();
}