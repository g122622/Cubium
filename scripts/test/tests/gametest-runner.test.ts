import assert from "node:assert/strict";
import { performance } from "node:perf_hooks";
import test from "node:test";
import { parseJunitXml, generateJunitXml } from "../run-gametests.ts";
import { runProcess } from "../gametest-process.ts";

test("mixed self-closing and paired cases retain failures, errors and skips", () => {
    const records = parseJunitXml(`<testsuites><testsuite>
        <testcase name="pass" time="1.25"/>
        <testcase name="failure"><failure message="bad &amp; worse"/></testcase>
        <testcase name="pending"><skipped message="not started"/></testcase>
        <testcase name="active"><error message="interrupted"/></testcase>
    </testsuite></testsuites>`);
    assert.deepEqual(records.map((r) => [r.name, r.failed, r.skipped, r.errored]), [
        ["pass", false, false, false], ["failure", true, false, false],
        ["pending", false, true, false], ["active", true, false, true],
    ]);
    assert.equal(records[1].message, "bad & worse");
    const xml = generateJunitXml(records, { rerunPassed: [], rerunFailed: [] });
    assert.match(xml, /failures="1" errors="1" skipped="1"/);
    assert.deepEqual(parseJunitXml(xml), records);
});

test("ordinary failed child exits without a timeout", async () => {
    const result = await runProcess(process.execPath, ["-e", "process.exit(1)"], performance.now() + 3000);
    assert.equal(result.code, 1);
    assert.equal(result.timedOut, false);
    assert.equal(result.signal, null);
});

test("blocked child is forcibly terminated by elapsed time", async () => {
    const started = performance.now();
    const result = await runProcess(process.execPath, ["-e", "while (true) {}"], started + 250);
    assert.equal(result.timedOut, true);
    assert.notEqual(result.code, 0);
    assert.ok(performance.now() - started < 3000, "hung process was not terminated promptly");
});

test("an expired shared deadline cannot give a rerun a fresh budget", async () => {
    const result = await runProcess(process.execPath, ["-e", "setInterval(() => {}, 1000)"], performance.now() - 1);
    assert.equal(result.timedOut, true);
    assert.notEqual(result.code, 0);
});
