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
 * entity_shape.ts — 实体碰撞箱尺寸。
 *
 * 单元列表取自原版运行时注册表（`EntityShapeOracle list`），比对由 java-oracle.ts
 * 完成：宽度、高度、眼睛高度、固定尺寸、分类全部逐字节比。
 * 不再使用 minecraft-data——它只给宽高两项，参考价值有限。
 */

import type { HarnessSpec } from "../runner.ts";

/**
 * 返回单元提供者（方块/实体等）的 stub。
 *
 * 这里保留 HarnessSpec 形状是为了让 runAll 的统一迭代保持简单；实际的单元枚举
 * 由 Java oracle 的 `list` 子命令给出，见 harnesses/index.ts 的 JAVA_HARNESSES。
 */
export function createEntityShapeHarness(): HarnessSpec {
    return {
        name: "entity_shape",
        domain: "entity_shape",
        key: "entity",
        units: () => [],
        input: (unit) => ({ entity: unit }),
        oracle: (unit) => ({ entity: unit }),
    } satisfies HarnessSpec;
}