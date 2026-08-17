// 铜灯笼发光等级测试：验证 copper_lantern 静态发光 15（对齐 wiki 发光方块表）。
//
// wiki 发光方块表（tech_亮度.txt#发光方块）：铜灯笼光照等级 15。1.21 新增方块，无 lit state，恒发光（区别于
// 需红石信号的红石灯、需点燃的蜡烛）。
// Cubium 实现（CopperBlocks.cpp:845-893）：WeatheringCopperLanternBlock 构造第 3 参数 lightValue=15，
// getLightLevel override 恒返回 15（不查 state，与氧化度/上蜡无关）。所有氧化变体（unaffected/exposed/
// weathered/oxidized）与上蜡变体均传 15，发光一致。
//
// 放置难点：铜灯笼有附着约束——isValidPosition 据 HANGING state 查上方（悬挂）或下方（站立）是否实体坚固面，
// updatePostPlacement 在支撑消失时变 air（对齐 vanilla LanternBlock）。但放置时 onBlockAdded 未 override
// （走默认空操作，不检查支撑），flags=3 的邻居更新只通知周围、不反向触发自身 updatePostPlacement，故站立态
// （HANGING=false 默认）放置在 stone 地板上能稳定存活。下方 (3,0,3)=stone 是实体坚固面满足支撑。
// 故用 setBlockType 直接放（默认站立态），不传 hanging=true（否则上方 air 不满足悬挂支撑，有自毁风险）。
//
// 设计：light_box（7×7×7 封顶实心盒，内部 x,z∈[1,5] y∈[1,5]，skyLight=0 隔绝天空光）。
// 在 PLACE (3,1,3) 放 copper_lantern（站立态，下方 stone 支撑），断言该格 blockLight=15。
//
// 现有 BlockLightEmissionTests 已覆盖多种 15 级光源但未覆盖铜灯笼。本组补基础 copper_lantern（unaffected 氧化度）
// 代表铜灯笼家族（所有氧化/上蜡变体发光均 15）。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#发光方块（铜灯笼 15）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

const PLACE = { x: 3, y: 1, z: 3 };

// 铜灯笼发光15：站立态（默认 hanging=false）放置在 stone 地板上，下方 solid 满足支撑，稳定存活发光15。
// 不传 hanging=true（悬挂态上方 air 不满足支撑有自毁风险）。验证铜灯笼恒发光15（无 lit state，与红石灯区别）。
function copperLanternEmitsFifteen(test: Test): void {
    test.setBlockType("minecraft:copper_lantern", PLACE);
    pollUntilSucceed(test, () => getBlockLight(test, PLACE.x, PLACE.y, PLACE.z) === 15, {
        startTick: 5,
        interval: 4,
        maxTick: 100,
        onTimeout: () => {
            const actual = getBlockLight(test, PLACE.x, PLACE.y, PLACE.z);
            test.assert(
                false,
                `copper_lantern: expected blockLight=15 at source, got ${actual} ` +
                    `(lantern may have been removed by updatePostPlacement without solid support below?)`,
            );
        },
    });
}

export function registerCopperLanternEmissionTests(): void {
    GameTest.register("LightingTests", "light_copper_lantern_emits_15", copperLanternEmitsFifteen)
        .structureName("gametests:light_box")
        .maxTicks(120);
}
