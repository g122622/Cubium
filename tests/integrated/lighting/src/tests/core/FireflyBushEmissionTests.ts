// 萤火虫灌木发光等级测试：验证 firefly_bush 静态发光 2（对齐 wiki 发光方块表，1.21.4 新增装饰植物）。
//
// wiki 发光方块表（tech_亮度.txt#发光方块，行398-399）：萤火虫灌木丛光照等级 2。
// Cubium 实现（GardenBlocks.cpp:138-145）：注册处 BlockProperties(Material::PLANT).lightLevel(2) 静态值，
// FireflyBushBlock 继承 BushBlock 无 getLightLevel override，故发光等级恒 2。
//
// 放置支撑：firefly_bush 继承 BushBlock，canSurvive 要求下方为 #dirt 标签或耕地（BushBlock.cpp:51-65
// canSustain → 下方方块 canSustainPlant）。light_box 地板 (3,0,3) 默认是 stone（不属 #dirt），若直接放
// firefly_bush 在 stone 上，虽 setBlockWithStates flags=3 不调新方块自身 updatePostPlacement（与 nether_portal
// 同理可强制存活），但 BushBlock::updatePostPlacement（cpp:67-93）在 facing==Down 时检查 canSustain，不满足
// 则返回 air 自毁——一旦后续任何邻居更新触发其 updatePostPlacement（facing=Down）即被移除，存在时序风险。
// 故本测试先用 setBlockType 把 (3,0,3) 地板换为 dirt（满足 #dirt 支撑），再在 (3,1,3) 放 firefly_bush，
// 使 canSurvive 真正成立，无自毁风险（更贴近 vanilla 放置语义）。dirt 在 #dirt 标签内，canSustainPlant 返回 true。
//
// 设计：light_box（7×7×7 封顶实心盒，内部 x,z∈[1,5] y∈[1,5]，skyLight=0 隔绝天空光）。
// (3,0,3) 放 dirt 支撑，(3,1,3) 放 firefly_bush，断言 (3,1,3) blockLight=2。
// light_box 封顶保证 skyLight=0，blockLight 是唯一光源，firefly_bush 自身格 blockLight 即其发光等级。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#发光方块（萤火虫灌木丛 2，行398-399）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

const SUPPORT = { x: 3, y: 0, z: 3 };
const PLACE = { x: 3, y: 1, z: 3 };

// 萤火虫灌木发光2：先在 (3,0,3) 放 dirt 作 #dirt 支撑（满足 BushBlock canSurvive），再 (3,1,3) 放 firefly_bush，
// 该格 blockLight=2（静态 lightLevel(2)）。验证萤火虫灌木发光等级与 wiki 一致。
// 用 dirt 支撑而非依赖 flags=3 强制存活，使 canSurvive 真正成立，规避 BushBlock updatePostPlacement 自毁时序风险。
function fireflyBushEmitsTwo(test: Test): void {
    test.setBlockType("minecraft:dirt", SUPPORT);
    test.setBlockType("minecraft:firefly_bush", PLACE);
    pollUntilSucceed(
        test,
        () => getBlockLight(test, PLACE.x, PLACE.y, PLACE.z) === 2,
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                const actual = getBlockLight(test, PLACE.x, PLACE.y, PLACE.z);
                test.assert(
                    false,
                    `firefly_bush: expected blockLight=2 at source, got ${actual} ` +
                        `(firefly_bush may have been removed by BushBlock updatePostPlacement if dirt support failed?)`,
                );
            },
        },
    );
}

export function registerFireflyBushEmissionTests(): void {
    GameTest.register("LightingTests", "light_firefly_bush_emits_2", fireflyBushEmitsTwo)
        .structureName("gametests:light_box")
        .maxTicks(120);
}
