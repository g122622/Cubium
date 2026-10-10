// 普通铁轨的放置连接与邻轨拆除行为。
// Ref: docs/minecraft-wiki-source/minecraft_wiki/tech_铁轨.txt#放置
// 普通铁轨只在新轨道连接传播、或三连接道岔收到电源变化时重算。
// 拆除弯道的一端不会自动把原弯轨改成直轨。
import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { assertState } from "../../utils/block/redstone.js";

const CORNER = { x: 3, y: 2, z: 2 };
const EAST_RAIL = { x: 4, y: 2, z: 2 };

function placeLRails(test: Test): void {
    for (const pos of [CORNER, { x: 3, y: 2, z: 3 }, EAST_RAIL]) {
        test.setBlockType("minecraft:stone", { x: pos.x, y: 1, z: pos.z });
        test.setBlockType("minecraft:rail", pos);
    }
}

// 南、东两个邻轨把中心轨道连接成东南弯轨。
function railBendsIntoLShapeAtCorner(test: Test): void {
    placeLRails(test);
    test.runAtTickTime(10, () => {
        assertState(test, CORNER, "shape", "south_east");
        test.succeed();
    });
}

// 邻轨拆除不会触发连接重算，弯轨在持续观察期间保持原方向。
function railKeepsCurveWhenLBranchRemoved(test: Test): void {
    placeLRails(test);
    test.runAtTickTime(5, () => {
        assertState(test, CORNER, "shape", "south_east");
        test.setBlockType("minecraft:air", EAST_RAIL);
    });
    for (const tick of [10, 20, 40]) test.runAtTickTime(tick, () => assertState(test, CORNER, "shape", "south_east"));
    test.runAtTickTime(45, () => test.succeed());
}

export function registerRailTests(): void {
    GameTest.register("BlockBehaviorTests", "rail_bends_into_l_shape_at_corner", railBendsIntoLShapeAtCorner)
        .structureName("gametests:glass_pit")
        .maxTicks(30);
    GameTest.register("BlockBehaviorTests", "rail_keeps_curve_when_l_branch_removed", railKeepsCurveWhenLBranchRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
