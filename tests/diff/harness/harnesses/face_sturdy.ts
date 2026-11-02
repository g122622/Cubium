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
 * face_sturdy.ts — `isFaceSturdy` 面朝支撑判定的输入构造。
 *
 * **探针调用 Cubium 的真实实现**：`BlockState::isFaceSturdy`。
 *
 * 单元格式：`<block>|<Direction>|<SupportType>`，输出 `"true"` / `"false"`。
 *
 * 这是**所有附着型方块存活判定的公共底层**：紫水晶簇、发光地衣、幽匿脉络、
 * 珊瑚、告示牌、火把、梯子的 `canSurvive` 最终都落到这个方法上。
 *
 * 三种支撑类型对同一方块可给出不同答案：
 * - 完整方块：`FULL=CENTER=RIGID=true`
 * - 台阶下半：`FULL=false`，`CENTER=true`，`RIGID=true`
 * - 地毯：三者全 `false`
 * - 玻璃板：`FULL=false`，`CENTER=true`，`RIGID=false`
 */

/** 面朝支撑能力的一个单元。 */
export interface FaceSturdyUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function faceSturdyInput(unit: string): FaceSturdyUnit {
    return { unit };
}