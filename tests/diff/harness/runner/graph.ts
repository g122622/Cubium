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
 * graph.ts — 能力依赖图：每个 harness 依赖哪些仓库文件、一次改动会影响哪些 harness。
 *
 * 依赖分三类，来源各不相同：
 *
 *   - **代码依赖**（files）：probe 的链接闭包 + oracle 的同包 import 闭包。来自构建图
 *     （见 tools/ninja.ts），在建图时计算一次并写进状态文件。
 *   - **运行期模块**（modules）：harness 实际运行一次后，子进程记录的 TS 模块。
 *   - **构建产物**（artifacts）：probe 可执行文件、oracle class 目录；缺失或被重建即过期。
 *
 * 依赖图是**过宽**的：C++ 侧按静态库边界展开，改一个 mc_common 的源文件会让所有链接它的
 * harness 一起重跑。这是有意的取舍——静态库边界是构建系统维护的契约，比用符号表反推
 * 「真正被引用的目标文件」稳；误触发只是慢几十秒，漏触发却会让缺陷逃过差分。
 *
 * 建图失败（找不到 ninja、probe 还没构建、依赖日志缺失）时抛 {@link GraphUnavailable}：
 * 调用方应把全部 harness 当作「必须运行」并给出原因，绝不把「没有依赖」当成「永远通过」。
 */

import fs from "node:fs";
import path from "node:path";

import type { FileHash, HashCache } from "../core/hash.ts";
import { digestOf, hashFiles } from "../core/hash.ts";
import { ORACLE_PACKAGE, ORACLE_SOURCE_DIR, SHARED_INPUTS } from "../core/types.ts";
import type { NinjaModel } from "../../tools/ninja.ts";
import { executableObjects, targetDependencies } from "../../tools/ninja.ts";
import type { ProjectLayout } from "./layout.ts";
import { isInside, probeArtifact, REPO_ROOT } from "./layout.ts";

/**
 * 第三方源码的判定：探针静态链接了它们（quickjs 经 mc_bedrock_addon、部分头文件被 CMake 复制
 * 进构建目录），但它们由 CMake 的 FetchContent 或 vcpkg 拉取，不由本仓库修改。计入仓库依赖
 * 只会让「重新配置 / 重新拉取依赖」误触发重跑，所以按路径排除。
 */

/** 构建图里无法解析时的原因。 */
export class GraphUnavailable extends Error {}

/** 一个 harness 的完整依赖条目（写进状态文件）。 */
export interface DependencyEntry {
    /** 代码依赖（相对仓库根，正斜杠）。 */
    readonly files: readonly string[];
    /** 构建产物（相对构建目录）；缺失或被重建即必须重跑。 */
    readonly artifacts: readonly string[];
    /** 最近一次真实运行记录的 TS 模块；还没运行过时为空。 */
    readonly modules: readonly string[];
}

/** 第三方源码路径（见文件头 SHARED_INPUTS 附近的说明）。 */
function isThirdParty(file: string): boolean {
    const normalized = file.split("\\").join("/");
    return normalized.includes("/third_party/") || normalized.includes("/_deps/") || normalized.includes("/vcpkg_installed/");
}

/** 把路径折算成仓库相对路径（正斜杠）；不在仓库内返回 undefined。 */
export function relativeToRepository(file: string): string | undefined {
    const absolute = path.resolve(file);
    const relative = path.relative(REPO_ROOT, absolute);
    if (relative.length === 0) {
        return "";
    }
    if (relative.startsWith("..") || path.isAbsolute(relative)) {
        return undefined;
    }
    return relative.split(path.sep).join("/");
}

/** oracle 源文件的同包 import 闭包（相对仓库根）。 */
export function oracleSources(oracle: string): string[] {
    const directory = path.join(REPO_ROOT, ...ORACLE_SOURCE_DIR.split("/"), ...ORACLE_PACKAGE.split("."));
    const seen = new Set<string>();
    const pending = [`${oracle}.java`];
    while (pending.length > 0) {
        const name = pending.pop() as string;
        if (seen.has(name)) {
            continue;
        }
        const file = path.join(directory, name);
        if (!fs.existsSync(file)) {
            throw new GraphUnavailable(`oracle 源文件不存在：${file}`);
        }
        seen.add(name);
        const text = fs.readFileSync(file, "utf8");
        for (const match of text.matchAll(/^\s*import\s+dev\.horizonlab\.diff\.([A-Za-z0-9_]+)\s*;/gm)) {
            pending.push(`${match[1]}.java`);
        }
        // 同包内不带 import 的直接引用：只认本目录真实存在的类名，避免把 JDK 的 Path/List
        // 之类当成同包类。
        for (const match of text.matchAll(/\b([A-Z][A-Za-z0-9_]*)\b/g)) {
            const candidate = `${match[1]}.java`;
            if (!seen.has(candidate) && fs.existsSync(path.join(directory, candidate))) {
                pending.push(candidate);
            }
        }
    }
    return [...seen]
        .sort()
        .map((name) => path.relative(REPO_ROOT, path.join(directory, name)).split(path.sep).join("/"));
}

