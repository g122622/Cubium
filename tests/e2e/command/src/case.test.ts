/* case.ts 的单元测试：坐标替换与用例数据校验。 */

import assert from "node:assert/strict";
import { test } from "node:test";

import { boardOriginFor, parseCaseValue, substituteCoordinates } from "./case.ts";

const ORIGIN = { x: 10, y: 100, z: -20 };

test("坐标占位符支持正负偏移", () => {
    assert.equal(
        substituteCoordinates("/setblock {x} {y+1} {z-2} stone", ORIGIN),
        "/setblock 10 101 -22 stone",
    );
});

test("同一占位符可多次出现", () => {
    assert.equal(substituteCoordinates("/fill {x} {y} {z} {x} {y} {z} air", ORIGIN), "/fill 10 100 -20 10 100 -20 air");
});

test("非 x/y/z 的花括号内容原样保留", () => {
    assert.equal(substituteCoordinates("/say {hello} {x}", ORIGIN), "/say {hello} 10");
});

test("棋盘按序号沿 Z 轴排布并留出间隙", () => {
    assert.deepEqual(boardOriginFor(0, 100, 32, 8), { x: 0, y: 100, z: 0 });
    assert.deepEqual(boardOriginFor(2, 100, 32, 8), { x: 0, y: 100, z: 80 });
});

test("合法用例通过校验", () => {
    const parsed = parseCaseValue(
        {
            id: "demo/case",
            title: "示例",
            category: "demo",
            setup: ["/say setup"],
            steps: [{ command: "/say hi" }],
            probes: [{ key: "p", command: "/data get block 0 0 0" }],
            knownDiffs: [{ path: "steps[0].feedback", reason: "已知" }],
        },
        "demo.json",
    );
    assert.equal(parsed.steps[0]?.command, "/say hi");
    assert.equal(parsed.knownDiffs[0]?.path, "steps[0].feedback");
});

test("缺字段直接报错", () => {
    assert.throws(() => parseCaseValue({ id: "x" }, "bad.json"), /缺少字符串字段 title/);
});

test("knownDiff 必须精确到步骤或探针", () => {
    assert.throws(
        () =>
            parseCaseValue(
                {
                    id: "demo/case",
                    title: "示例",
                    category: "demo",
                    setup: [],
                    steps: [],
                    probes: [],
                    knownDiffs: [{ path: "case", reason: "整条豁免" }],
                },
                "bad.json",
            ),
        /knownDiffs\.path 必须是/,
    );
});
