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
 * food_properties.ts — 食物属性能力的输入构造。
 *
 * 单元的枚举由 oracle 的 `list` 决定（原版哪些物品带食物属性就是全集），探针侧按同一个
 * 物品 ID 输出自己的属性。比三项：饥饿值、饱和度修正、能否饱腹后继续吃。
 *
 * 不比较食用时长、动画与附加效果：原版这些在 `Consumable` 组件与数据包里，
 * 与 Cubium `Food` 的字段划分不同，属于两套结构，强行对齐会引入假差异。
 */

/** 物品 ID → 探针与 oracle 共用的一行输入。 */
export function foodPropertiesInput(unit: string): { item: string } {
    return { item: unit };
}