// Ref: docs/minecraft-wiki-source/minecraft_wiki/tech_红石粉.txt#行为、形状
import { assertState, registerCircuitTest, setCircuitBlock, wireLine } from "../../utils/block/redstone.js";

const EAST = { x: 1, y: 0, z: 0 };

export function registerWireCircuitTests(): void {
    // 中继器前后两端均可连接红石，侧端仅用于二极管锁存。
    registerCircuitTest("wire", "wire_connects_repeater_input", test => {
        setCircuitBlock(test, "repeater", { x: 7, y: 2, z: 6 }, "facing=east");
        test.setBlockType("minecraft:redstone_wire", { x: 8, y: 2, z: 6 });
        test.runAtTickTime(10, () => { assertState(test, { x: 8, y: 2, z: 6 }, "west", "side"); test.succeed(); });
    }, 25);

    // 比较器侧面也接受红石输入，因此红石线须连到其侧端。
    registerCircuitTest("wire", "wire_connects_comparator_side", test => {
        setCircuitBlock(test, "comparator", { x: 7, y: 2, z: 6 }, "facing=east");
        test.setBlockType("minecraft:redstone_wire", { x: 7, y: 2, z: 5 });
        test.runAtTickTime(10, () => { assertState(test, { x: 7, y: 2, z: 5 }, "south", "side"); test.succeed(); });
    }, 25);

    // 完整测量 15 到 0 的梯度，防止互相反馈把整段线路锁在高电平。
    registerCircuitTest("wire", "wire_full_attenuation", test => {
        wireLine(test, { x: 3, y: 2, z: 6 }, EAST, 17);
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", { x: 2, y: 2, z: 6 }));
        test.runAtTickTime(30, () => {
            for (let i = 0; i < 17; i++) assertState(test, { x: 3 + i, y: 2, z: 6 }, "power", Math.max(0, 15 - i));
            test.succeed();
        });
    }, 50);

    registerCircuitTest("wire", "wire_cut_and_reconnect", test => {
        wireLine(test, { x: 3, y: 2, z: 6 }, EAST, 7);
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", { x: 2, y: 2, z: 6 }));
        test.runAtTickTime(20, () => { assertState(test, { x: 9, y: 2, z: 6 }, "power", 9); test.setBlockType("minecraft:air", { x: 6, y: 2, z: 6 }); });
        test.runAtTickTime(40, () => { assertState(test, { x: 9, y: 2, z: 6 }, "power", 0); test.setBlockType("minecraft:redstone_wire", { x: 6, y: 2, z: 6 }); });
        test.runAtTickTime(60, () => { assertState(test, { x: 9, y: 2, z: 6 }, "power", 9); test.succeed(); });
    }, 80);

    // 双电源移除一端后重建另一端梯度，最后移除全部电源后必须归零。
    registerCircuitTest("wire", "wire_dual_source_removal", test => {
        wireLine(test, { x: 3, y: 2, z: 6 }, EAST, 7);
        test.runAtTickTime(5, () => { test.setBlockType("minecraft:redstone_block", { x: 2, y: 2, z: 6 }); test.setBlockType("minecraft:redstone_block", { x: 10, y: 2, z: 6 }); });
        test.runAtTickTime(20, () => { assertState(test, { x: 6, y: 2, z: 6 }, "power", 12); test.setBlockType("minecraft:air", { x: 2, y: 2, z: 6 }); });
        test.runAtTickTime(40, () => { assertState(test, { x: 3, y: 2, z: 6 }, "power", 9); test.setBlockType("minecraft:air", { x: 10, y: 2, z: 6 }); });
        test.runAtTickTime(65, () => { for (let x = 3; x <= 9; x++) assertState(test, { x, y: 2, z: 6 }, "power", 0); test.succeed(); });
    }, 85);

    // 台阶红石可双向衰减传递，封住低处红石上方后必须断开。
    registerCircuitTest("wire", "wire_step_ceiling_disconnects", test => {
        test.setBlockType("minecraft:stone", { x: 6, y: 2, z: 6 });
        test.setBlockType("minecraft:redstone_wire", { x: 5, y: 2, z: 6 });
        test.setBlockType("minecraft:redstone_wire", { x: 6, y: 3, z: 6 });
        test.runAtTickTime(5, () => test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 6 }));
        test.runAtTickTime(20, () => { assertState(test, { x: 6, y: 3, z: 6 }, "power", 14); test.setBlockType("minecraft:stone", { x: 5, y: 3, z: 6 }); });
        test.runAtTickTime(40, () => { assertState(test, { x: 6, y: 3, z: 6 }, "power", 0); test.succeed(); });
    }, 60);
}
