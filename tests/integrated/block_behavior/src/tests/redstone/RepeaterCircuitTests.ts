// 中继器实际电路：facing 指向输入端，输出端在反方向。
// Ref: docs/minecraft-wiki-source/minecraft_wiki/tech_红石中继器.txt#输入输出、延迟信号、锁存信号
import type { Test } from "@minecraft/server-gametest";
import { assertState, assertType, createLeverSwitch, HORIZONTAL_INPUTS, offset, registerCircuitTest, setCircuitBlock } from "../../utils/block/redstone.js";

const DIODE = { x: 7, y: 2, z: 6 };
const INPUT = { x: 8, y: 2, z: 6 };
const OUTPUT = { x: 6, y: 2, z: 6 };
const LOCK = { x: 7, y: 2, z: 5 };
const LOCK_SOURCE = { x: 7, y: 2, z: 4 };

function repeater(test: Test, delay: number): void {
    setCircuitBlock(test, "repeater", DIODE, `facing=east,delay=${delay}`);
    test.setBlockType("minecraft:redstone_wire", OUTPUT);
}

export function registerRepeaterCircuitTests(): void {
    // 串联 4 档与 1 档中继器，上升沿延迟应为 10 游戏刻。
    registerCircuitTest("repeater", "repeater_series_delay", test => {
        repeater(test, 4);
        test.setBlockType("minecraft:air", OUTPUT);
        setCircuitBlock(test, "repeater", OUTPUT, "facing=east,delay=1");
        test.runAtTickTime(10, () => test.setBlockType("minecraft:redstone_block", INPUT));
        test.runAtTickTime(19, () => assertState(test, OUTPUT, "powered", false));
        test.runAtTickTime(20, () => { assertState(test, OUTPUT, "powered", true); test.succeed(); });
    }, 35);

    // 低强度输入必须恢复成 15，且上游断电后输出也要撤销。
    registerCircuitTest("repeater", "repeater_regenerates_weak_input", test => {
        repeater(test, 1);
        test.setBlockType("minecraft:redstone_wire", INPUT);
        test.setBlockType("minecraft:redstone_wire", { x: 9, y: 2, z: 6 });
        test.setBlockType("minecraft:redstone_wire", { x: 10, y: 2, z: 6 });
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", { x: 11, y: 2, z: 6 }));
        test.runAtTickTime(20, () => {
            assertState(test, INPUT, "power", 13);
            assertState(test, OUTPUT, "power", 15);
            test.setBlockType("minecraft:air", { x: 11, y: 2, z: 6 });
        });
        test.runAtTickTime(40, () => { assertState(test, OUTPUT, "power", 0); test.succeed(); });
    }, 55);

    // 二极管强充能输出方块，方块另一侧的红石线须接收满强度。
    registerCircuitTest("repeater", "repeater_output_through_conductor", test => {
        repeater(test, 1);
        test.setBlockType("minecraft:stone", OUTPUT);
        test.setBlockType("minecraft:redstone_wire", { x: 5, y: 2, z: 6 });
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", INPUT));
        test.runAtTickTime(20, () => { assertState(test, { x: 5, y: 2, z: 6 }, "power", 15); test.succeed(); });
    }, 35);

    // 四向逐一验证输入端激活、输出端满强度与移除电源后的撤销。
    for (const direction of HORIZONTAL_INPUTS) {
        registerCircuitTest("repeater", `repeater_direction_${direction.name}`, test => {
            setCircuitBlock(test, "repeater", DIODE, `facing=${direction.name}`);
            const output = offset(DIODE, direction, -1);
            test.setBlockType("minecraft:redstone_wire", output);
            test.runAtTickTime(10, () => test.setBlockType("minecraft:redstone_block", offset(DIODE, direction, 1)));
            test.runAtTickTime(20, () => {
                assertState(test, DIODE, "powered", true);
                assertState(test, output, "power", 15);
                test.setBlockType("minecraft:air", offset(DIODE, direction, 1));
            });
            test.runAtTickTime(30, () => {
                assertState(test, DIODE, "powered", false);
                assertState(test, output, "power", 0);
                test.succeed();
            });
        }, 45);
    }

    // 每档分别验证上升沿与下降沿边界；检查点在世界计划刻执行之后。
    for (const delay of [1, 2, 3, 4]) {
        registerCircuitTest("repeater", `repeater_delay_${delay}`, test => {
            repeater(test, delay);
            test.runAtTickTime(10, () => test.setBlockType("minecraft:redstone_block", INPUT));
            test.runAtTickTime(10 + delay * 2 - 1, () => assertState(test, DIODE, "powered", false));
            test.runAtTickTime(10 + delay * 2, () => assertState(test, DIODE, "powered", true));
            test.runAtTickTime(30, () => test.setBlockType("minecraft:air", INPUT));
            test.runAtTickTime(30 + delay * 2 - 1, () => assertState(test, DIODE, "powered", true));
            test.runAtTickTime(30 + delay * 2, () => {
                assertState(test, DIODE, "powered", false);
                test.succeed();
            });
        }, 50);
    }

    // 4 档中继器把 2 游戏刻的正脉冲扩展为 8 游戏刻。
    registerCircuitTest("repeater", "repeater_stretches_short_pulse", test => {
        repeater(test, 4);
        test.runAtTickTime(10, () => test.setBlockType("minecraft:redstone_block", INPUT));
        test.runAtTickTime(12, () => test.setBlockType("minecraft:air", INPUT));
        test.runAtTickTime(17, () => assertState(test, DIODE, "powered", false));
        test.runAtTickTime(18, () => assertState(test, DIODE, "powered", true));
        test.runAtTickTime(25, () => assertState(test, DIODE, "powered", true));
        test.runAtTickTime(26, () => { assertState(test, DIODE, "powered", false); test.succeed(); });
    }, 40);

    // 小于延迟的负脉冲不会中断已开启的输出。
    registerCircuitTest("repeater", "repeater_filters_short_negative_pulse", test => {
        repeater(test, 4);
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", INPUT));
        test.runAtTickTime(20, () => { assertState(test, DIODE, "powered", true); test.setBlockType("minecraft:air", INPUT); });
        test.runAtTickTime(22, () => test.setBlockType("minecraft:redstone_block", INPUT));
        for (let tick = 23; tick <= 35; tick++) test.runAtTickTime(tick, () => assertState(test, DIODE, "powered", true));
        test.runAtTickTime(36, () => test.succeed());
    }, 50);

    for (const initial of [false, true]) {
        // 锁存保持原输出，解除侧面信号后按档位延迟重新采样主输入。
        registerCircuitTest("repeater", `repeater_latch_${initial ? "on" : "off"}`, test => {
            repeater(test, 2);
            setCircuitBlock(test, "repeater", LOCK, "facing=north");
            if (initial) test.setBlockType("minecraft:redstone_block", INPUT);
            test.runAtTickTime(12, () => test.setBlockType("minecraft:redstone_block", LOCK_SOURCE));
            test.runAtTickTime(20, () => {
                assertState(test, DIODE, "locked", true);
                assertState(test, DIODE, "powered", initial);
                test.setBlockType(initial ? "minecraft:air" : "minecraft:redstone_block", INPUT);
            });
            test.runAtTickTime(30, () => {
                assertState(test, DIODE, "powered", initial);
                test.setBlockType("minecraft:air", LOCK_SOURCE);
            });
            test.runAtTickTime(40, () => {
                assertState(test, DIODE, "locked", false);
                assertState(test, DIODE, "powered", !initial);
                test.succeed();
            });
        }, 55);
    }

    // 侧面的普通电源不锁存；输出端信号也不能反向进入主输入。
    for (const source of [LOCK, OUTPUT]) {
        registerCircuitTest("repeater", `repeater_ignores_${source === LOCK ? "side_source" : "reverse_input"}`, test => {
            setCircuitBlock(test, "repeater", DIODE, "facing=east");
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", source));
            test.runAtTickTime(20, () => {
                assertState(test, DIODE, "powered", false);
                assertState(test, DIODE, "locked", false);
                test.succeed();
            });
        }, 35);
    }

    // 输入来自弱充能实心方块，不能只读取该方块本身的信号源接口。
    registerCircuitTest("repeater", "repeater_reads_powered_conductor", test => {
        repeater(test, 1);
        test.setBlockType("minecraft:stone", INPUT);
        const toggle = createLeverSwitch(test, { x: 8, y: 3, z: 6 }, "face=floor");
        test.runAtTickTime(5, toggle);
        test.runAtTickTime(15, () => { assertState(test, OUTPUT, "power", 15); test.succeed(); });
    }, 30);

    // 下方支撑移除后元件应掉落，不能成为悬空二极管。
    registerCircuitTest("repeater", "repeater_loses_support", test => {
        repeater(test, 1);
        test.runAtTickTime(5, () => test.setBlockType("minecraft:air", { x: 7, y: 1, z: 6 }));
        test.runAtTickTime(10, () => { assertType(test, DIODE, "air"); test.succeed(); });
    }, 25);
}
