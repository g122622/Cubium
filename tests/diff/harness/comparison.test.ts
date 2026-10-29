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

import assert from "node:assert/strict";
import { test } from "node:test";
import { projectDeclaredFields } from "./comparison.ts";

const equal = (expected: unknown, actual: unknown): boolean =>
    JSON.stringify(projectDeclaredFields(expected, expected)) === JSON.stringify(projectDeclaredFields(expected, actual));

test("对象额外字段不影响比较，数组内部仍按声明字段比较", () => {
    assert.ok(equal({ nodes: [{ id: 0 }] }, { nodes: [{ duration: 20, id: 0 }], debug: true }));
    assert.ok(!equal({ nodes: [{ id: 0 }] }, { nodes: [{ id: 1 }] }));
    assert.ok(!equal({ nodes: [{ id: 0 }] }, { nodes: [{}] }));
});

test("隐藏节点、回调次数和操作轨迹不能通过数组前缀投影被忽略", () => {
    assert.ok(!equal({ nodes: [{ id: 0 }] }, { nodes: [{ id: 0 }, { id: 1 }] }));
    assert.ok(!equal({ nodes: [{ id: 0 }] }, { nodes: [] }));
    assert.ok(!equal({ calls: [] }, { calls: [20] }));
    assert.ok(!equal({ trace: [{ result: true }] }, { trace: [{ result: true }, { result: false }] }));
});

test("默认有序：同样元素换顺序仍算差异", () => {
    assert.ok(
        !equal(
            { levels: [{ attributes: [{ slot: "head" }, { slot: "feet" }] }] },
            { levels: [{ attributes: [{ slot: "feet" }, { slot: "head" }] }] },
        ),
    );
});

test("显式声明为无序的路径：同样元素换顺序算一致，元素本身仍逐一比对", () => {
    const unordered = (expected: unknown, actual: unknown): boolean =>
        JSON.stringify(projectDeclaredFields(expected, expected, ["levels/attributes"])) ===
        JSON.stringify(projectDeclaredFields(expected, actual, ["levels/attributes"]));
    assert.ok(
        unordered(
            { levels: [{ attributes: [{ slot: "head" }, { slot: "feet" }] }] },
            { levels: [{ attributes: [{ slot: "feet" }, { slot: "head", extra: 1 }] }] },
        ),
    );
    assert.ok(
        !unordered(
            { levels: [{ attributes: [{ slot: "head" }, { slot: "feet" }] }] },
            { levels: [{ attributes: [{ slot: "head" }, { slot: "chest" }] }] },
        ),
    );
    // 无序不等于无约束：元素数量仍必须一致。
    assert.ok(
        !unordered(
            { levels: [{ attributes: [{ slot: "head" }] }] },
            { levels: [{ attributes: [{ slot: "head" }, { slot: "feet" }] }] },
        ),
    );
    // 未声明的父路径数组保持有序。
    assert.ok(!unordered({ levels: [{ attributes: [] }, { attributes: [] }] }, { levels: [{ attributes: [] }] }));
});
