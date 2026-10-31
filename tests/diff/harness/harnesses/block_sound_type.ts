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
 * block_sound_type.ts — 每个方块的声音组：五个音效事件 + 音量 + 音调。
 *
 * 单元是方块 ID（默认状态），另有饰纹陶罐的裂纹状态。两侧都输出同样的七个字段，
 * 比的是玩家实际会听到的东西。声音事件名就是客户端查找音效用的注册键，名字对上
 * 就等于播的是同一个声音。
 */

import type { HarnessDeclaration } from "../core/types.ts";

/**
 * 比对前的规范化。
 *
 * 失败侧（方块未注册）也必须补齐七个字段，否则「原版有这个方块、本侧根本没有」
 * 会因为缺键被静默通过。
 */
export function normaliseBlockSoundType(_expected: unknown, actual: unknown): unknown {
    if (actual === null || typeof actual !== "object") {
        return actual;
    }
    const record = actual as Record<string, unknown>;
    return {
        ok: record["ok"] ?? null,
        break: record["break"] ?? null,
        step: record["step"] ?? null,
        place: record["place"] ?? null,
        hit: record["hit"] ?? null,
        fall: record["fall"] ?? null,
        volume: record["volume"] ?? null,
        pitch: record["pitch"] ?? null,
    };
}

normaliseBlockSoundType.canonicalising = true;

/** 能力声明。 */
export function blockSoundTypeDeclaration(): HarnessDeclaration {
    return {
        name: "block_sound_type",
        probe: "block_sound_type",
        oracle: "BlockSoundTypeOracle",
        key: "block",
        units: { kind: "listing" },
        input: (block: string): { block: string } => ({ block }),
        normalise: normaliseBlockSoundType,
        // oracle 要建原版服务端世界并持有区域锁：同组必须串行。
        exclusiveGroup: "loot-world",
    };
}