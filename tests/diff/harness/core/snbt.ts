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
 * snbt.ts — 把 SNBT 文本重写成唯一规范形，用于两侧比对。
 *
 * 为什么需要：同一个 NBT 标签在两侧由**不同的打印器**产出——原版是 `StringTagVisitor`，
 * Cubium 是 `nbt::contexts::mojangson`。两者在下列地方写法不同，但标签本身相同：
 *
 *   | 差异 | 原版 | Cubium |
 *   |---|---|---|
 *   | 字符串含特殊字符时用单引号 | `'{"text":"x"}'` | `"{\"text\":\"x\"}"` |
 *   | 浮点写 `1.5` 还是 `1.5f` | `4.0f` | `4f` |
 *   | 布尔 / 单字节 | `1b` | `1b` |
 *   | 短整数后缀 | `4s` | `4s` |
 *
 * 另外两侧的**键顺序**都可能不稳定（`CompoundTag` 内部是 map，但打印顺序不保证）。
 *
 * 规范形做三件事，全部是等价变换（不丢弃任何信息，只换写法）：
 *   1. 解析 SNBT → 统一的中间表示（类型 + 值）；
 *   2. 按类型与值重新打印：数字去掉多余的小数位、字符串**一律**用双引号转义、复合键按名字排序；
 *   3. 任何解析不了的部分原样保留，比较时照常报出差异（绝不静默吞掉）。
 *
 * 类型后缀保留原样（`1b` 与 `1` 不同，`4.0f` 与 `4.0d` 不同）：那正是我们要比的东西。
 */

/** SNBT 值的中间表示。 */
type SnbtValue =
    | { readonly kind: "string"; readonly value: string; readonly quoted: boolean }
    | { readonly kind: "number"; readonly value: number; readonly suffix: string }
    | { readonly kind: "compound"; readonly entries: [string, SnbtValue][] }
    | { readonly kind: "list"; readonly items: SnbtValue[]; readonly arrayPrefix?: string };

class Cursor {
    index = 0;
    readonly text: string;
    constructor(text: string) {
        this.text = text;
    }
    get done(): boolean {
        return this.index >= this.text.length;
    }
    peek(offset = 0): string {
        return this.text[this.index + offset] ?? "";
    }
    take(): string {
        return this.text[this.index++] ?? "";
    }
    skipSpace(): void {
        while (!this.done && /\s/.test(this.text[this.index]!)) {
            this.index++;
        }
    }
}

class Unparsable extends Error {}

function parseValue(cursor: Cursor): SnbtValue {
    cursor.skipSpace();
    const ch = cursor.peek();
    if (ch === "{") {
        return parseCompound(cursor);
    }
    if (ch === "[") {
        return parseList(cursor);
    }
    if (ch === "'" || ch === '"') {
        return { kind: "string", value: parseQuoted(cursor), quoted: true };
    }
    return parseBare(cursor);
}

function parseQuoted(cursor: Cursor): string {
    const quote = cursor.take();
    let out = "";
    while (!cursor.done) {
        const ch = cursor.take();
        if (ch === "\\") {
            const next = cursor.take();
            out += next === "n" ? "\n" : next === "t" ? "\t" : next === "r" ? "\r" : next;
            continue;
        }
        if (ch === quote) {
            return out;
        }
        out += ch;
    }
    throw new Unparsable("unterminated string");
}

/** 未加引号的标量：数字（可带类型后缀）或裸字符串（标签名、资源位置……）。 */
function parseBare(cursor: Cursor): SnbtValue {
    let text = "";
    while (!cursor.done) {
        const ch = cursor.peek();
        if (ch === "," || ch === "}" || ch === "]" || ch === ":" || /\s/.test(ch)) {
            break;
        }
        text += cursor.take();
    }
    if (text.length === 0) {
        throw new Unparsable(`empty token at ${cursor.index}`);
    }
    const numeric = /^(-?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?)([bBsSlLfFdD])?$/.exec(text);
    if (numeric !== null) {
        return { kind: "number", value: Number(numeric[1]), suffix: (numeric[2] ?? "").toLowerCase() };
    }
    return { kind: "string", value: text, quoted: false };
}

function parseCompound(cursor: Cursor): SnbtValue {
    cursor.take(); // {
    const entries: [string, SnbtValue][] = [];
    for (;;) {
        cursor.skipSpace();
        if (cursor.peek() === "}") {
            cursor.take();
            return { kind: "compound", entries };
        }
        const key = parseValue(cursor);
        if (key.kind !== "string") {
            throw new Unparsable("compound key must be a string");
        }
        cursor.skipSpace();
        if (cursor.take() !== ":") {
            throw new Unparsable("expected ':' in compound");
        }
        entries.push([key.value, parseValue(cursor)]);
        cursor.skipSpace();
        const sep = cursor.peek();
        if (sep === ",") {
            cursor.take();
            continue;
        }
        if (sep === "}") {
            continue;
        }
        throw new Unparsable(`unexpected '${sep}' in compound`);
    }
}

