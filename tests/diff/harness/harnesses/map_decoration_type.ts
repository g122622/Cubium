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

/**
 * map_decoration_type.ts — 地图装饰类型注册名的输入构造。
 *
 * **探针调用 Cubium 的真实实现**：`world::map::decorationTypeToString`。
 *
 * 三组关键用例：
 * - `toString|DESERT_VILLAGE`：**原版注册名是 `village_desert`**（`village_` 前缀），
 *   而 Cubium 输出纹理名 `desert_village`。预期差异点。
 * - `toString|WOODLAND_MANSION`：**原版注册名是短名 `mansion`**（长名才是纹理名）。
 * - `fromString|village_desert` / `fromString|desert_village`：Cubium 两种都接受。
 */

/** 地图装饰类型能力的一个单元。 */
export interface MapDecorationTypeUnit {
    readonly unit: string;
}

/**
 * 单元字符串原样透传。
 *
 * `toString` 输出注册名；`fromString` 输出常量名或字面量 `null`。
 */
export function mapDecorationTypeInput(unit: string): MapDecorationTypeUnit {
    return { unit };
}