/** 建图输入。 */
/** 纯数据能力共用的工厂类模块：它们与具体能力无关，计入代码依赖。 */
const SHARED_HARNESS_MODULES: readonly string[] = [
    "tests/diff/harness/harnesses/shared.ts",
    "tests/diff/harness/data/vanilla.ts",
];

export interface GraphInputs {
    readonly layout: ProjectLayout;
    readonly model: NinjaModel;
    /** 能力名 → 探针域名（纯数据 harness 也有一个同名的探针）。 */
    readonly probes: ReadonlyMap<string, string>;
    /** 能力名 → oracle 类名（纯数据 harness 没有）。 */
    readonly oracles: ReadonlyMap<string, string>;
    /** 全部探针的 `.obj` 与依赖闭包：探针之间共享大量目标文件，整体只展开一次。 */
    readonly objects?: ReadonlyMap<string, { objects: readonly string[]; missing: readonly string[] }>;
    /** 上次建出的条目：保留其运行期模块，避免重新建图时丢掉已学到的模块列表。 */
    readonly previous?: ReadonlyMap<string, DependencyEntry>;
}

/**
 * 为全部能力建立依赖条目。
 *
 * 「改了 mc_common 会重跑所有能力」是**刻意**的：C++ 探针都静态链接同一个 mc_common，
 * 链接闭包里的任何一个头文件变化都可能改变任一探针的行为，而用符号表反推「真的被引用了」
 * 既脆弱（模板、内联、静态初始化都会跨目标文件泄漏）又慢。反过来，探针的**源码**（例如
 * `probes/loot_table.cpp`）只能通过该探针的编译单元进入，所以按「每个探针自己的编译单元
 * （源文件 + 它包含的头文件）」精确隔离；共享库的源码按静态库整体计入。
 *
 * @throws GraphUnavailable 任何一个 probe 的链接闭包解析不出来时（整张图不可信）
 */
export function buildGraph(inputs: GraphInputs): Map<string, DependencyEntry> {
    const { layout, model, probes, oracles } = inputs;
    const entries = new Map<string, DependencyEntry>();
    // 同一个 .obj 会被多个可执行文件共用：它的依赖列表只整理一次。
    const normalized = new Map<string, string[]>();
    const fileSetOf = (object: string): string[] | undefined => {
        const cached = normalized.get(object);
        if (cached !== undefined) {
            return cached;
        }
        const deps = targetDependencies(model, object);
        if (deps === undefined) {
            return undefined;
        }
        const files: string[] = [];
        for (const dep of deps) {
            if (isInside(dep, layout.buildDir) || isThirdParty(dep)) {
                // 构建目录里的东西（CMake 生成的 pch、第三方源码）由构建系统维护，不由本仓库修改；
                // 把它们计入依赖只会让「重新配置」误触发重跑，而且它们的字节在不同机器上没有可比性。
                continue;
            }
            const relative = relativeToRepository(dep);
            if (relative !== undefined && relative.length > 0) {
                files.push(relative);
            }
        }
        normalized.set(object, files);
        return files;
    };
    const expansion = (probe: string): { objects: readonly string[]; missing: readonly string[] } => {
        const cached = inputs.objects?.get(probe);
        return cached ?? executableObjects(model, probeArtifact(probe, layout));
    };
    /** 探针自己的编译单元：`.../diff_<domain>.dir/<配置>/probes/`。路径起点随生成器变化，用包含判断。 */
    const probeSpecific = (object: string): string | undefined => {
        const match = /tests\/diff\/CMakeFiles\/diff_([A-Za-z0-9_]+)\.dir\/[^/]+\/probes\//.exec(object);
        return match?.[1];
    };
    const shared = new Set<string>(SHARED_INPUTS);
    for (const file of SHARED_HARNESS_MODULES) {
        shared.add(file);
    }
    for (const file of SHARED_INPUTS) {
        shared.add(file);
    }
    for (const [name, probe] of probes) {
        const expanded = expansion(probe);
        if (expanded.missing.length > 0) {
            throw new GraphUnavailable(`${probeArtifact(probe, layout)} 引用的静态库在构建图里缺失：${expanded.missing.join("、")}`);
        }
        const files = new Set<string>(shared);
        const oracle = oracles.get(name);
        if (oracle !== undefined) {
            for (const file of oracleSources(oracle)) {
                files.add(file);
            }
        }
        for (const object of expanded.objects) {
            // 别的探针自己的编译单元不属于这个探针：它的源码不会进入这个可执行文件。
            const owner = probeSpecific(object);
            if (owner !== undefined && owner !== probe) {
                continue;
            }
            const objectFiles = fileSetOf(object);
            if (objectFiles === undefined) {
                throw new GraphUnavailable(
                    `${object} 在依赖日志里没有有效记录（先完整构建一次即可修复）`,
                );
            }
            for (const file of objectFiles) {
                files.add(file);
            }
        }
        entries.set(name, {
            files: [...files].sort(),
            artifacts: [probeArtifact(probe, layout)],
            modules: inputs.previous?.get(name)?.modules ?? [],
        });
    }
    return entries;
}

