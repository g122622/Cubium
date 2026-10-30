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
 * layout.ts — 构建布局的单一来源：构建目录、配置名、产物路径、ninja 位置、仓库根。
 *
 * 这些路径规则以前散落在 runner.ts 与 java-oracle.ts 两处，任何一处改动都要同步改好几份。
 * 依赖图、状态文件、调度器都需要同一个答案，这里统一推导一次。
 *
 * 仓库根的推导：本文件位于 `tests/diff/harness/runner/`，仓库根在上三级。
 * 不依赖任何上层模块，避免循环导入。
 */

import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

/** 仓库根（`harness/runner/layout.ts` → `tests/diff` → `tests` → 根）。 */
export const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../../..");

/** 构建配置名，与 `cmake --build --config` 一致。 */
export const BUILD_CONFIG = "RelWithDebInfo";

export interface ProjectLayout {
    readonly repoRoot: string;
    readonly buildDir: string;
    readonly config: string;
    /** 多配置生成器的实现文件名：`impl-<配置>.ninja`。 */
    readonly implConfig: string;
    /** 可执行文件落点，相对构建目录（`bin/<配置>`），正斜杠。 */
    readonly binDirectory: string;
    /** 平台可执行文件后缀（`.exe` / 空）。 */
    readonly exeSuffix: string;
    /** ninja 可执行文件的绝对路径（来自 CMakeCache 的 CMAKE_MAKE_PROGRAM）。 */
    readonly ninja: string;
    /** 状态与缓存目录：`<构建目录>/diff/state`。 */
    readonly stateDirectory: string;
}

/** 构建目录：显式参数 > MC_DIFF_BUILD_DIR > 仓库根的 build/。 */
export function resolveBuildDir(explicit?: string): string {
    return path.resolve(explicit ?? process.env.MC_DIFF_BUILD_DIR ?? path.join(REPO_ROOT, "build"));
}

/**
 * 读取 CMakeCache 的 `CMAKE_MAKE_PROGRAM`。
 *
 * 不能直接用 PATH 上的 `ninja`：本机 PATH 里排在前面的是 depot_tools 的 shell 包装脚本，
 * 它要求 python3，而 Windows 上通常没有。CMake 记录的是构建时真正使用的二进制。
 */
function configuredMakeProgram(buildDir: string): string | undefined {
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

let cached: ProjectLayout | undefined;

/** 当前运行使用的构建布局。 */
export function projectLayout(buildDir?: string): ProjectLayout {
    const resolved = resolveBuildDir(buildDir);
    if (cached !== undefined && cached.buildDir === resolved) {
        return cached;
    }
    const ninja = configuredMakeProgram(resolved) ?? process.env.MC_DIFF_NINJA ?? (process.platform === "win32" ? "ninja.exe" : "ninja");
    cached = {
        repoRoot: REPO_ROOT,
        buildDir: resolved,
        config: BUILD_CONFIG,
        implConfig: `impl-${BUILD_CONFIG}`,
        binDirectory: `bin/${BUILD_CONFIG}`,
        exeSuffix: process.platform === "win32" ? ".exe" : "",
        ninja,
        stateDirectory: path.join(resolved, "diff", "state"),
    };
    return cached;
}

/** 逻辑 CPU 数（默认并行度用）。 */
export function cpuCount(): number {
    return os.cpus().length;
}

/** 当前可用内存（字节）。 */
export function freeMemoryBytes(): number {
    return os.freemem();
}

/**
 * 定位构建出的 probe 可执行文件。
 *
 * 按候选顺序直接看文件在不在，不猜生成器：多配置生成器落在 `bin/<配置>/`，单配置落在
 * `bin/`；`RUNTIME_OUTPUT_DIRECTORY_<配置>` 是整体覆盖而不是叠加，所以不会有
 * `bin/<配置>/diff/` 这一层，但历史上见过，一并兜底。
 */
export function probeExecutable(domain: string, layout: ProjectLayout = projectLayout()): string {
    const name = `diff_${domain}${layout.exeSuffix}`;
    const candidates = [
        path.join(layout.buildDir, "bin", layout.config, name),
        path.join(layout.buildDir, "bin", layout.config, "diff", name),
        path.join(layout.buildDir, "bin", name),
        path.join(layout.buildDir, "bin", "diff", name),
    ];
    for (const candidate of candidates) {
        if (fs.existsSync(candidate)) {
            return candidate;
        }
    }
    throw new Error(
        `找不到 probe 可执行文件（已查找 ${candidates.join("、")}）；` +
            "先运行 `node tests/diff/harness/runner.ts build`，或确认 MC_DIFF_BUILD_DIR 指向正确的构建目录",
    );
}

/** 探针可执行文件相对构建目录的路径（依赖图与构建目标名使用）。 */
export function probeArtifact(domain: string, layout: ProjectLayout = projectLayout()): string {
    return `${layout.binDirectory}/diff_${domain}${layout.exeSuffix}`;
}

/**
 * 路径是否位于某个目录之内。
 *
 * 用于把「构建目录里的生成物」（pch、第三方解包源码）从仓库依赖里剔除：它们由构建系统维护，
 * 不由本仓库修改，计入依赖只会让「重新配置」误触发重跑。
 */
export function isInside(file: string, directory: string): boolean {
    const relative = path.relative(path.resolve(directory), path.resolve(file));
    return relative.length > 0 && !relative.startsWith("..") && !path.isAbsolute(relative);
}
