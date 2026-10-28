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
 * 沿用 Horizon 的共同操作序列与对象身份快照方法。
 * 服务端范围覆盖实例数据、隐藏链和时长推进；不包含客户端 BlendState。
 */
interface Instance {
    effect: number;
    duration: number;
    amplifier: number;
    ambient: boolean;
    visible: boolean;
    icon: boolean;
    hidden: number;
}

interface Operation {
    op: string;
    a: number;
    b: number;
    c: number;
    value: number;
    factor: number;
    mutate: boolean;
    fail: boolean;
}

function instance(overrides: Partial<Instance>): Instance {
    return { effect: 0, duration: 20, amplifier: 1, ambient: false, visible: true, icon: true, hidden: -1, ...overrides };
}

function op(name: string, overrides: Partial<Operation>): Operation {
    return { op: name, a: 0, b: 1, c: 2, value: 2, factor: 0x3f000000, mutate: false, fail: false, ...overrides };
}

const DURATIONS = [-2147483648, -2, -1, 0, 1, 2, 20, 32147, 2147483647];
const AMPLIFIERS = [-2147483648, -1, 0, 1, 2, 255, 256, 2147483647];
const FLOATS = [0, 0x80000000, 1, 0x3f000000, 0x3f800000, 0xbf800000, 0x40000000,
    0x7f800000, 0xff800000, 0x7fc00000, 0x7f7fffff];

function corpus(): Map<string, unknown> {
    const fixtures = new Map<string, unknown>();
    const add = (unit: string, instances: Instance[], ops: Operation[]) => {
        fixtures.set(unit, { unit, instances, ops });
    };
    for (const a of DURATIONS) {
        for (const b of DURATIONS) {
            for (const amplifier of AMPLIFIERS) {
                add(`update/${a}/${b}/${amplifier}`,
                    [instance({ duration: a }), instance({ duration: b, amplifier }), instance({ hidden: 0 })],
                    [op("update", {}), op("copy", {}), op("eq", {}), op("same", {}), op("tick", {})]);
            }
        }
        for (const factor of FLOATS) {
            add(`scale/${a}/${factor}`, [instance({ duration: a }), instance({ hidden: 0 })],
                [op("scale", { factor }), op("eq", {}), op("ends", { value: a }), op("infinite", {})]);
        }
    }
    for (let flags = 0; flags < 64; flags++) {
        add(`flags/${flags}`, [
            instance({ ambient: !!(flags & 1), visible: !!(flags & 2), icon: !!(flags & 4) }),
            instance({ ambient: !!(flags & 8), visible: !!(flags & 16), icon: !!(flags & 32) }),
            instance({}),
        ], [op("update", {}), op("copy", { b: 2 }), op("alias", { a: 2 }), op("update", { a: 1, b: 2 }), op("eq", {})]);
    }
    // 明确构造两级隐藏链，逐 tick 观察倒计时、恢复和外部别名看到的变化。
    add("hidden/restoration", [instance({ duration: 8, amplifier: 0 }),
        instance({ duration: 4, amplifier: 1 }), instance({ duration: 2, amplifier: 2 }), instance({ hidden: 0 })],
        [op("update", {}), op("update", { b: 2 }), op("copy", { b: 3 }),
            ...Array.from({ length: 9 }, () => op("tick", {}))]);

    // 固定种子生成序列；语料只在编排层生成，两侧不各自推演输入。
    let seed = 0x53544154;
    const next = (bound: number): number => {
        seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
        return Math.floor(seed / 0x100000000 * bound);
    };
    const choose = <T>(values: readonly T[]): T => values[next(values.length)];
    for (let i = 0; i < 1600; i++) {
        const instances = Array.from({ length: 6 }, (_, j) => instance({
            effect: next(3), duration: choose(DURATIONS), amplifier: choose(AMPLIFIERS),
            ambient: !!next(2), visible: !!next(2), icon: !!next(2), hidden: next(j + 1) - 1,
        }));
        const ops = Array.from({ length: 28 }, () => op(
            choose(["update", "update", "copy", "alias", "scale", "ends", "infinite", "eq", "same", "map", "tick"]), {
                a: next(6), b: next(6), c: next(6), value: choose(DURATIONS), factor: choose(FLOATS),
                mutate: !!next(2), fail: !!next(2),
            }));
        add(`sequence/${i}`, instances, ops);
    }
    // 回调可能修改自身别名后失败；零和无限时长则不调用回调。
    for (const duration of DURATIONS) {
        for (const fail of [false, true]) {
            add(`map/${duration}/${fail}`, [instance({ duration }), instance({ duration: 100, amplifier: 2 }), instance({})],
                [op("alias", { b: 2 }), op("map", { b: 2, c: 1, mutate: true, fail }), op("eq", { b: 2 })]);
        }
    }
    return fixtures;
}

const FIXTURES = corpus();

export function statusEffectUnits(): string[] {
    return [...FIXTURES.keys()];
}

export function statusEffectInput(unit: string): unknown {
    const fixture = FIXTURES.get(unit);
    if (fixture === undefined) {
        throw new Error(`Unknown status effect fixture: ${unit}`);
    }
    return fixture;
}
