/* normalize.ts 的单元测试：只覆盖「等价改写」，任何放宽都是 bug。 */

import assert from "node:assert/strict";
import { test } from "node:test";

import type { FeedbackEntry } from "../observe/feedback.ts";
import { normalizeFeedback, normalizeText } from "./normalize.ts";

/** 构造一条反馈。 */
function entry(text: string): FeedbackEntry {
    return { text, position: "system", raw: null };
}

test("裁剪行尾空白并把 CRLF 统一为 LF", () => {
    assert.equal(normalizeText("a  \r\nb\t\r\n"), "a\nb");
});

test("去掉尾部空行但保留中间空行", () => {
    assert.equal(normalizeText("a\n\nb\n\n"), "a\n\nb");
});

test("整行 SNBT 规范化键序与空格", () => {
    assert.equal(normalizeText("{b: 1, a: 2}"), "{a:2,b:1}");
});

test("前缀 + SNBT 只规范化 SNBT 部分", () => {
    assert.equal(
        normalizeText("e2e_cmd has the following entity data: {Health: 20.0f, Glowing: 0b}"),
        "e2e_cmd has the following entity data: {Glowing:0b,Health:20f}",
    );
});

test("类型后缀保留：1b 与 1 不属于同一规范形", () => {
    assert.notEqual(normalizeText("{a:1b}"), normalizeText("{a:1}"));
});

test("非 SNBT 文本原样保留（不抹平数字格式差异）", () => {
    assert.equal(normalizeText("Teleported e2e_cmd to 0.500000, 100.000000, 0.500000").endsWith("0.500000"), true);
});

test("多行反馈按行规范化并用 LF 连接", () => {
    const text = normalizeFeedback([entry("{b:1,a:2}"), entry("plain  ")]);
    assert.equal(text, "{a:2,b:1}\nplain");
});

test("无反馈得到空串", () => {
    assert.equal(normalizeFeedback([]), "");
});
