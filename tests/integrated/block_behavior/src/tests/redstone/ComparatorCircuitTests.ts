// Ref: docs/minecraft-wiki-source/minecraft_wiki/mechanism_红石比较器.txt#输入输出、比较模式、减法模式、检测方块
import type { Test } from "@minecraft/server-gametest";
import { assertState, assertType, registerCircuitTest, setCircuitBlock } from "../../utils/block/redstone.js";

const POS = { x: 7, y: 2, z: 6 };
const INPUT = { x: 8, y: 2, z: 6 };
const OUTPUT = { x: 6, y: 2, z: 6 };
const SIDE = { x: 7, y: 2, z: 5 };

function comparator(test: Test, mode: string): void {
    setCircuitBlock(test, "comparator", POS, `facing=east,mode=${mode}`);
    test.setBlockType("minecraft:redstone_wire", OUTPUT);
}

export function registerComparatorCircuitTests(): void {
    // 容器模拟信号覆盖后方导体已有的低于 15 的信号，不能与其取最大值。
    registerCircuitTest("comparator_container_overrides_conductor_signal", test => {
        comparator(test, "compare");
        test.setBlockType("minecraft:stone", INPUT);
        setCircuitBlock(test, "comparator", { x: 8, y: 2, z: 5 }, "facing=north");
        test.runAtTickTime(5, () => {
            setCircuitBlock(test, "composter", { x: 9, y: 2, z: 6 }, "level=3");
            setCircuitBlock(test, "composter", { x: 8, y: 2, z: 4 }, "level=7");
        });
        test.runAtTickTime(20, () => {
            assertState(test, { x: 8, y: 2, z: 5 }, "powered", true);
            assertState(test, OUTPUT, "power", 3);
            test.succeed();
        });
    }, 35);

    // 两侧模拟输入取最大值，不能求和或只使用某一侧。
    registerCircuitTest("comparator_uses_maximum_side", test => {
        comparator(test, "subtract");
        setCircuitBlock(test, "comparator", SIDE, "facing=north");
        setCircuitBlock(test, "comparator", { x: 7, y: 2, z: 7 }, "facing=south");
        test.runAtTickTime(5, () => {
            test.setBlockType("minecraft:redstone_block", INPUT);
            setCircuitBlock(test, "composter", { x: 7, y: 2, z: 4 }, "level=3");
            setCircuitBlock(test, "composter", { x: 7, y: 2, z: 8 }, "level=7");
        });
        test.runAtTickTime(20, () => { assertState(test, OUTPUT, "power", 8); test.succeed(); });
    }, 35);

    // 侧输入撤销时输出由 0 恢复；主输入撤销时输出必须归零。
    registerCircuitTest("comparator_side_and_main_removal", test => {
        comparator(test, "compare");
        test.runAtTickTime(5, () => {
            setCircuitBlock(test, "composter", INPUT, "level=7");
            test.setBlockType("minecraft:redstone_block", SIDE);
        });
        test.runAtTickTime(15, () => { assertState(test, OUTPUT, "power", 0); test.setBlockType("minecraft:air", SIDE); });
        test.runAtTickTime(25, () => { assertState(test, OUTPUT, "power", 7); test.setBlockType("minecraft:air", INPUT); });
        test.runAtTickTime(35, () => { assertState(test, OUTPUT, "power", 0); test.succeed(); });
    }, 50);

    // 实际交互改变工作模式时，输出值也必须更新。
    registerCircuitTest("comparator_mode_toggle_updates_output", test => {
        comparator(test, "compare");
        setCircuitBlock(test, "comparator", SIDE, "facing=north");
        const player = test.spawnSimulatedPlayer({ x: 8, y: 2, z: 8 }, "comparator_switch");
        test.runAtTickTime(5, () => {
            test.setBlockType("minecraft:redstone_block", INPUT);
            setCircuitBlock(test, "composter", { x: 7, y: 2, z: 4 }, "level=7");
        });
        test.runAtTickTime(20, () => { assertState(test, OUTPUT, "power", 15); test.assert(player.interactWithBlock(POS), "Comparator interaction failed"); });
        test.runAtTickTime(30, () => { assertState(test, OUTPUT, "power", 8); test.succeed(); });
    }, 45);
    // 堆肥桶提供稳定的模拟输入，覆盖 0 与非满强度信号，不能把比较器实现成布尔中继器。
    for (const level of [0, 1, 3, 7, 8]) {
        registerCircuitTest(`comparator_analog_${level}`, test => {
            comparator(test, "compare");
            test.runAtTickTime(5, () => setCircuitBlock(test, "composter", INPUT, `level=${level}`));
            test.runAtTickTime(15, () => { assertState(test, OUTPUT, "power", level); test.succeed(); });
        }, 30);
    }

    // 两侧输入取最大值；红石块和红石线均属于有效侧输入。
    for (const mode of ["compare", "subtract"]) {
        registerCircuitTest(`comparator_${mode}_equal_side`, test => {
            comparator(test, mode);
            test.runAtTickTime(5, () => {
                test.setBlockType("minecraft:redstone_block", INPUT);
                test.setBlockType("minecraft:redstone_block", SIDE);
            });
            test.runAtTickTime(15, () => { assertState(test, OUTPUT, "power", mode === "compare" ? 15 : 0); test.succeed(); });
        }, 30);
        registerCircuitTest(`comparator_${mode}_stronger_side`, test => {
            comparator(test, mode);
            test.runAtTickTime(5, () => {
                setCircuitBlock(test, "composter", INPUT, "level=7");
                test.setBlockType("minecraft:redstone_block", SIDE);
            });
            test.runAtTickTime(15, () => { assertState(test, OUTPUT, "power", 0); test.succeed(); });
        }, 30);
    }

    registerCircuitTest("comparator_subtracts_wire_side", test => {
        comparator(test, "subtract");
        test.setBlockType("minecraft:redstone_wire", SIDE);
        test.setBlockType("minecraft:redstone_wire", { x: 7, y: 2, z: 4 });
        test.runAtTickTime(5, () => {
            test.setBlockType("minecraft:redstone_block", INPUT);
            test.setBlockType("minecraft:redstone_block", { x: 7, y: 2, z: 3 });
        });
        test.runAtTickTime(20, () => {
            assertState(test, SIDE, "power", 14);
            assertState(test, OUTPUT, "power", 1);
            test.succeed();
        });
    }, 35);

    // 模拟输出改变但 powered 布尔始终为真，仍须更新方块实体并通知下游。
    registerCircuitTest("comparator_updates_nonzero_analog", test => {
        comparator(test, "compare");
        test.runAtTickTime(5, () => setCircuitBlock(test, "composter", INPUT, "level=3"));
        test.runAtTickTime(15, () => { assertState(test, OUTPUT, "power", 3); setCircuitBlock(test, "composter", INPUT, "level=7"); });
        test.runAtTickTime(25, () => { assertState(test, OUTPUT, "power", 7); setCircuitBlock(test, "composter", INPUT, "level=1"); });
        test.runAtTickTime(35, () => { assertState(test, OUTPUT, "power", 1); test.succeed(); });
    }, 50);

    // 侧面比较器提供有效信号，但不会像中继器一样锁住比较器。
    registerCircuitTest("comparator_side_diode_does_not_lock", test => {
        comparator(test, "subtract");
        setCircuitBlock(test, "comparator", SIDE, "facing=north");
        test.runAtTickTime(5, () => {
            test.setBlockType("minecraft:redstone_block", INPUT);
            setCircuitBlock(test, "composter", { x: 7, y: 2, z: 4 }, "level=3");
        });
        test.runAtTickTime(20, () => { assertState(test, OUTPUT, "power", 12); test.succeed(); });
    }, 35);

    // 主输入可以隔一块导体读取容器。
    registerCircuitTest("comparator_reads_through_conductor", test => {
        comparator(test, "compare");
        test.setBlockType("minecraft:stone", INPUT);
        test.runAtTickTime(5, () => setCircuitBlock(test, "composter", { x: 9, y: 2, z: 6 }, "level=7"));
        test.runAtTickTime(20, () => {
            assertState(test, OUTPUT, "power", 7);
            test.setBlockType("minecraft:air", { x: 9, y: 2, z: 6 });
        });
        test.runAtTickTime(30, () => { assertState(test, OUTPUT, "power", 0); test.succeed(); });
    }, 45);

    // 固定 2 游戏刻延迟：计划刻前不得根据新输入实时重算输出。
    registerCircuitTest("comparator_keeps_output_until_tick", test => {
        comparator(test, "compare");
        test.runAtTickTime(5, () => setCircuitBlock(test, "composter", INPUT, "level=3"));
        test.runAtTickTime(15, () => { assertState(test, OUTPUT, "power", 3); setCircuitBlock(test, "composter", INPUT, "level=7"); });
        test.runAtTickTime(16, () => assertState(test, OUTPUT, "power", 3));
        test.runAtTickTime(20, () => { assertState(test, OUTPUT, "power", 7); test.succeed(); });
    }, 35);

    registerCircuitTest("comparator_loses_support", test => {
        comparator(test, "compare");
        test.runAtTickTime(5, () => test.setBlockType("minecraft:air", { x: 7, y: 1, z: 6 }));
        test.runAtTickTime(10, () => { assertType(test, POS, "air"); test.succeed(); });
    }, 25);
}
