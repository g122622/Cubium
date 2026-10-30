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
 * module-trace.ts — 记录执行进程实际加载的仓库内模块。
 *
 * 依赖图需要知道「这个能力的代码由哪些文件组成」，而代码组成不靠人工维护：执行进程用一个
 * 同步模块解析钩子记下所有被解析到仓库内的文件。TypeScript 源码在导入时必须写扩展名
 * （`./x.ts`），因此解析结果就是磁盘上真实存在的路径，不需要猜 `.ts`/`.js`/`index.ts`。
 *
 * 钩子是**进程级**的：只有在当前进程只服务一个能力时才可靠（换个能力时模块全部命中缓存、
 * 绕过钩子）。这正是「每个能力一个执行子进程」的理由之一。
 */

import { registerHooks } from "node:module";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { REPO_ROOT } from "./layout.ts";

/** 本文件自身：追踪基础设施，不算任何能力的业务代码。 */
const SELF = path.resolve(fileURLToPath(import.meta.url));

const loaded = new Set<string>();

let installed = false;

/** 安装模块解析钩子；重复调用只生效一次。 */
export function installModuleTrace(): void {
    if (installed) {
        return;
    }
    installed = true;
    registerHooks({
        resolve(specifier, context, nextResolve) {
            const result = nextResolve(specifier, context);
            if (result.url.startsWith("file:")) {
                const file = path.resolve(fileURLToPath(result.url));
                if (file.startsWith(REPO_ROOT + path.sep)) {
                    loaded.add(file);
                }
            }
            return result;
        },
    });
}

/** 已加载的仓库内模块（相对仓库根，正斜杠，排序）。 */
export function loadedModules(): string[] {
    return [...loaded]
        .filter((file) => file !== SELF)
        .map((file) => path.relative(REPO_ROOT, file).split(path.sep).join("/"))
        .sort();
}