function parseList(cursor: Cursor): SnbtValue {
    cursor.take(); // [
    // 数组前缀：[I; 1, 2] / [B; ...] / [L; ...]
    let arrayPrefix: string | undefined;
    cursor.skipSpace();
    if (/[BIL]/.test(cursor.peek()) && cursor.peek(1) === ";") {
        arrayPrefix = cursor.take().toLowerCase();
        cursor.take();
    }
    const items: SnbtValue[] = [];
    for (;;) {
        cursor.skipSpace();
        if (cursor.peek() === "]") {
            cursor.take();
            return { kind: "list", items, arrayPrefix };
        }
        items.push(parseValue(cursor));
        cursor.skipSpace();
        const sep = cursor.peek();
        if (sep === ",") {
            cursor.take();
            continue;
        }
        if (sep === "]") {
            continue;
        }
        throw new Unparsable(`unexpected '${sep}' in list`);
    }
}

/**
 * 打印成规范形。
 *
 * 数字后缀**原样保留**（`1b` 与 `1` 不同，那正是要比的类型信息），只统一引号与键序。
 */
function print(value: SnbtValue): string {
    switch (value.kind) {
        case "number":
            return `${formatNumber(value.value)}${value.suffix}`;
        case "string":
            // 字符串可能是**内嵌 JSON**（custom_name、lore 这类组件的持久形态就是 JSON 文本）。
            // 两侧都会把同一份 JSON 重新序列化，键序因此可能不同（Cubium 用 nlohmann 的
            // 字典序，原版保留输入顺序）。语义相同，比对前统一成字典序。
            return value.quoted ? quote(canonicaliseJsonText(value.value)) : value.value;
        case "list": {
            const inner = value.items.map((item) => print(item)).join(",");
            return value.arrayPrefix === undefined ? `[${inner}]` : `[${value.arrayPrefix.toUpperCase()};${inner}]`;
        }
        case "compound":
            return `{${value.entries
                .map(([key, item]) => [key, item] as [string, SnbtValue])
                .sort((left, right) => (left[0] < right[0] ? -1 : left[0] > right[0] ? 1 : 0))
                .map(([key, item]) => `${key}:${print(item)}`)
                .join(",")}}`;
    }
}

/**
 * 若字符串的内容是 JSON 对象/数组，按「键字典序、紧凑写法」重新序列化；否则原样返回。
 *
 * 用途：`custom_name` / `lore` 这类组件的持久形态就是**一段 JSON 文本**。两侧都会把它
 * 解析后再序列化，但 Cubium 走 nlohmann（键字典序），原版保留输入顺序——同一份文本组件
 * 因此会印出两种字节序。只重排键、不改任何值。
 */
function canonicaliseJsonText(text: string): string {
    const trimmed = text.trim();
    if (!trimmed.startsWith("{") && !trimmed.startsWith("[")) {
        return text;
    }
    try {
        const parsed: unknown = JSON.parse(trimmed);
        return JSON.stringify(sortJsonKeys(parsed));
    } catch {
        return text;
    }
}

/** 递归按字典序重排对象键（数组保持原顺序）。 */
function sortJsonKeys(value: unknown): unknown {
    if (Array.isArray(value)) {
        return value.map(sortJsonKeys);
    }
    if (value !== null && typeof value === "object") {
        const source = value as Record<string, unknown>;
        const sorted: Record<string, unknown> = {};
        for (const key of Object.keys(source).sort()) {
            sorted[key] = sortJsonKeys(source[key]);
        }
        return sorted;
    }
    return value;
}

/**
 * 数字的最短表示。
 *
 * 只做「去掉多余的 0」这一类等价改写：`4.0` → `4`、`0.750` → `0.75`。
 * 不改变量级（`1e-7` 不展开成 `0.0000001`），避免两个打印器对同一 f32 给出不同的
 * 十进制展开长度。
 */
function formatNumber(value: number): string {
    if (!Number.isFinite(value)) {
        return String(value);
    }
    if (Number.isInteger(value)) {
        return String(value);
    }
    return String(Number(value.toPrecision(15)));
}

function quote(text: string): string {
    let out = '"';
    for (const ch of text) {
        switch (ch) {
            case '"':
                out += '\\"';
                break;
            case "\\":
                out += "\\\\";
                break;
            case "\n":
                out += "\\n";
                break;
            case "\t":
                out += "\\t";
                break;
            case "\r":
                out += "\\r";
                break;
            default:
                out += ch;
        }
    }
    return `${out}"`;
}

/**
 * 规范形：解析后重新打印；解析失败时返回 `null`（调用方原样保留，照常报差异）。
 */
export function canonicalSnbt(text: string): string | null {
    try {
        const cursor = new Cursor(text);
        const value = parseValue(cursor);
        cursor.skipSpace();
        if (!cursor.done) {
            return null;
        }
        return print(value);
    } catch (error) {
        if (error instanceof Unparsable) {
            return null;
        }
        throw error;
    }
}