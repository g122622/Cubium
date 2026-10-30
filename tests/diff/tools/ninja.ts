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
 * ninja.ts — 从构建系统里读出「一个可执行文件由哪些源文件构成」。
 *
 * 差分编排的增量判定需要回答：改了某个 .cpp/.hpp，会影响哪些 harness？答案全部在构建图里，
 * 不需要（也不应该）人工维护清单：
 *
 *   1. 该配置的 ninja 文件（多配置生成器为 `CMakeFiles/impl-<配置>.ninja`）里，
 *      可执行文件的链接语句给出它自己的 .obj 与顺序依赖里的静态库；
 *   2. 静态库的链接语句给出构成它的全部 .obj（展开一层即可，库不嵌套）；
 *   3. 每个 .obj 的头文件闭包由 `ninja -t deps <目标>` 给出——这是 ninja 自己维护的
 *      依赖日志，与编译时 `-MD` 写下的内容完全一致，比重新跑一遍编译器预处理快几个数量级。
 *
 * 关于 ninja 的参数形式（实测，ninja 1.13 + CMake 多配置生成器）：
 *   - 不带 `-f` 时 ninja 读 `build.ninja`，那是「默认配置」的选择器；这里必须用
 *     `-f build-<配置>.ninja` 指定配置，否则 `-t deps` 与当前配置对不上；
 *   - `-t deps` 可以一次接收多个目标（批量），但命令行长度有上限：上千个目标会触发
 *     `ENAMETOOLONG`，必须分块调用；
 *   - `-t deps` **不带目标**时不列出任何内容（它不会列出全部目标），所以批量查询必须
 *     显式给出目标列表。
 *
 * ninja 可执行文件取自 CMakeCache 的 `CMAKE_MAKE_PROGRAM`：本机 PATH 上的 `ninja`
 * 可能是 depot_tools 的 Python 包装脚本，依赖 python3，而 Windows 上通常没有。
 *
 * 只在「建图」时调用 ninja（一次 compdb 列出全部编译目标，再分块批量查询依赖），
 * 结果写进状态文件；运行 harness 时只读文件，不启动进程。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

/** 构建图里的一条链接语句。 */
export interface NinjaLinkRule {
    /** 普通输入（编译产物：.obj）。 */
    readonly inputs: readonly string[];
    /** 顺序依赖（`| ` 之后）：静态库都在这里。 */
    readonly orderOnly: readonly string[];
}

export interface NinjaTargetDeps {
    readonly deps: readonly string[];
    /** 依赖日志里该目标是否有效；无效说明编译后日志没有刷新，依赖不可信。 */
    readonly valid: boolean;
}

export interface NinjaModel {
    readonly buildDir: string;
    readonly linkRules: ReadonlyMap<string, NinjaLinkRule>;
    /** 目标（其在依赖日志中的键）→ 依赖列表。 */
    readonly deps: ReadonlyMap<string, NinjaTargetDeps>;
}

/** 还原 ninja 的 `$` 转义（`$ ` 空格、`$:` 冒号、`$$` 美元符）。 */
export function unescapeNinja(token: string): string {
    return token.replace(/\$[ :$]/g, (match) => match[1]);
}

function normalize(token: string): string {
    return unescapeNinja(token).replace(/\\/g, "/");
}

/**
 * 读取 CMakeCache 的 `CMAKE_MAKE_PROGRAM`：构建时真正使用的 ninja 二进制。
 *
 * 不能直接用 PATH 上的 `ninja`：本机 PATH 里排在前面的可能是 depot_tools 的 shell
 * 包装脚本，它要求 python3，而 Windows 上通常没有。
 */
