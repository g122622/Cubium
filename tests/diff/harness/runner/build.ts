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

/**
 * Windows 预设名。
 *
 * 用 server 预设（`MC_BUILD_CLIENT=OFF`）：客户端已停止维护，`vcpkg.json` 不再保证提供
 * glfw3 等客户端依赖，用含客户端的预设会在 configure 阶段直接失败；差异探针本身也不依赖
 * `src/client`（`tests/diff/CMakeLists.txt` 未引用它）。与日常构建共用同一个 build 目录。
 */
const WINDOWS_PRESET = "windows-clang-relwithdebinfo-server";

/**
 * 把命令包装成 `cmd /c` 的单行脚本。
 *
 * 整行是 cmd 自己的语法，反斜杠不是转义符，但 `path.join` 产出的 Windows 路径带反斜杠，
 * 直接拼进 `cmd /c "..."` 会被 Node 的引号处理吃掉一层，路径静默退化成
 * `E:devCubiumranch-1`。因此改用正斜杠（cmd 与 cmake 都接受），彻底绕开转义问题。
 */
function toCmdLine(parts: readonly string[]): string {
    return parts.map((part) => part.split("\\").join("/")).join(" ");
}

/** 执行一条需要 VS 环境的命令；Windows 下先 call vsenv.bat 再执行。 */
function execWithEnvironment(command: string, args: readonly string[]): void {
    if (process.platform !== "win32") {
        execFileSync(command, [...args], { cwd: REPO_ROOT, stdio: "inherit" });
        return;
    }
    const vsenv = toCmdLine([path.join(REPO_ROOT, "scripts", "vsenv.bat")]);
    const line = toCmdLine([command, ...args.map((argument) => (argument.includes(" ") ? `"${argument}"` : argument))]);
    execFileSync("cmd", ["/c", `call ${vsenv} >nul 2>&1 && ${line}`], { cwd: REPO_ROOT, stdio: "inherit" });
}

/**
 * 构建探针。
 *
 * 全部能力共用一个 `diff_runner` 目标：无论请求哪些能力，构建的是同一个可执行文件。
 * 参数保留只是为了签名稳定（历史上每能力一个 target），不再决定构建目标。
 */
export function buildProbes(layout: ProjectLayout, probes?: readonly string[]): void {
    void probes;
    const targets = ["diff_runner"];
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