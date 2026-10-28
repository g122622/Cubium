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

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { oracleClasspath, prepareOracle, vanillaDataPackDir } from "../oracle-env.ts";
import { resolveBuildDir } from "../runner.ts";

interface RecipeFixture {
    readonly recipe: string;
    readonly assemblyInput?: readonly ({ readonly id: string; readonly count: number } | null)[];
}

/** 先由原版导出具体三槽输入，再将同一份输入交给两侧。 */
export function createRecipeHarness() {
    const fixtures = new Map<string, RecipeFixture>();
    return {
        name: "recipe",
        oracle: "RecipeOracle",
        key: "recipe",
        units: (): string[] => {
            // runJavaOracle 在枚举单元之前已编译 oracle，此处只运行导出命令。
            const directory = path.join(resolveBuildDir(), "diff/recipe/java");
            const environment = prepareOracle(directory);
            const output = path.join(directory, "fixtures.jsonl");
            execFileSync("java", [
                "-cp", environment.classesDir + path.delimiter + oracleClasspath(environment),
                "dev.horizonlab.diff.RecipeOracle", "dump", output,
            ], {
                cwd: directory,
                env: { ...process.env, MC_DIFF_VANILLA_DATAPACK: vanillaDataPackDir() },
                stdio: ["ignore", "pipe", "pipe"],
            });
            fixtures.clear();
            for (const line of fs.readFileSync(output, "utf8").split(/\r?\n/).filter(Boolean)) {
                const fixture = JSON.parse(line) as RecipeFixture;
                fixtures.set(fixture.recipe, fixture);
            }
            return [...fixtures.keys()].sort();
        },
        input: (recipe: string): RecipeFixture => {
            const fixture = fixtures.get(recipe);
            if (fixture === undefined) {
                throw new Error(`No recipe fixture: ${recipe}`);
            }
            return fixture;
        },
    };
}
