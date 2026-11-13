import assert from "node:assert/strict";
import test from "node:test";
import { missingViewChunks } from "../src/scenarios/movement.ts";

test("final view checks every required column", () => {
    const visited: Array<[number, number]> = [];
    assert.deepEqual(missingViewChunks(6.5, 32.5, 1, (x, z) => {
        visited.push([x, z]);
        return true;
    }), []);
    assert.equal(visited.length, 9);
    assert.deepEqual(visited[0], [-1, 1]);
    assert.deepEqual(visited.at(-1), [1, 3]);
});

test("final view detects a missing column", () => {
    assert.deepEqual(missingViewChunks(6.5, 32.5, 1, (x, z) => x !== 1 || z !== 2), [[1, 2]]);
});

test("negative positions use floor for the view center", () => {
    assert.deepEqual(missingViewChunks(-0.25, -16.25, 0, () => false), [[-1, -2]]);
});
