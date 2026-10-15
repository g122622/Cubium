import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { parseOptions } from "../src/config.ts";
import { readMetrics, recoverySummary, statistics, validateMetrics, type TickSample } from "../src/metrics.ts";

/** 构造包含真实采样时间关系的输入；不启动服务器或机器人。 */
function sample(timeMs: number, tickMs: number, rssMiB: number, commitMiB: number): TickSample {
    return { timeMs, tickMs, intervalMs: 50, memorySampleMs: timeMs - 1,
        rssMiB, commitMiB, players: 1, chunkHolders: 361, loadedChunks: 289, entities: 1 };
}

test("reports tick execution separately from interval and uses nearest-rank percentiles", () => {
    const values = Array.from({ length: 100 }, (_, index) => index + 1);
    assert.equal(statistics(values)?.p95, 95);
    assert.equal(statistics(values)?.p50, 50);
    assert.equal(statistics([]), null);
    const phases = [{ name: "baseline", startMs: 1000, endMs: 3000 }, { name: "recovery", startMs: 4000, endMs: 6000 }];
    const result = recoverySummary(phases, [sample(1100, 20, 100, 80), sample(5900, 45, 110, 100)], 1.2, 50)!;
    assert.equal(result.finalWindow.tickMs?.p95, 45);
    assert.equal(result.finalWindow.intervalMs?.p95, 50);
    assert.equal(result.tickRecovered, true);
    assert.equal(result.rssRecovered, true);
    assert.equal(result.commitRecovered, false);
});

test("recovery compares only the final five seconds and preserves missing data", () => {
    const phases = [{ name: "baseline", startMs: 1000, endMs: 3000 }, { name: "recovery", startMs: 4000, endMs: 19000 }];
    const result = recoverySummary(phases, [sample(1100, 20, 100, 80), sample(4100, 200, 500, 400), sample(18500, 25, 105, 85)], 1.2, 50)!;
    assert.equal(result.finalWindow.rssMiB?.p50, 105);
    assert.equal(result.tickRecovered, true);
    assert.equal(recoverySummary(phases, [], 1.2, 50)?.tickRecovered, null);
    assert.equal(recoverySummary([], [], 1.2, 50), null);
});

test("missing stage coverage and a stalled memory sampler cannot silently pass", () => {
    const phases = [{ name: "baseline", startMs: 1000, endMs: 11000 }];
    assert.throws(() => validateMetrics(phases, []), /No server metrics/);
    assert.throws(() => validateMetrics(phases, [sample(1001, 20, 100, 80)]), /Missing metrics at end/);
    const stale = [sample(1001, 20, 100, 80), { ...sample(10999, 20, 100, 80), memorySampleMs: 1000 }];
    assert.throws(() => validateMetrics(phases, stale), /Memory sampler stalled/);
    assert.doesNotThrow(() => validateMetrics(phases, [sample(1001, 20, 100, 80), sample(10999, 20, 100, 80)]));
});

test("a long tick spanning a phase boundary remains visible and is not mistaken for lost samples", () => {
    const phases = [{ name: "recovery", startMs: 1000, endMs: 16000 }];
    const samples = [sample(1001, 20, 100, 80), sample(21000, 21000, 200, 180)];
    assert.doesNotThrow(() => validateMetrics(phases, samples));
    const result = recoverySummary([{ name: "baseline", startMs: 0, endMs: 900 }, ...phases], samples, 1.2, 50)!;
    assert.equal(result.finalWindow.tickMs?.max, 21000);
    assert.equal(result.tickRecovered, false);
    assert.equal(result.finalWindow.rssMiB, null);
});

test("partial CSV tail is discarded while malformed complete rows fail", () => {
    const directory = fs.mkdtempSync(path.join(os.tmpdir(), "cubium-stress-metrics-"));
    const file = path.join(directory, "metrics.csv");
    const header = "time_ms,tick_ms,interval_ms,memory_sample_ms,rss_mib,commit_mib,players,chunk_holders,loaded_chunks,entities\n";
    try {
        fs.writeFileSync(file, header + "1000,20,50,999,100,80,1,361,289,1\n1050,30");
        assert.equal(readMetrics(file).length, 1);
        assert.equal(readMetrics(file)[0].rssMiB, 100);
        fs.writeFileSync(file, header + "1000,20,50,,,,1,361,289,1\n");
        assert.equal(readMetrics(file)[0].rssMiB, null);
        fs.writeFileSync(file, header + "1000,broken,50,999,100,80,1,361,289,1\n");
        assert.throws(() => readMetrics(file), /Non-numeric/);
    } finally {
        fs.unlinkSync(file);
        fs.rmdirSync(directory);
    }
});

test("CLI expands all speed tiers and rejects invalid workloads", () => {
    const options = parseOptions([])!;
    assert.equal(options.world, "default");
    assert.equal(options.skipMovementSpeedCheck, false);
    assert.equal(parseOptions(["--skip-movement-speed-check"])?.skipMovementSpeedCheck, true);
    assert.deepEqual(options.speeds, [5.6, 16, 32, 64, 128, 256]);
    assert.equal(options.moveSeconds, 60);
    assert.equal(options.recoverySeconds, 15);
    assert.equal(options.viewDistance, 8);
    assert.deepEqual(parseOptions(["--speeds=4.3,512", "--case=movement"])?.speeds, [4.3, 512]);
    assert.equal(parseOptions(["--join-rate=0"])?.joinRate, 0);
    assert.throws(() => parseOptions(["--speeds=32,NaN"]), /Invalid speeds/);
    assert.throws(() => parseOptions(["--players=0"]), /Invalid value/);
    assert.throws(() => parseOptions(["--case=unknown"]), /Unknown scenario/);
});
