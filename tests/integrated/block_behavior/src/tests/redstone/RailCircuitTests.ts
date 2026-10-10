// Ref: docs/minecraft-wiki-source/minecraft_wiki/tech_动力铁轨.txt#红石元件
// Ref: docs/minecraft-wiki-source/minecraft_wiki/tech_激活铁轨.txt#红石元件
// Ref: docs/minecraft-wiki-source/minecraft_wiki/tech_铁轨.txt#放置
import type { Test } from "@minecraft/server-gametest";
import { assertState, assertType, registerCircuitTest, setCircuitBlock } from "../../utils/block/redstone.js";

function track(test: Test, type: string, count: number): void {
    for (let x = 2; x < 2 + count; x++) setCircuitBlock(test, type, { x, y: 2, z: 5 }, "shape=east_west");
}

export function registerRailCircuitTests(): void {
    for (const type of ["powered_rail", "activator_rail"]) {
        // 斜坡两端都能传电；撤销电源后的传播也要跨越高度差。
        for (const sourceEnd of ["low", "high"]) {
            registerCircuitTest(`${type}_slope_${sourceEnd}_source`, test => {
                for (let x = 4; x <= 6; x++) setCircuitBlock(test, type, { x, y: 2, z: 5 }, "shape=east_west");
                for (let x = 7; x <= 10; x++) {
                    test.setBlockType("minecraft:stone", { x, y: 2, z: 5 });
                    setCircuitBlock(test, type, { x, y: 3, z: 5 }, "shape=east_west");
                }
                const source = sourceEnd === "low" ? { x: 4, y: 2, z: 4 } : { x: 10, y: 3, z: 4 };
                test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", source));
                test.runAtTickTime(20, () => {
                    assertState(test, { x: 6, y: 2, z: 5 }, "shape", "ascending_east");
                    assertState(test, { x: 4, y: 2, z: 5 }, "powered", true);
                    assertState(test, { x: 10, y: 3, z: 5 }, "powered", true);
                    test.setBlockType("minecraft:air", source);
                });
                test.runAtTickTime(35, () => {
                    assertState(test, { x: 4, y: 2, z: 5 }, "powered", false);
                    assertState(test, { x: 10, y: 3, z: 5 }, "powered", false);
                    test.succeed();
                });
            }, 50);
        }
        // 直接充能轨本体之外最多再传 8 格；第 10 节轨道必须保持断电。
        registerCircuitTest(`${type}_eight_block_limit`, test => {
            track(test, type, 11);
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", { x: 2, y: 2, z: 4 }));
            test.runAtTickTime(15, () => {
                for (let x = 2; x <= 10; x++) assertState(test, { x, y: 2, z: 5 }, "powered", true);
                for (let x = 11; x <= 12; x++) assertState(test, { x, y: 2, z: 5 }, "powered", false);
                test.succeed();
            });
        }, 30);

        // 已激活的轨道不是新的无限电源，移除真正电源后整段必须断电。
        registerCircuitTest(`${type}_source_removal`, test => {
            track(test, type, 7);
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", { x: 2, y: 2, z: 4 }));
            test.runAtTickTime(15, () => { assertState(test, { x: 8, y: 2, z: 5 }, "powered", true); test.setBlockType("minecraft:air", { x: 2, y: 2, z: 4 }); });
            test.runAtTickTime(25, () => {
                for (let x = 2; x <= 8; x++) assertState(test, { x, y: 2, z: 5 }, "powered", false);
                test.succeed();
            });
        }, 40);

        // 断轨撤销下游信号；重新接上后恢复。
        registerCircuitTest(`${type}_cut_and_reconnect`, test => {
            track(test, type, 7);
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", { x: 2, y: 2, z: 4 }));
            test.runAtTickTime(15, () => { assertState(test, { x: 8, y: 2, z: 5 }, "powered", true); test.setBlockType("minecraft:air", { x: 5, y: 2, z: 5 }); });
            test.runAtTickTime(25, () => {
                assertState(test, { x: 4, y: 2, z: 5 }, "powered", true);
                assertState(test, { x: 8, y: 2, z: 5 }, "powered", false);
                setCircuitBlock(test, type, { x: 5, y: 2, z: 5 }, "shape=east_west");
            });
            test.runAtTickTime(35, () => { assertState(test, { x: 8, y: 2, z: 5 }, "powered", true); test.succeed(); });
        }, 50);

        // 普通铁轨不作为动力/激活轨之间的信号桥。
        registerCircuitTest(`${type}_ordinary_rail_isolates`, test => {
            track(test, type, 5);
            setCircuitBlock(test, "rail", { x: 4, y: 2, z: 5 }, "shape=east_west");
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", { x: 2, y: 2, z: 4 }));
            test.runAtTickTime(15, () => { assertState(test, { x: 3, y: 2, z: 5 }, "powered", true); assertState(test, { x: 5, y: 2, z: 5 }, "powered", false); test.succeed(); });
        }, 30);

        // 信号不能横穿不连接的平行轨道。
        registerCircuitTest(`${type}_parallel_track_isolates`, test => {
            track(test, type, 4);
            for (let x = 2; x <= 5; x++) setCircuitBlock(test, type, { x, y: 2, z: 6 }, "shape=east_west");
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", { x: 2, y: 2, z: 4 }));
            test.runAtTickTime(15, () => { assertState(test, { x: 5, y: 2, z: 5 }, "powered", true); assertState(test, { x: 5, y: 2, z: 6 }, "powered", false); test.succeed(); });
        }, 30);

        // 两种带电轨不能互相延续轨道内信号。
        registerCircuitTest(`${type}_other_powered_type_isolates`, test => {
            track(test, type, 5);
            setCircuitBlock(test, type === "powered_rail" ? "activator_rail" : "powered_rail", { x: 4, y: 2, z: 5 }, "shape=east_west");
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", { x: 2, y: 2, z: 4 }));
            test.runAtTickTime(15, () => { assertState(test, { x: 5, y: 2, z: 5 }, "powered", false); test.succeed(); });
        }, 30);
    }

    for (const type of ["rail", "powered_rail", "activator_rail", "detector_rail"]) {
        // 每种轨道均须响应支撑丢失，包括重写邻居更新的子类。
        registerCircuitTest(`${type}_loses_support`, test => {
            setCircuitBlock(test, type, { x: 5, y: 2, z: 5 }, "shape=east_west");
            test.runAtTickTime(5, () => test.setBlockType("minecraft:air", { x: 5, y: 1, z: 5 }));
            test.runAtTickTime(10, () => { assertType(test, { x: 5, y: 2, z: 5 }, "air"); test.succeed(); });
        }, 25);
    }

    // 动力轨是消费者，不能把内部充能状态输出给灯。
    registerCircuitTest("powered_rail_does_not_power_lamp", test => {
        track(test, "powered_rail", 3);
        test.setBlockType("minecraft:redstone_lamp", { x: 4, y: 2, z: 6 });
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", { x: 2, y: 2, z: 4 }));
        test.runAtTickTime(15, () => { assertState(test, { x: 4, y: 2, z: 5 }, "powered", true); assertState(test, { x: 4, y: 2, z: 6 }, "lit", false); test.succeed(); });
    }, 30);

    // Ref: docs/minecraft-wiki-source/minecraft_wiki/tech_探测铁轨.txt#红石元件
    // 探测轨须立即识别矿车，并在矿车移除后按 20 刻周期撤销输出。
    registerCircuitTest("detector_rail_detects_and_releases_cart", test => {
        const rail = { x: 5, y: 2, z: 5 };
        setCircuitBlock(test, "detector_rail", rail, "shape=north_south");
        test.setBlockType("minecraft:redstone_lamp", { x: 6, y: 2, z: 5 });
        test.spawn("minecraft:minecart", rail);
        test.runAtTickTime(10, () => {
            assertState(test, rail, "powered", true);
            assertState(test, { x: 6, y: 2, z: 5 }, "lit", true);
            test.killAllEntities();
        });
        test.runAtTickTime(40, () => {
            assertState(test, rail, "powered", false);
            assertState(test, { x: 6, y: 2, z: 5 }, "lit", false);
            test.succeed();
        });
    }, 55);

    // 探测轨强充能下方导体，导体旁的灯是间接消费者。
    registerCircuitTest("detector_rail_powers_support", test => {
        test.setBlockType("minecraft:stone", { x: 5, y: 2, z: 5 });
        setCircuitBlock(test, "detector_rail", { x: 5, y: 3, z: 5 }, "shape=north_south");
        test.setBlockType("minecraft:redstone_lamp", { x: 6, y: 2, z: 5 });
        test.spawn("minecraft:minecart", { x: 5, y: 3, z: 5 });
        test.runAtTickTime(10, () => { assertState(test, { x: 6, y: 2, z: 5 }, "lit", true); test.succeed(); });
    }, 25);
}
