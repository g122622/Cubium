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
 * recipe.ts — 配方：定义与装配结果。
 *
 * 参照实现是原版 `RecipeOracle`：先从它导出「具体三槽输入」夹具（`dump` 子命令），再让两侧
 * 用同一份输入做 `matches` / `assemble`。
 *
 * 为什么必须导出夹具：锻造配方的原料是标签，两侧各自从标签里取一个成员会得到不同的具体输入
 * （例如一侧取皮革靴、另一侧取钻石胸甲），比的是「标签成员顺序」而不是配方逻辑。夹具让样本
 * 由同一份数据决定。
 *
 * 夹具在 `beforeUnits` 里导出：它是单元枚举的输入，不是比对的一部分。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessContext, HarnessDeclaration } from "../core/types.ts";

interface RecipeFixture {
    readonly recipe: string;
    readonly assemblyInput?: readonly unknown[];
}

/** 配方能力的声明；导出夹具的状态放在闭包里（每个执行进程只运行一个能力）。 */
export function recipeDeclaration(): HarnessDeclaration {
    const fixtures = new Map<string, RecipeFixture>();
    return {
        name: "recipe",
        probe: "recipe",
        oracle: "RecipeOracle",
        key: "recipe",
        units: { kind: "computed", units: () => [...fixtures.keys()].sort() },
        input: (recipe: string): RecipeFixture => {
            const fixture = fixtures.get(recipe);
            if (fixture === undefined) {
                throw new Error(`No recipe fixture: ${recipe}`);
            }
            return fixture;
        },
        beforeUnits: (context: HarnessContext): void => {
            const directory = context.outputDirectory("recipe", "oracle");
            context.compileOracle("RecipeOracle", "recipe");
            const output = path.join(directory, "fixtures.jsonl");
            execFileSync(
                "java",
                [
                    "-cp",
                    context.oracleClasspath(context.classesDirectory("recipe")),
                    "dev.horizonlab.diff.RecipeOracle",
                    "dump",
                    output,
                ],
                { cwd: directory, env: context.environment, stdio: ["ignore", "pipe", "pipe"] },
            );
            fixtures.clear();
            for (const line of fs.readFileSync(output, "utf8").split(/\r?\n/).filter(Boolean)) {
                const fixture = JSON.parse(line) as RecipeFixture;
                fixtures.set(fixture.recipe, fixture);
            }
        },
    };
}