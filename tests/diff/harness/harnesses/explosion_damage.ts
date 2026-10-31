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
 * explosion_damage.ts — 爆炸对实体伤害与击退能力的输入构造。
 *
 * 单元形如 `radius|distance|seenPercent|knockbackResistance`，由探针与 oracle 的
 * `list` 各自枚举同一张表。距离取值刻意跨越 `distance == radius * 2` 这条边界
 * （`inRange` 的分界），也包含超距值，用来验证「距离比例不钳制」这条口径。
 */

/** 爆炸伤害能力的一个单元。 */
export interface ExplosionDamageUnit {
    readonly radius: string;
    readonly distance: string;
    readonly seenPercent: string;
    readonly knockbackResistance: string;
}

/**
 * 单元字符串 → 探针与 oracle 共用的一行输入。
 *
 * 四个分量全部按字符串传递：`0.25` 这类值在 JSON 数字里有多种写法，统一走字符串
 * 可以保证两侧解析出同一个值。
 */
export function explosionDamageInput(unit: string): ExplosionDamageUnit {
    const parts = unit.split("|");
    if (parts.length !== 4) {
        throw new Error(`explosion_damage: 单元应为 4 段 '半径|距离|可见度|抗性'：${unit}`);
    }
    return {
        radius: parts[0],
        distance: parts[1],
        seenPercent: parts[2],
        knockbackResistance: parts[3],
    };
}