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
 * build.ts — 构建差分探针，且只构建本次要用的那几个。
 *
 * `MC_BUILD_DIFF_PROBES` 默认 OFF，因此构建前必须显式打开；重复 configure 幂等（CMake 复用
 * 缓存），开销远小于一次编译。
 *
 * Windows 上必须先注入 Visual Studio 开发环境：vcpkg 的 manifest 安装要调 vswhere 找 MSVC，
 * 脱离 VS 环境直接跑 cmake 会报 "Unable to find a valid Visual Studio instance"，并把失败的
 * 配置写进缓存。scripts/vsenv.bat 干的就是这件事（设置 VS 变量与 VCPKG_ROOT）。
 *
 * 差分测试需要的是「增量构建几个 target」，不是项目规定的人工全量构建命令：
 * 这里在 VS 环境里直接调用 `cmake --build <构建目录> --config <配置> --target ...`，
 * 与人工构建共用同一个构建目录、同一个 ninja 图，不存在第二份构建状态。
 */

import { execFileSync } from "node:child_process";
import path from "node:path";

import type { ProjectLayout } from "./layout.ts";
import { REPO_ROOT } from "./layout.ts";

/** Windows 预设名（与 configure.bat 使用的预设一致）。 */
const WINDOWS_PRESET = "windows-clang-relwithdebinfo";

/** 执行一条需要 VS 环境的命令；Windows 下先 call vsenv.bat 再执行。 */
function execWithEnvironment(command: string, args: readonly string[]): void {
    if (process.platform !== "win32") {
        execFileSync(command, [...args], { cwd: REPO_ROOT, stdio: "inherit" });
        return;
    }
    const vsenv = path.join(REPO_ROOT, "scripts", "vsenv.bat");
    const line = [command, ...args.map((argument) => (argument.includes(" ") ? `"${argument}"` : argument))].join(" ");
    execFileSync("cmd", ["/c", `call "${vsenv}" >nul 2>&1 && ${line}`], { cwd: REPO_ROOT, stdio: "inherit" });
}

/** 构建指定探针（以及它们依赖的库目标）。 */
export function buildProbes(layout: ProjectLayout, probes?: readonly string[]): void {
    const targets = probes === undefined || probes.length === 0
        ? ["diff_probes"]
        : [...new Set(probes.map((probe) => `diff_${probe}`))];
    if (process.platform === "win32") {
        // configure.bat 的「非 build」分支把参数原样透传给 cmake，并在调用前注入 VS 环境。
        const configure = path.join(REPO_ROOT, "scripts", "configure.bat");
        execFileSync("cmd", ["/c", configure, "--preset", WINDOWS_PRESET, "-DMC_BUILD_DIFF_PROBES=ON", "-B", layout.buildDir], {
            cwd: REPO_ROOT,
            stdio: "inherit",
        });
        execWithEnvironment("cmake", ["--build", layout.buildDir, "--config", layout.config, "--target", ...targets]);
        return;
    }
    const preset = process.platform === "darwin" ? "macos-relwithdebinfo" : "linux-relwithdebinfo";
    execFileSync("cmake", ["--preset", preset, "-DMC_BUILD_DIFF_PROBES=ON", "-B", layout.buildDir], {
        cwd: REPO_ROOT,
        stdio: "inherit",
    });
    execWithEnvironment("cmake", ["--build", layout.buildDir, "--config", layout.config, "--target", ...targets]);
}

/** 构建全部探针（`--all` / 首次建图时使用）。 */
export function buildAllProbes(layout: ProjectLayout): void {
    buildProbes(layout, undefined);
}