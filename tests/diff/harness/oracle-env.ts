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
 * oracle-env.ts — 定位并准备原版 Java oracle 所需的运行时。
 *
 * 权威参照实现是原版 1.21.11 的**已反混淆 jar**（Mojang mappings），直接调用原版 API。
 * 它不进入 Git，也不随发布分发；由本脚本在本机定位，找不到就明确报错并给出获取方式。
 *
 * 查找顺序（可用环境变量覆盖）：
 *   MC_VANILLA_MERGED_JAR  → 反混淆后的 merged jar
 *   MC_VANILLA_SERVER_JAR  → bundler jar（内含混淆服务端与 39 个依赖库）
 *
 * fence 默认位置是 Gradle/fabric-loom 的缓存，与该机器上的构建环境一致。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

/** 仓库根（tests/diff/harness → tests/diff → tests → 根）。 */
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../..");

/** 反混淆 jar（优先）。含 net.minecraft.* 的可读类名。 */
const MERGED_CANDIDATES = [
    path.join(
        os.homedir(),
        ".gradle/caches/fabric-loom/minecraftMaven/net/minecraft/minecraft-merged",
        "1.21.11-loom.mappings.1_21_11.layered+hash.1230526264-v2",
        "minecraft-merged-1.21.11-loom.mappings.1_21_11.layered+hash.1230526264-v2.jar",
    ),
];

/** bundler jar（备选：需要先解包出依赖）。 */
const SERVER_CANDIDATES = [
    path.join(os.homedir(), ".gradle/caches/neoformruntime/artifacts/minecraft_1.21.11_server.jar"),
];

export interface OracleEnvironment {
    /** 反混淆的 minecraft jar。 */
    readonly minecraftJar: string;
    /** 依赖库所在目录（bundler 解包后的 META-INF/libraries）。 */
    readonly librariesDir: string;
    /** 编译产物目录。 */
    readonly classesDir: string;
}

function firstExisting(candidates: readonly string[]): string | undefined {
    return candidates.find((candidate) => fs.existsSync(candidate));
}

/**
 * 解包 bundler 的依赖库。
 *
 * bundler 的结构是标准的：`META-INF/versions/<ver>/server-<ver>.jar` 是内层服务端，
 * `META-INF/libraries/**` 是 39 个依赖。这里解到 `<build>/diff/java/libraries`，
 * 解过一次就复用（用版本文件作哨兵）。
 */
function unpackLibraries(bundler: string, cacheDir: string): string {
    const librariesDir = path.join(cacheDir, "META-INF", "libraries");
    const marker = path.join(cacheDir, ".unpacked");
    if (fs.existsSync(marker)) {
        return librariesDir;
    }
    fs.mkdirSync(cacheDir, { recursive: true });
    // 用 jar 工具而非 node 解压：bundler 内部有大量空目录项，只解 .jar 文件。
    execFileSync("jar", ["xf", bundler, "META-INF/libraries/"], { cwd: cacheDir, stdio: "ignore" });
    fs.writeFileSync(marker, "ok");
    return librariesDir;
}

/** 准备 oracle 环境；缺任何一件都抛出带指引的错误。 */
export function prepareOracle(cacheDir: string): OracleEnvironment {
    const minecraftJar = process.env.MC_VANILLA_MERGED_JAR ?? firstExisting(MERGED_CANDIDATES);
    if (minecraftJar === undefined) {
        throw new Error(
            "找不到原版反混淆 jar。请设置 MC_VANILLA_MERGED_JAR 指向 1.21.11 的 merged jar" +
                "（fabric-loom 生成的 minecraft-merged-*.jar），或先用 Gradle/loom 生成一份。",
        );
    }

    const librariesDir = process.env.MC_VANILLA_LIBRARIES ?? (() => {
        const bundler = process.env.MC_VANILLA_SERVER_JAR ?? firstExisting(SERVER_CANDIDATES);
        if (bundler === undefined) {
            throw new Error(
                "找不到原版服务端 bundler jar（提供依赖库）。请设置 MC_VANILLA_SERVER_JAR，" +
                    "或用 MC_VANILLA_LIBRARIES 直接指定依赖库目录。",
            );
        }
        return unpackLibraries(bundler, path.join(cacheDir, "bundler"));
    })();

    return { minecraftJar, librariesDir, classesDir: path.join(cacheDir, "classes") };
}

/**
 * 原版数据包目录（含 `data/` 与 `pack.mcmeta` 的那一层）。
 *
 * 战利品、进阶、配方这类「资源」不在 jar 的类文件里，而是数据包 JSON；
 * 探针按目录直接读取，因此两侧都必须指向**同一份**目录。
 */
export function vanillaDataPackDir(): string {
    const explicit = process.env.MC_VANILLA_DATAPACK;
    if (explicit !== undefined && explicit.length > 0) {
        return explicit;
    }
    const candidates = [
        path.join(REPO_ROOT, "tests/diff/harness/datapacks/Vanilla"),
        path.resolve(os.homedir(), "minecraft_reborn", "datapacks", "Vanilla"),
    ];
    for (const candidate of candidates) {
        if (fs.existsSync(path.join(candidate, "pack.mcmeta"))) {
            return candidate;
        }
    }
    throw new Error(
        "找不到原版数据包目录。请设置 MC_DIFF_VANILLA_DATAPACK 指向含 pack.mcmeta 的 Vanilla 数据包目录。",
    );
}

/** 收集依赖库 jar，拼成 Java 的 classpath（Windows 用分号，其余用冒号）。 */
export function oracleClasspath(environment: OracleEnvironment): string {
    const jars = [environment.minecraftJar];
    const walk = (dir: string): void => {
        for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
            const full = path.join(dir, entry.name);
            if (entry.isDirectory()) {
                walk(full);
            } else if (entry.name.endsWith(".jar")) {
                jars.push(full);
            }
        }
    };
    if (fs.existsSync(environment.librariesDir)) {
        walk(environment.librariesDir);
    }
    return jars.join(path.delimiter);
}