export function configuredMakeProgram(buildDir: string): string | undefined {
    const cachePath = path.join(buildDir, "CMakeCache.txt");
    if (!fs.existsSync(cachePath)) {
        return undefined;
    }
    const prefix = "CMAKE_MAKE_PROGRAM:FILEPATH=";
    for (const line of fs.readFileSync(cachePath, "utf8").split("\n")) {
        if (line.startsWith(prefix)) {
            const value = line.slice(prefix.length).trim();
            return value.length > 0 ? value : undefined;
        }
    }
    return undefined;
}

/**
 * 解析构建文件里的链接语句。
 *
 * 只收「产出 .exe 或 lib/ 下的 .lib」的语句：编译语句每文件一行、数量上千，
 * 它们的依赖由 `ninja -t deps` 整体给出，不需要在这里解析。
 *
 * 行格式：`build <输出...>: <规则> <普通输入...> | <顺序依赖...> || <隐式输入> <隐式输出>`。
 * `||` 之后的隐式部分与链接无关（那里面是 DLL 复制之类的依赖），先截掉。
 */
function parseLinkRules(file: string): Map<string, NinjaLinkRule> {
    const rules = new Map<string, NinjaLinkRule>();
    for (const line of fs.readFileSync(file, "utf8").split("\n")) {
        if (!line.startsWith("build ")) {
            continue;
        }
        const afterBuild = line.slice("build ".length);
        const colon = afterBuild.indexOf(": ");
        if (colon < 0) {
            continue;
        }
        const outputs = afterBuild.slice(0, colon).trim().split(/\s+/).map(normalize);
        if (!outputs.some((output) => output.endsWith(".exe") || (output.startsWith("lib/") && output.endsWith(".lib")))) {
            continue;
        }
        const beforeImplicit = afterBuild.slice(colon + 2).split(" || ")[0];
        const segments = beforeImplicit.split(" | ");
        const ruleAndInputs = segments[0];
        const space = ruleAndInputs.indexOf(" ");
        const inputs = space < 0 ? [] : splitPaths(ruleAndInputs.slice(space + 1));
        const orderOnly = splitPaths(segments[1] ?? "");
        for (const output of outputs) {
            rules.set(output, { inputs, orderOnly });
        }
    }
    return rules;
}

function splitPaths(text: string): string[] {
    const trimmed = text.trim();
    return trimmed.length === 0 ? [] : trimmed.split(/\s+/).map(normalize);
}

/**
 * 用 `ninja -f <构建文件> -t deps <目标...>` 批量查询头文件闭包。
 *
 * 输出格式（ninja 1.13），每个目标一段：
 *
 *   <目标>: #deps <n>, deps mtime <mtime> (<VALID|STALE|INCORRECT|MISSING|empty>)
 *       <依赖路径>
 *       ...
 */
export function queryDependencies(ninja: string, buildDir: string, buildFile: string, targets: readonly string[]): Map<string, NinjaTargetDeps> {
    const result = new Map<string, NinjaTargetDeps>();
    // 分块：Windows 命令行长度上限约 32 KiB，每个目标路径平均近百字节，256 个一块有充足余量。
    const chunkSize = 256;
    for (let start = 0; start < targets.length; start += chunkSize) {
        const chunk = targets.slice(start, start + chunkSize);
        // 管道捕获在 Windows 上很慢（几千个目标、数百万行输出能到二十秒）。依赖日志是纯文本、
        // 每行以「目标: #deps」开头，改为写到文件再一次性读回：解析逻辑完全相同，只是不进管道。
        const capture = path.join(buildDir, `.diff-ninja-deps.${process.pid}.txt`);
        const handle = fs.openSync(capture, "w");
        try {
            execFileSync(ninja, ["-f", buildFile, "-t", "deps", ...chunk], {
                cwd: buildDir,
                stdio: ["ignore", handle, "pipe"],
            });
        } finally {
            fs.closeSync(handle);
        }
        const output = fs.readFileSync(capture, "utf8");
        fs.rmSync(capture, { force: true });
        let current: string | undefined;
        let valid = true;
        let deps: string[] = [];
        const flush = (): void => {
            if (current !== undefined) {
                result.set(current, { deps, valid });
            }
        };
        for (const raw of output.split("\n")) {
            const line = raw.endsWith("\r") ? raw.slice(0, -1) : raw;
            if (line.length === 0) {
                continue;
            }
            if (line.charCodeAt(0) !== 0x20) {
                flush();
                const colon = line.indexOf(": #deps");
                current = colon < 0 ? line.replace(/:$/, "") : line.slice(0, colon);
                valid = line.includes("(VALID)");
                deps = [];
                continue;
            }
            deps.push(line.trim());
        }
        flush();
    }
    return result;
}

