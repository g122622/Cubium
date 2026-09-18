/*
 * expect.ts — 断言助手。
 *
 * 比裸 assert 多做一件事：失败时输出**期望值 vs 实际值**的可读对照，
 * 并支持在断言里附带上文（例如「挖掉 x,y,z 后方块应变为空气」）。
 */

/** 断言失败（与 bot 故障、超时区分开，便于 runner 分类报告）。 */
export class AssertionError extends Error {
    constructor(message: string) {
        super(message);
        this.name = "AssertionError";
    }
}

function render(value: unknown): string {
    const text = JSON.stringify(value);
    return text === undefined ? String(value) : text;
}

/** 断言两个值全等。 */
export function expectEq(actual: unknown, expected: unknown, context: string): void {
    if (actual !== expected) {
        throw new AssertionError(`${context}\n  期望: ${render(expected)}\n  实际: ${render(actual)}`);
    }
}

/** 断言两个 JSON 可序列化值深度相等。 */
export function expectDeepEq(actual: unknown, expected: unknown, context: string): void {
    const a = JSON.stringify(actual);
    const b = JSON.stringify(expected);
    if (a !== b) {
        throw new AssertionError(`${context}\n  期望: ${b}\n  实际: ${a}`);
    }
}

/** 断言条件为真。 */
export function expectTrue(condition: boolean, context: string): void {
    if (!condition) {
        throw new AssertionError(context);
    }
}

/** 断言数值落在闭区间内。 */
export function expectBetween(actual: number, min: number, max: number, context: string): void {
    if (!(actual >= min && actual <= max)) {
        throw new AssertionError(`${context}\n  期望区间: [${min}, ${max}]\n  实际: ${actual}`);
    }
}

/** 断言值属于给定集合。 */
export function expectOneOf(actual: unknown, allowed: readonly unknown[], context: string): void {
    if (!allowed.includes(actual)) {
        throw new AssertionError(`${context}\n  期望之一: ${render(allowed)}\n  实际: ${render(actual)}`);
    }
}
