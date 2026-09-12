// 末地传送门发光等级测试：验证 end_portal / end_gateway 静态发光 15（对齐 wiki 发光方块表）。
//
// wiki 发光方块表（tech_亮度.txt#发光方块）：末地传送门方块光照等级 15、末地折跃门方块光照等级 15。
// Cubium 实现（NetherBlocks.cpp:689-700）：
//   end_portal  注册处 .lightLevel(15) 静态值，EndPortalBlock 无 getLightLevel override，故发光等级恒 15。
//   end_gateway 注册处 .lightLevel(15) 静态值，EndGatewayBlock 无 getLightLevel override，故发光等级恒 15。
//
// 放置难点与解决方案：end_portal / end_gateway 的 Block 子类未重写 isValidPosition / updatePostPlacement /
// neighborChanged / scheduledTick（与 nether_portal 同构）。setBlockWithStates 走 ServerWorld::setBlockState
// (flags=3)，flags=3 下 setBlockState 对新方块只调用 onBlockAdded（EndPortalBlock/EndGatewayBlock 未重写，空
// 操作），不调用新方块自身的 updatePostPlacement，故 portal/gateway 在无框架的 light_box 内能稳定存活。放置后
// 保持周围方块不动即可（不再 setBlock 触发邻居更新链）。该行为与 vanilla Java 1.21.11 对齐。
//
// 设计：light_box（7×7×7 封顶实心盒，内部 x,z∈[1,5] y∈[1,5]，skyLight=0 隔绝天空光）。
// 在 PLACE (3,1,3) 用 setBlockWithStates 放置目标方块，断言该格 blockLight=15。
// light_box 封顶保证 skyLight=0，blockLight 是唯一光源，portal/gateway 自身格 blockLight 即其发光等级。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#发光方块（末地传送门方块 15、末地折跃门方块 15）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

const PLACE = { x: 3, y: 1, z: 3 };

// 末地传送门发光15：setBlockWithStates 在 light_box 内强制放置 end_portal，无框架仍存活，
// 该格 blockLight=15（静态 lightLevel(15)）。验证末地传送门方块发光等级与 wiki 一致。
function endPortalEmitsFifteen(test: Test): void {
    test.setBlockWithStates("minecraft:end_portal", PLACE, "");
    pollUntilSucceed(test, () => getBlockLight(test, PLACE.x, PLACE.y, PLACE.z) === 15, {
        startTick: 5,
        interval: 4,
        maxTick: 100,
        onTimeout: () => {
            const actual = getBlockLight(test, PLACE.x, PLACE.y, PLACE.z);
            const block = test.getBlock(PLACE);
            const typeId = block?.typeId ?? "null";
            test.assert(
                false,
                `end_portal: expected blockLight=15 at source, got ${actual} ` +
                    `(blockType=${typeId}; portal may have been removed by updatePostPlacement without frame?)`,
            );
        },
    });
}

// 末地折跃门发光15：setBlockWithStates 在 light_box 内强制放置 end_gateway，无框架仍存活，
// 该格 blockLight=15（静态 lightLevel(15)）。验证末地折跃门方块发光等级与 wiki 一致。
function endGatewayEmitsFifteen(test: Test): void {
    test.setBlockWithStates("minecraft:end_gateway", PLACE, "");
    pollUntilSucceed(test, () => getBlockLight(test, PLACE.x, PLACE.y, PLACE.z) === 15, {
        startTick: 5,
        interval: 4,
        maxTick: 100,
        onTimeout: () => {
            const actual = getBlockLight(test, PLACE.x, PLACE.y, PLACE.z);
            const block = test.getBlock(PLACE);
            const typeId = block?.typeId ?? "null";
            test.assert(
                false,
                `end_gateway: expected blockLight=15 at source, got ${actual} ` +
                    `(blockType=${typeId}; gateway may have been removed by updatePostPlacement without frame?)`,
            );
        },
    });
}

export function registerEndPortalEmissionTests(): void {
    GameTest.register("LightingTests", "light_end_portal_emits_15", endPortalEmitsFifteen)
        .structureName("gametests:light_box")
        .maxTicks(120);

    GameTest.register("LightingTests", "light_end_gateway_emits_15", endGatewayEmitsFifteen)
        .structureName("gametests:light_box")
        .maxTicks(120);
}
