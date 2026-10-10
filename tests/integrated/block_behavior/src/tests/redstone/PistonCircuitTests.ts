// Ref: docs/minecraft-wiki-source/minecraft_wiki/tech_活塞.txt#推动方块、黏性活塞、激活
import { assertState, assertType, HORIZONTAL_INPUTS, offset, registerCircuitTest, setCircuitBlock } from "../../utils/block/redstone.js";

const PISTON = { x: 5, y: 3, z: 6 };
const SOURCE = { x: 5, y: 3, z: 5 };
const EAST = { x: 1, y: 0, z: 0 };

export function registerPistonCircuitTests(): void {
    // 可推动材质不能因材质表中的旧分类而变成障碍或被破坏。
    for (const material of ["gold_block", "iron_block", "glass", "ice", "packed_ice"]) {
        registerCircuitTest(`piston_pushes_${material}`, test => {
            setCircuitBlock(test, "piston", PISTON, "facing=east");
            test.setBlockType(`minecraft:${material}`, offset(PISTON, EAST, 1));
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", SOURCE));
            test.runAtTickTime(15, () => {
                assertState(test, PISTON, "extended", true);
                assertType(test, offset(PISTON, EAST, 1), "piston_head");
                assertType(test, offset(PISTON, EAST, 2), material);
                test.succeed();
            });
        }, 30);
    }

    // 多侧枝会扩展移动列表；扩展期间不能保留指向旧列表存储的引用。
    registerCircuitTest("piston_slime_multiple_branches", test => {
        setCircuitBlock(test, "piston", PISTON, "facing=east");
        test.setBlockType("minecraft:slime_block", offset(PISTON, EAST, 1));
        const branches = [{ x: 6, y: 4, z: 6 }, { x: 6, y: 3, z: 5 }, { x: 6, y: 3, z: 7 }];
        for (const branch of branches) test.setBlockType("minecraft:gold_block", branch);
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", SOURCE));
        test.runAtTickTime(20, () => {
            for (const branch of branches) {
                assertType(test, offset(branch, EAST, 1), "gold_block");
                assertType(test, branch, "air");
            }
            test.succeed();
        });
    }, 35);

    // 障碍移除后，已通电的活塞须响应邻居更新，恢复伸出。
    registerCircuitTest("piston_extends_after_obstacle_removed", test => {
        setCircuitBlock(test, "piston", PISTON, "facing=east");
        test.setBlockType("minecraft:obsidian", offset(PISTON, EAST, 1));
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", SOURCE));
        test.runAtTickTime(15, () => {
            assertState(test, PISTON, "extended", false);
            test.setBlockType("minecraft:air", offset(PISTON, EAST, 1));
        });
        test.runAtTickTime(25, () => { assertState(test, PISTON, "extended", true); test.succeed(); });
    }, 40);

    // 一个黏性活塞反复推动、拉回后不能遗失或复制方块。
    registerCircuitTest("sticky_piston_repeated_cycles", test => {
        setCircuitBlock(test, "sticky_piston", PISTON, "facing=east");
        test.setBlockType("minecraft:iron_block", offset(PISTON, EAST, 1));
        for (let i = 0; i < 3; i++) {
            test.runAtTickTime(5 + i * 30, () => test.setBlockType("minecraft:redstone_block", SOURCE));
            test.runAtTickTime(15 + i * 30, () => {
                assertType(test, offset(PISTON, EAST, 2), "iron_block");
                test.setBlockType("minecraft:air", SOURCE);
            });
            test.runAtTickTime(25 + i * 30, () => {
                assertType(test, offset(PISTON, EAST, 1), "iron_block");
                assertType(test, offset(PISTON, EAST, 2), "air");
            });
        }
        test.runAtTickTime(90, () => test.succeed());
    }, 105);

    // 六向直接供电与收回，不使用预设 extended 来替代真正活塞动作。
    for (const direction of [...HORIZONTAL_INPUTS, { name: "up", x: 0, y: 1, z: 0 }, { name: "down", x: 0, y: -1, z: 0 }]) {
        registerCircuitTest(`piston_cycle_${direction.name}`, test => {
            const pos = { x: 7, y: 4, z: 7 };
            setCircuitBlock(test, "piston", pos, `facing=${direction.name}`);
            const source = direction.name === "north" ? { x: 7, y: 4, z: 8 } : { x: 7, y: 4, z: 6 };
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", source));
            test.runAtTickTime(15, () => {
                assertState(test, pos, "extended", true);
                assertType(test, offset(pos, direction, 1), "piston_head");
                test.setBlockType("minecraft:air", source);
            });
            test.runAtTickTime(25, () => {
                assertState(test, pos, "extended", false);
                assertType(test, offset(pos, direction, 1), "air");
                test.succeed();
            });
        }, 40);
    }

    for (const sticky of [false, true]) {
        // 普通活塞推出后留下方块；黏性活塞收回时把方块拉到原位置。
        registerCircuitTest(`${sticky ? "sticky_piston" : "piston"}_moves_and_retracts_block`, test => {
            setCircuitBlock(test, sticky ? "sticky_piston" : "piston", PISTON, "facing=east");
            setCircuitBlock(test, "oak_log", offset(PISTON, EAST, 1), "axis=z");
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", SOURCE));
            test.runAtTickTime(15, () => {
                assertState(test, PISTON, "extended", true);
                assertType(test, offset(PISTON, EAST, 2), "oak_log");
                assertState(test, offset(PISTON, EAST, 2), "axis", "z");
                test.setBlockType("minecraft:air", SOURCE);
            });
            test.runAtTickTime(25, () => {
                assertState(test, PISTON, "extended", false);
                assertType(test, offset(PISTON, EAST, sticky ? 1 : 2), "oak_log");
                assertType(test, offset(PISTON, EAST, sticky ? 2 : 1), "air");
                test.succeed();
            });
        }, 40);
    }

    // 推动上限恰为 12；13 个方块时整条链不得发生部分位移。
    for (const count of [12, 13]) {
        registerCircuitTest(`piston_push_limit_${count}`, test => {
            setCircuitBlock(test, "piston", PISTON, "facing=east");
            for (let i = 1; i <= count; i++) test.setBlockType("minecraft:stone", offset(PISTON, EAST, i));
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", SOURCE));
            test.runAtTickTime(20, () => {
                assertState(test, PISTON, "extended", count === 12);
                assertType(test, offset(PISTON, EAST, 1), count === 12 ? "piston_head" : "stone");
                assertType(test, offset(PISTON, EAST, count + 1), count === 12 ? "stone" : "air");
                test.succeed();
            });
        }, 35);
    }

    for (const obstacle of ["obsidian", "chest"]) {
        // 不可移动方块和有方块实体的容器会阻止整次推动。
        registerCircuitTest(`piston_blocked_by_${obstacle}`, test => {
            setCircuitBlock(test, "piston", PISTON, "facing=east");
            test.setBlockType("minecraft:stone", offset(PISTON, EAST, 1));
            test.setBlockType(`minecraft:${obstacle}`, offset(PISTON, EAST, 2));
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", SOURCE));
            test.runAtTickTime(15, () => { assertState(test, PISTON, "extended", false); assertType(test, offset(PISTON, EAST, 1), "stone"); assertType(test, offset(PISTON, EAST, 2), obstacle); test.succeed(); });
        }, 30);
    }

    // 活塞面前的电源不激活活塞。
    registerCircuitTest("piston_ignores_front_power", test => {
        setCircuitBlock(test, "piston", PISTON, "facing=east");
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", offset(PISTON, EAST, 1)));
        test.runAtTickTime(15, () => { assertState(test, PISTON, "extended", false); test.succeed(); });
    }, 30);

    // 准连接：上方一格的相邻电源经活塞邻居更新触发动作，撤销后也须更新才能收回。
    registerCircuitTest("piston_quasi_connection", test => {
        setCircuitBlock(test, "piston", PISTON, "facing=east");
        const quasi = { x: 5, y: 4, z: 5 };
        test.runAtTickTime(5, () => {
            test.setBlockType("minecraft:redstone_block", quasi);
            test.setBlockType("minecraft:stone", { x: 4, y: 3, z: 6 });
        });
        test.runAtTickTime(15, () => {
            assertState(test, PISTON, "extended", true);
            test.setBlockType("minecraft:air", quasi);
            test.setBlockType("minecraft:air", { x: 4, y: 3, z: 6 });
        });
        test.runAtTickTime(25, () => { assertState(test, PISTON, "extended", false); test.succeed(); });
    }, 40);

    // 黏性分支应带动侧面方块；黏液与蜂蜜之间不黏连。
    for (const adhesive of ["slime_block", "honey_block"]) {
        registerCircuitTest(`piston_${adhesive}_side_branch`, test => {
            setCircuitBlock(test, "piston", PISTON, "facing=east");
            test.setBlockType(`minecraft:${adhesive}`, offset(PISTON, EAST, 1));
            test.setBlockType("minecraft:gold_block", { x: 6, y: 3, z: 7 });
            test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", SOURCE));
            test.runAtTickTime(20, () => {
                assertState(test, PISTON, "extended", true);
                assertType(test, { x: 7, y: 3, z: 6 }, adhesive);
                const moved = test.getBlock({ x: 7, y: 3, z: 7 })?.typeId;
                const original = test.getBlock({ x: 6, y: 3, z: 7 })?.typeId;
                test.assert(moved === "minecraft:gold_block", `Side branch expected gold_block, destination=${moved}, origin=${original}`);
                assertType(test, { x: 6, y: 3, z: 7 }, "air");
                test.succeed();
            });
        }, 35);
    }
    registerCircuitTest("piston_slime_honey_do_not_stick", test => {
        setCircuitBlock(test, "piston", PISTON, "facing=east");
        test.setBlockType("minecraft:slime_block", offset(PISTON, EAST, 1));
        test.setBlockType("minecraft:honey_block", { x: 6, y: 3, z: 7 });
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", SOURCE));
        test.runAtTickTime(20, () => { assertType(test, { x: 7, y: 3, z: 6 }, "slime_block"); assertType(test, { x: 6, y: 3, z: 7 }, "honey_block"); test.succeed(); });
    }, 35);
}
