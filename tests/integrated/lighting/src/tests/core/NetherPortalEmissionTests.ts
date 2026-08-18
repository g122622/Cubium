// 下界传送门发光等级测试：验证 nether_portal 静态发光 11（对齐 wiki 发光方块表）。
//
// wiki 发光方块表（tech_亮度.txt#发光方块）：下界传送门方块光照等级 11。
// Cubium 实现（NetherBlocks.cpp:235-237）：注册处 .lightLevel(11) 静态值，NetherPortalBlock 无 getLightLevel
// override，故发光等级恒 11，与 axis state 无关（原版亦固定 11）。
//
// 放置难点与解决方案：nether_portal 的 isValidPosition 要求至少一个相邻方块是 nether_portal 或 obsidian
// （NetherPortalBlock.cpp:84-129），纯空气环境返回 false。但 setBlockWithStates 走 ServerWorld::setBlockState
// (flags=3)，flags=3 下 setBlockState 对新方块只调用 onBlockAdded（NetherPortalBlock 未重写，空操作），
// 不调用新方块自身的 updatePostPlacement（含 isValidPosition 检查），故 portal 在无框架的 light_box 内能稳定
// 存活。邻居（air）的 updatePostPlacement 返回自身，不反向改 portal。NetherPortalBlock 未重写 neighborChanged/
// scheduledTick，无后续 tick 移除风险。该行为与 vanilla Java 1.21.11 对齐（Level.setBlock 不对新放 portal
// 自身调用 updateShape）。放置后保持 portal 周围 6 格不动即可（不再 setBlock 触发邻居更新链）。
//
// 设计：light_box（7×7×7 封顶实心盒，内部 x,z∈[1,5] y∈[1,5]，skyLight=0 隔绝天空光）。
// 在 PLACE (3,1,3) 用 setBlockWithStates 放 nether_portal(axis=x)，断言该格 blockLight=11。
// light_box 封顶保证 skyLight=0，blockLight 是唯一光源，portal 自身格 blockLight 即其发光等级。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#发光方块（下界传送门方块 11）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

const PLACE = { x: 3, y: 1, z: 3 };

// 下界传送门发光11：setBlockWithStates 在 light_box 内强制放置 nether_portal(axis=x)，无黑曜石框架仍存活，
// 该格 blockLight=11（静态 lightLevel(11)）。验证下界传送门方块发光等级与 wiki 一致。
// 放置后不再修改 portal 周围方块，避免触发邻居更新链导致 portal updatePostPlacement→isValidPosition 失败变 air。
function netherPortalEmitsEleven(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PLACE, "axis=x");
    pollUntilSucceed(test, () => getBlockLight(test, PLACE.x, PLACE.y, PLACE.z) === 11, {
        startTick: 5,
        interval: 4,
        maxTick: 100,
        onTimeout: () => {
            const actual = getBlockLight(test, PLACE.x, PLACE.y, PLACE.z);
            test.assert(
                false,
                `nether_portal: expected blockLight=11 at source, got ${actual} ` +
                    `(portal may have been removed by updatePostPlacement without obsidian frame?)`,
            );
        },
    });
}

export function registerNetherPortalEmissionTests(): void {
    GameTest.register("LightingTests", "light_nether_portal_emits_11", netherPortalEmitsEleven)
        .structureName("gametests:light_box")
        .maxTicks(120);
}
