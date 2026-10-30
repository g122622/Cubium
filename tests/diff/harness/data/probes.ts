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
 * probes.ts — 差分探针的能力清单。
 *
 * tests/diff/CMakeLists.txt 用三张平行列表登记全部探针（后缀 / 源文件 / 宏名）。这里直接从
 * 那份 CMake 清单读出域名：新增探针只需要改 CMake 一处，编排层自动认识它（不用在两处各登记
 * 一次，也就不会漂移），「只构建本次要用的 probe」也有了可靠的 target 名来源。
 *
 * CMakeLists 不可读或清单不完整时直接报错：清单是探针构建的唯一事实来源，猜不出来。
 */

import fs from "node:fs";
import path from "node:path";

import { REPO_ROOT } from "../runner/layout.ts";

const CMAKE_LISTS = "tests/diff/CMakeLists.txt";

/** 从 CMakeLists 里取一个 `set(变量 值...)` 的全部值。 */
function cmakeList(text: string, variable: string): string[] {
    const match = new RegExp(`set\\(${variable}\\s+([^)]*)\\)`, "s").exec(text);
    if (match === null) {
        throw new Error(`${CMAKE_LISTS} 里找不到 set(${variable} ...)，探针清单无法读取`);
    }
    return match[1].trim().split(/\s+/).filter((entry) => entry.length > 0);
}

/** 全部探针的域名（与可执行文件 diff_<域名> 的域名一致）。 */
export function probeDomains(): string[] {
    const text = fs.readFileSync(path.join(REPO_ROOT, ...CMAKE_LISTS.split("/")), "utf8");
    const domains = cmakeList(text, "MC_DIFF_PROBE_DOMAINS");
    const suffixes = cmakeList(text, "MC_DIFF_PROBE_SUFFIXES");
    const sources = cmakeList(text, "MC_DIFF_PROBE_SOURCES");
    if (domains.length !== suffixes.length || domains.length !== sources.length) {
        throw new Error(
            `${CMAKE_LISTS} 的三张探针清单长度不一致（后缀 ${suffixes.length}，源文件 ${sources.length}，宏名 ${domains.length}）`,
        );
    }
    return domains;
}