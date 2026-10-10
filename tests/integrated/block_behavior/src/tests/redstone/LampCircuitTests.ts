// Ref: docs/minecraft-wiki-source/minecraft_wiki/mechanism_红石灯.txt#用途
import { assertState, createLeverSwitch, registerCircuitTest } from "../../utils/block/redstone.js";

const LAMP = { x: 7, y: 2, z: 6 };
const SOURCE = { x: 8, y: 2, z: 6 };

export function registerLampCircuitTests(): void {
    // 充能来自灯上方红石时可激活；线路移除后仍遵守熄灭延迟。
    registerCircuitTest("lamp_wire_on_top", test => {
        test.setBlockType("minecraft:redstone_lamp", LAMP);
        test.setBlockType("minecraft:redstone_wire", { x: 7, y: 3, z: 6 });
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", { x: 8, y: 3, z: 6 }));
        test.runAtTickTime(15, () => { assertState(test, LAMP, "lit", true); test.setBlockType("minecraft:air", { x: 7, y: 3, z: 6 }); });
        test.runAtTickTime(25, () => { assertState(test, LAMP, "lit", false); test.succeed(); });
    }, 40);

    // 拉杆强充能灯后可激活邻灯，断电后两者均应熄灭。
    registerCircuitTest("lamp_strong_power_reaches_adjacent_lamp", test => {
        test.setBlockType("minecraft:redstone_lamp", LAMP);
        test.setBlockType("minecraft:redstone_lamp", { x: 6, y: 2, z: 6 });
        const toggle = createLeverSwitch(test, { x: 7, y: 3, z: 6 }, "face=floor");
        test.runAtTickTime(5, toggle);
        test.runAtTickTime(15, () => { assertState(test, { x: 6, y: 2, z: 6 }, "lit", true); toggle(); });
        test.runAtTickTime(25, () => { assertState(test, LAMP, "lit", false); assertState(test, { x: 6, y: 2, z: 6 }, "lit", false); test.succeed(); });
    }, 40);
    // 通电立即点亮；断电满 4 游戏刻才熄灭。
    registerCircuitTest("lamp_four_tick_fall_delay", test => {
        test.setBlockType("minecraft:redstone_lamp", LAMP);
        test.runAtTickTime(5, () => { test.setBlockType("minecraft:redstone_block", SOURCE); assertState(test, LAMP, "lit", true); });
        test.runAtTickTime(10, () => test.setBlockType("minecraft:air", SOURCE));
        test.runAtTickTime(13, () => assertState(test, LAMP, "lit", true));
        test.runAtTickTime(14, () => { assertState(test, LAMP, "lit", false); test.succeed(); });
    }, 30);

    // 熄灭计划刻执行时重新检查电源，不可关闭已恢复供电的灯。
    registerCircuitTest("lamp_repower_cancels_fall", test => {
        test.setBlockType("minecraft:redstone_lamp", LAMP);
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", SOURCE));
        test.runAtTickTime(10, () => test.setBlockType("minecraft:air", SOURCE));
        test.runAtTickTime(12, () => test.setBlockType("minecraft:redstone_block", SOURCE));
        test.runAtTickTime(16, () => { assertState(test, LAMP, "lit", true); test.succeed(); });
    }, 30);

    // 点亮不等于电源：一盏亮灯不能持续传递自己的亮起状态。
    registerCircuitTest("lamp_lit_state_is_not_signal", test => {
        test.setBlockType("minecraft:redstone_lamp", LAMP);
        test.setBlockType("minecraft:redstone_lamp", { x: 6, y: 2, z: 6 });
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", SOURCE));
        test.runAtTickTime(15, () => { assertState(test, LAMP, "lit", true); assertState(test, { x: 6, y: 2, z: 6 }, "lit", false); test.succeed(); });
    }, 30);

    // 火把对侧面的灯激活，对附着灯不激活。
    registerCircuitTest("lamp_torch_attachment_excluded", test => {
        test.setBlockType("minecraft:redstone_lamp", LAMP);
        test.setBlockType("minecraft:redstone_torch", { x: 7, y: 3, z: 6 });
        test.setBlockType("minecraft:redstone_lamp", { x: 8, y: 3, z: 6 });
        test.runAtTickTime(15, () => { assertState(test, LAMP, "lit", false); assertState(test, { x: 8, y: 3, z: 6 }, "lit", true); test.succeed(); });
    }, 30);

    // 实心导体能把强信号传给邻灯，玻璃不能。
    for (const conductor of ["stone", "glass"]) {
        registerCircuitTest(`lamp_through_${conductor}`, test => {
            test.setBlockType("minecraft:redstone_lamp", LAMP);
            test.setBlockType(`minecraft:${conductor}`, SOURCE);
            const toggle = createLeverSwitch(test, { x: 8, y: 3, z: 6 }, "face=floor");
            test.runAtTickTime(5, toggle);
            test.runAtTickTime(15, () => { assertState(test, LAMP, "lit", conductor === "stone"); test.succeed(); });
        }, 30);
    }
}