/**
 * 建立构建模型：解析链接语句，列出当前配置的可编译目标，并批量查询它们的依赖。
 *
 * @param buildDir 构建目录
 * @param implConfig 多配置生成器的实现文件名（如 `impl-RelWithDebInfo`）；单配置生成器传 `build`。
 */
export function loadNinjaModel(buildDir: string, implConfig: string, ninja: string): NinjaModel {
    const linkFile = implConfig === "build"
        ? path.join(buildDir, "build.ninja")
        : path.join(buildDir, "CMakeFiles", `${implConfig}.ninja`);
    if (!fs.existsSync(linkFile)) {
        throw new Error(`找不到构建图文件：${linkFile}`);
    }
    const linkRules = parseLinkRules(linkFile);
    // compdb 的文件名：多配置生成器用 `build-<配置>.ninja`（别名 build.ninja 指向默认配置），
    // 单配置生成器直接用 build.ninja。
    const compdbFile = implConfig === "build" ? "build.ninja" : `build-${implConfig.replace(/^impl-/, "")}.ninja`;
    const compdb = JSON.parse(
        execFileSync(ninja, ["-f", compdbFile, "-t", "compdb"], {
            cwd: buildDir,
            encoding: "utf8",
            maxBuffer: 1024 * 1024 * 1024,
            stdio: ["ignore", "pipe", "pipe"],
        }),
    ) as Array<{ output: string }>;
    // 只查本仓库的编译产物：vcpkg 依赖的源码不在仓库里，也不由我们修改。
    const targets = compdb
        .map((entry) => entry.output)
        .filter((output) => typeof output === "string" && output.endsWith(".obj") && !output.startsWith("_deps/") && !output.startsWith("vcpkg_installed/"));
    const deps = queryDependencies(ninja, buildDir, compdbFile, targets);
    return { buildDir, linkRules, deps };
}

/**
 * 一个可执行文件的全部编译产物来源。
 *
 * @returns 构成该可执行文件的 .obj（构建目录相对路径），以及构建图里缺失的静态库清单。
 */
export function executableObjects(model: NinjaModel, executable: string): { objects: string[]; missing: string[] } {
    const rule = model.linkRules.get(executable);
    if (rule === undefined) {
        throw new Error(`构建图里没有可执行文件 ${executable} 的链接语句`);
    }
    const objects = new Set<string>();
    for (const input of rule.inputs) {
        if (input.endsWith(".obj")) {
            objects.add(input);
        }
    }
    const missing: string[] = [];
    for (const library of rule.orderOnly) {
        if (!library.startsWith("lib/") || !library.endsWith(".lib")) {
            continue;
        }
        const libraryRule = model.linkRules.get(library);
        if (libraryRule === undefined) {
            missing.push(library);
            continue;
        }
        for (const input of libraryRule.inputs) {
            if (input.endsWith(".obj")) {
                objects.add(input);
            }
        }
    }
    return { objects: [...objects].sort(), missing };
}

/** 目标对应的依赖（依赖日志口径）；找不到或无效时返回 undefined。 */
export function targetDependencies(model: NinjaModel, object: string): readonly string[] | undefined {
    const entry = model.deps.get(object);
    if (entry === undefined || !entry.valid) {
        return undefined;
    }
    return entry.deps;
}