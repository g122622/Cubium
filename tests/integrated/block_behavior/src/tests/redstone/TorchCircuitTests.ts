// Ref: docs/minecraft-wiki-source/minecraft_wiki/tech_红石火把.txt#输入、输出、烧毁
import { assertState, assertType, createLeverSwitch, registerCircuitTest, setCircuitBlock } from "../../utils/block/redstone.js";

const TORCH = { x: 7, y: 3, z: 6 };
const SUPPORT = { x: 7, y: 2, z: 6 };
const SOURCE = { x: 8, y: 2, z: 6 };

export function registerTorchCircuitTests(): void {
    // 末次熄灭时输入仍有效，烧毁会安排 160 刻后的复查；普通更新不替换已有计划刻。
    registerCircuitTest("torch", "torch_burnout_keeps_scheduled_recheck", test => {
        test.setBlockType("minecraft:stone", SUPPORT);
        test.setBlockType("minecraft:redstone_torch", TORCH);
        const toggle = createLeverSwitch(test, SOURCE, "face=wall,facing=east");
        for (let i = 0; i < 8; i++) {
            test.runAtTickTime(5 + i * 6, toggle);
            test.runAtTickTime(8 + i * 6, toggle);
        }
        test.runAtTickTime(60, () => assertState(test, TORCH, "lit", false));
        test.runAtTickTime(75, () => test.setBlockType("minecraft:stone", { x: 7, y: 3, z: 7 }));
        test.runAtTickTime(80, () => assertState(test, TORCH, "lit", false));
        test.runAtTickTime(215, () => { assertState(test, TORCH, "lit", true); test.succeed(); });
    }, 235);

    // 烧毁历史按方块位置保存，快速拆除重放不能清空最近的熄灭事件。
    registerCircuitTest("torch", "torch_replacement_keeps_burnout_history", test => {
        test.setBlockType("minecraft:stone", SUPPORT);
        test.setBlockType("minecraft:redstone_torch", TORCH);
        const toggle = createLeverSwitch(test, SOURCE, "face=wall,facing=east");
        for (let i = 0; i < 7; i++) {
            test.runAtTickTime(5 + i * 6, toggle);
            test.runAtTickTime(8 + i * 6, toggle);
        }
        test.runAtTickTime(48, () => {
            assertState(test, TORCH, "lit", true);
            test.setBlockType("minecraft:air", TORCH);
            test.setBlockType("minecraft:redstone_torch", TORCH);
        });
        test.runAtTickTime(53, toggle);
        test.runAtTickTime(56, toggle);
        test.runAtTickTime(62, () => { assertState(test, TORCH, "lit", false); test.succeed(); });
    }, 80);

    // 墙火把必须根据侧面附着反相；不能继承落地火把的下方检测。
    registerCircuitTest("torch", "wall_torch_inverts_support", test => {
        test.setBlockType("minecraft:stone", SUPPORT);
        setCircuitBlock(test, "redstone_wall_torch", SOURCE, "facing=east");
        const toggle = createLeverSwitch(test, { x: 7, y: 3, z: 6 }, "face=floor");
        test.runAtTickTime(5, toggle);
        test.runAtTickTime(15, () => { assertState(test, SOURCE, "lit", false); toggle(); });
        test.runAtTickTime(25, () => { assertState(test, SOURCE, "lit", true); test.succeed(); });
    }, 40);

    // 两刻以下的脉冲在计划刻采样前已结束，火把输出不得被中断。
    registerCircuitTest("torch", "torch_filters_one_tick_input", test => {
        test.setBlockType("minecraft:stone", SUPPORT);
        test.setBlockType("minecraft:redstone_torch", TORCH);
        const toggle = createLeverSwitch(test, SOURCE, "face=wall,facing=east");
        test.runAtTickTime(10, toggle);
        test.runAtTickTime(11, toggle);
        for (let tick = 12; tick <= 20; tick++) test.runAtTickTime(tick, () => assertState(test, TORCH, "lit", true));
        test.runAtTickTime(21, () => test.succeed());
    }, 35);

    // 火把侧面仅提供弱信号，不能经侧面导体继续激活导体另一侧的灯。
    registerCircuitTest("torch", "torch_side_conductor_not_strongly_powered", test => {
        test.setBlockType("minecraft:stone", SUPPORT);
        test.setBlockType("minecraft:redstone_torch", TORCH);
        test.setBlockType("minecraft:stone", { x: 8, y: 3, z: 6 });
        test.setBlockType("minecraft:redstone_lamp", { x: 9, y: 3, z: 6 });
        test.runAtTickTime(15, () => { assertState(test, { x: 9, y: 3, z: 6 }, "lit", false); test.succeed(); });
    }, 30);
    // 火把只强充能正上方导体，该导体能继续激活另一侧红石线。
    registerCircuitTest("torch", "torch_strongly_powers_above", test => {
        test.setBlockType("minecraft:stone", SUPPORT);
        test.setBlockType("minecraft:redstone_torch", TORCH);
        test.setBlockType("minecraft:stone", { x: 7, y: 4, z: 6 });
        test.setBlockType("minecraft:stone", { x: 8, y: 3, z: 6 });
        test.setBlockType("minecraft:redstone_wire", { x: 8, y: 4, z: 6 });
        test.runAtTickTime(15, () => { assertState(test, { x: 8, y: 4, z: 6 }, "power", 15); test.succeed(); });
    }, 30);

    // 火把熄灭和复亮均延迟 2 游戏刻。
    registerCircuitTest("torch", "torch_two_tick_inversion", test => {
        test.setBlockType("minecraft:stone", SUPPORT);
        test.setBlockType("minecraft:redstone_torch", TORCH);
        const toggle = createLeverSwitch(test, SOURCE, "face=wall,facing=east");
        test.runAtTickTime(10, toggle);
        test.runAtTickTime(11, () => assertState(test, TORCH, "lit", true));
        test.runAtTickTime(12, () => assertState(test, TORCH, "lit", false));
        test.runAtTickTime(20, toggle);
        test.runAtTickTime(21, () => assertState(test, TORCH, "lit", false));
        test.runAtTickTime(22, () => { assertState(test, TORCH, "lit", true); test.succeed(); });
    }, 35);

    // 直接替换附着为持续电源不会让火把永远亮起。
    registerCircuitTest("torch", "torch_on_redstone_block_turns_off", test => {
        test.setBlockType("minecraft:redstone_block", SUPPORT);
        test.setBlockType("minecraft:redstone_torch", TORCH);
        test.runAtTickTime(10, () => { assertState(test, TORCH, "lit", false); test.succeed(); });
    }, 25);

    registerCircuitTest("torch", "torch_loses_support", test => {
        test.setBlockType("minecraft:stone", SUPPORT);
        test.setBlockType("minecraft:redstone_torch", TORCH);
        test.runAtTickTime(5, () => test.setBlockType("minecraft:air", SUPPORT));
        test.runAtTickTime(10, () => { assertType(test, TORCH, "air"); test.succeed(); });
    }, 25);

    // 墙火把不向附着方块供电，正面输出仍有效。
    registerCircuitTest("torch", "wall_torch_excludes_support", test => {
        test.setBlockType("minecraft:redstone_lamp", SUPPORT);
        setCircuitBlock(test, "redstone_wall_torch", SOURCE, "facing=east");
        test.setBlockType("minecraft:redstone_lamp", { x: 9, y: 2, z: 6 });
        test.runAtTickTime(15, () => { assertState(test, SUPPORT, "lit", false); assertState(test, { x: 9, y: 2, z: 6 }, "lit", true); test.succeed(); });
    }, 30);

    // 烧毁只累计熄灭事件，七次不能烧毁；第八次熄灭后保持熄灭并在冷却后复燃。
    for (const cycles of [7, 8]) {
        registerCircuitTest("torch", `torch_burnout_${cycles}_off_events`, test => {
            test.setBlockType("minecraft:stone", SUPPORT);
            test.setBlockType("minecraft:redstone_torch", TORCH);
            const toggle = createLeverSwitch(test, SOURCE, "face=wall,facing=east");
            for (let i = 0; i < cycles; i++) {
                test.runAtTickTime(5 + i * 6, toggle);
                test.runAtTickTime(8 + i * 6, toggle);
            }
            test.runAtTickTime(5 + cycles * 6, () => assertState(test, TORCH, "lit", cycles < 8));
            test.runAtTickTime(215, () => { assertState(test, TORCH, "lit", true); test.succeed(); });
        }, 235);
    }
}
