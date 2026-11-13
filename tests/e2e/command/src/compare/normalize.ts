/*
 * normalize.ts — 两侧反馈文本的规范形。
 *
 * 归一化只做**等价改写**，不做任何「差不多就行」的放宽：
 *   - 行尾空白与换行风格统一（两侧的聊天组件都会带尾随空格）；
 *   - 反馈里的 SNBT 片段按 `canonicalSnbt` 重排（键序、引号、数字写法），
 *     因为两侧由不同打印器产出同一份 NBT；
 *   - 除此之外一切原样比对——数字格式、单复数、坐标精度都是真实差异，
 *     交给 knownDiff 显式登记，而不是在这里悄悄抹平。
 *
 * 刻意**不做** NFKC：两侧消费同一份 en_us.json，任何字符形态差异都值得报出来。
 */

import type { FeedbackEntry } from "../observe/feedback.ts";
import { canonicalSnbt } from "../../../../diff/harness/core/snbt.ts";

/** 行内 `前缀: {SNBT}` 的分割点：左侧不含花括号/方括号。 */
const SNBT_TAIL = /^([^{[]*?:\s*)([{[][\s\S]*)$/;

/**
 * 把若干条反馈拼成一段用于比对的文本。
 *
 * @param entries 一次步骤/探针采集到的反馈。
 * @returns 规范形文本；没有反馈时为空串。
 */
export function normalizeFeedback(entries: readonly FeedbackEntry[]): string {
    return normalizeText(entries.map((entry) => entry.text).join("\n"));
}

/**
 * 规范化一段反馈文本。
 *
 * @param text 原始文本（可能是多行）。
 * @returns 规范形。
 */
export function normalizeText(text: string): string {
    const lines = text.replace(/\r\n/g, "\n").split("\n").map(normalizeLine);
    // 去掉尾部空行：多行 NBT 的末尾换行数两侧不一定一致。
    while (lines.length > 0 && lines[lines.length - 1] === "") {
        lines.pop();
    }
    return lines.join("\n");
}

/**
 * 解析错误的「光标上下文」行统一成固定标记。
 *
 * 这一行是客户端帮助信息（输入前缀 + `<--[HERE]`），其截断/省略规则由 Brigadier 与
 * 客户端渲染共同决定，与命令本身是否被正确解析无关；错误消息本身（前一行）仍严格比对。
 */
const PARSE_CONTEXT_MARKER = "<--[HERE]";

/** `/scoreboard objectives list` 的目标列举行：前缀 + 逗号分隔的目标名。 */
const OBJECTIVE_LIST = /^(There are \d+ objective\(s\): )(.*)$/;

/** 单行规范形：先去行尾空白，再尝试把其中的 SNBT 片段规范化。 */
function normalizeLine(line: string): string {
    const trimmed = line.replace(/\s+$/, "");
    if (trimmed.length === 0) {
        return "";
    }
    if (trimmed.includes(PARSE_CONTEXT_MARKER)) {
        return "<parse-context>";
    }

    // 目标列举是有序无关的集合：两侧的存储容器不同（vanilla 的哈希序 / Cubium 的创建序），
    // 顺序本身没有语义，排序后再比对。
    const objectiveList = OBJECTIVE_LIST.exec(trimmed);
    if (objectiveList !== null) {
        const items = (objectiveList[2] ?? "")
            .split(", ")
            .map((item) => item.trim())
            .filter((item) => item.length > 0)
            .sort();
        return `${objectiveList[1] ?? ""}${items.join(", ")}`;
    }

    // 整行就是 SNBT（`data get` 的短输出、`storage` 的 dump）。
    const whole = canonicalSnbt(trimmed);
    if (whole !== null) {
        return whole;
    }

    // `CommandSource has the following entity data: {...}` 这类「前缀 + SNBT」。
    const tail = SNBT_TAIL.exec(trimmed);
    if (tail !== null) {
        const prefix = tail[1] ?? "";
        const snbt = tail[2] ?? "";
        const canonical = canonicalSnbt(snbt);
        if (canonical !== null) {
            return `${prefix}${canonical}`;
        }
    }

    return trimmed;
}
