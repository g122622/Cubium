// 天空光水平多级侧传衰减测试：验证天空光在水平方向 flood-fill 每格衰减1（与垂直传播同机制）。
//
// 核心机制（SkyLightEngine 垂直列传播 + BaseLightEngine 水平 flood-fill）：
//   - 垂直：露天列（上方无遮挡）从世界顶向下直达 skyLight=15，遇 opacity>0 方块 break。
//   - 水平：被遮挡列下方的空气格，改由水平方向露天邻居（skyLight=15）经 flood-fill 侧传，
//     每穿一格衰减 max(1, opacity)，空气 opacity=0 → 每格衰减1。
//   故遮挡区下方距露天边界水平距离 d 的空气格 skyLight = 15 - d。
//
// 已覆盖（SkyLightTests.stoneBlocksSkyLight / SkyLightColumnDepthTests）：单格/双格遮挡列下方
// 侧传得 14（距离1）。本组补「多级侧传」——遮挡一片区域，使下方内部点距露天边界距离递增，
// 验证 skyLight 随水平距离线性衰减（14→13），确证水平 flood-fill 每格衰减1（非一次性衰减）。
//
// 设计：grass_pen（9×5×9，内部 x,z∈[1,7] y∈[1,3] air，y=4 air 露天）+ skyAccess(true) + setupTicks(20)。
// 在 y=4 露天层放 3×3 石头天棚（x,z∈[3,5]，共9格）遮挡这9列垂直天空光。天棚下方 y=3 层 x,z∈[3,5]
// 的空气格只能从水平方向天棚外的露天列（y=3 天棚外是露天列 skyLight=15）侧传接收天空光：
//   - 边缘角格 (3,3,3)：距天棚外露天格 (2,3,3) 或 (3,3,2) 水平距离1 → skyLight=14。
//   - 中心格 (4,3,4)：距最近天棚外露天格（(2,3,4)/(4,3,2)/(6,3,4)/(4,3,6)）水平距离2 → skyLight=13。
//   - 天棚外露天列 (2,3,3)：垂直直达 skyLight=15（对照组）。
// canSeeSky：Cubium canSeeSky = (skyLight>=15)，故 14/13 格 canSeeSky=false，露天格=true。
//
// 光照重算异步：放天棚后入队，pollUntilSucceed 轮询等到 skyLight 稳定。
//
// 跨服务端：skyLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#天空光照（露天=15，Flood Fill 衰减传播）
// Ref: SkyLightTests.ts（单格遮挡列下方侧传得14）、SkyLightColumnDepthTests.ts（列深衰减）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getSkyLight, getCanSeeSky } from "../utils/lightAssert.js";

// 天空光水平多级侧传衰减：y=4 放 3×3 石头天棚（x,z∈[3,5]），遮挡9列垂直天空光。
// y=3 天棚下方：边缘角格 (3,3,3) 距露天水平1 → 14，中心格 (4,3,4) 距露天水平2 → 13。
// 天棚外露天列 (2,3,3) 垂直直达 → 15（对照组）。验证水平 flood-fill 每格衰减1（14→13 线性）。
function skyLightHorizontalSpread(test: Test): void {
    // y=4 露天层放 3×3 石头天棚，遮挡 x,z∈[3,5] 这9列的垂直天空光。
    for (let x = 3; x <= 5; ++x) {
        for (let z = 3; z <= 5; ++z) {
            test.setBlockType("minecraft:stone", { x, y: 4, z });
        }
    }
    pollUntilSucceed(
        test,
        () => {
            return (
                // 对照组：天棚外露天列 (2,3,3)，垂直直达 skyLight=15，canSeeSky=true。
                getSkyLight(test, 2, 3, 3) === 15 &&
                getCanSeeSky(test, 2, 3, 3) === true &&
                // 天棚下边缘角格 (3,3,3)：距露天 (2,3,3)/(3,3,2) 水平距离1 → 15-1=14。
                getSkyLight(test, 3, 3, 3) === 14 &&
                getCanSeeSky(test, 3, 3, 3) === false &&
                // 天棚下中心格 (4,3,4)：距最近露天格水平距离2 → 15-2=13。
                getSkyLight(test, 4, 3, 4) === 13 &&
                getCanSeeSky(test, 4, 3, 4) === false
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `sky horizontal spread: open(2,3,3)=${getSkyLight(test, 2, 3, 3)} ` +
                        `edge(3,3,3)=${getSkyLight(test, 3, 3, 3)} ` +
                        `center(4,3,4)=${getSkyLight(test, 4, 3, 4)} expected 15/14/13`,
                );
            },
        },
    );
}

export function registerSkyLightHorizontalSpreadTests(): void {
    GameTest.register("LightingTests", "light_sky_light_horizontal_spread", skyLightHorizontalSpread)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
}