/** 一个依赖条目涉及的全部仓库文件（代码依赖 + 运行期模块）。 */
export function entryFiles(entry: DependencyEntry): string[] {
    return [...new Set([...entry.files, ...entry.modules])].sort();
}

/**
 * 计算一个依赖条目的内容摘要：仓库文件 + 运行期模块 + 构建产物 + 环境。
 *
 * 环境由调用方给出（配置名、ninja 版本、Node 版本）；任一项变化即视为过期。
 * `sharedFiles` 是所有能力共用的输入哈希（由调用方在循环外算一次），避免逐个能力重复读同一批文件。
 */
export function entryDigest(
    layout: ProjectLayout,
    entry: DependencyEntry,
    cache: HashCache,
    environment: string,
    sharedFiles: Map<string, FileHash>,
): string {
    const files = hashFiles(
        entryFiles(entry).map((file) => path.join(REPO_ROOT, ...file.split("/"))),
        cache,
    );
    const artifacts = hashFiles(
        entry.artifacts.map((file) => path.join(layout.buildDir, ...file.split("/"))),
        cache,
    );
    const combined = new Map<string, FileHash>(sharedFiles);
    for (const [file, hash] of files) {
        combined.set(file, hash);
    }
    for (const [file, hash] of artifacts) {
        combined.set(file, hash);
    }
    combined.set("<environment>", environment);
    return digestOf(combined);
}

/**
 * 直接从上一次的记录重算摘要，跳过建图。
 *
 * 建图要调用 `ninja -t compdb` 与几千次 `-t deps`，在百万级仓库上要十几秒；而「要不要重跑」
 * 在绝大多数情况下答案都是「不用」——那就只把上次记下的依赖清单哈希一遍（有缓存时约 0.3 秒）。
 * 只有摘要变了才去建图，确认某个能力真的依赖了变化的文件，或者依赖清单本身需要更新。
 *
 * @param state 上次的状态文件
 * @param cache 哈希缓存（会被更新）
 * @param environment 环境指纹
 */
export function digestsFromRecords(
    layout: ProjectLayout,
    state: RunnerState,
    cache: HashCache,
    environment: string,
    sharedFiles: Map<string, FileHash>,
): Map<string, string> {
    const digests = new Map<string, string>();
    for (const [name, record] of Object.entries(state.harnesses)) {
        const entry: DependencyEntry = { files: record.files, artifacts: record.artifacts, modules: record.modules };
        const files = hashFiles(
            entryFiles(entry).map((file) => path.join(REPO_ROOT, ...file.split("/"))),
            cache,
        );
        const artifacts = hashFiles(
            entry.artifacts.map((file) => path.join(layout.buildDir, ...file.split("/"))),
            cache,
        );
        const combined = new Map<string, FileHash>(sharedFiles);
        for (const [file, hash] of files) {
            combined.set(file, hash);
        }
        for (const [file, hash] of artifacts) {
            combined.set(file, hash);
        }
        combined.set("<environment>", environment);
        digests.set(name, digestOf(combined));
    }
    return digests;
}

/**
 * 给定一批改动文件（仓库相对路径），找出依赖图里受影响的能力。
 *
 * 判定规则：能力的任一代码依赖或运行期模块出现在改动列表里。从未运行过的能力还没有
 * 运行期模块，调用方应把它单独当作「必须运行」——它本来就没有通过记录。
 */
export function affectedHarnesses(entries: ReadonlyMap<string, DependencyEntry>, changed: readonly string[]): string[] {
    const changedSet = new Set(changed.map((file) => file.split("\\").join("/")).filter((file) => file.length > 0));
    const affected: string[] = [];
    for (const [name, entry] of entries) {
        if (entryFiles(entry).some((file) => changedSet.has(file))) {
            affected.push(name);
        }
    }
    return affected.sort();
}