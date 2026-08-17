// 草方块蔓延与退化集成测试：验证 SpreadableSnowyDirtBlock 的 randomTick 蔓延/退化门槛
// （对齐 wiki 草方块扩散与转变章节）。
//
// wiki tech_草方块.txt#扩散（:56-68）：
//   "得到随机刻时，草方块会以自身所在位置作为扩散中心，在该中心3×5×3的范围内随机挑四次方块尝试传播。
//    要使泥土接受附近草方块的扩散，必须达到下列要求：
//    - 草方块的上方必须至少有9级的亮度，泥土则不需要。
//    - 泥土上方不能被任何下表面遮挡形状完整的方块覆盖，但可以是雪。
//    - 泥土上方不能是水或熔岩。"
// wiki tech_草方块.txt#转变（:70-77）：
//   "如果草方块满足以下任何条件，草方块将会在收到随机刻后变为泥土：
//    - 被任何下表面遮挡形状完整的方块覆盖。
//    - 亮度小于9并且上方是水或熔岩源方块（JE）。"
//
// Cubium 实现（SpreadableSnowyDirtBlock.cpp:66-111 randomTick）：
//   - isSnowyConditions（:113-144）判退化：上方方块经 LightEngineUtils::getLightBlockInto（LightEngineUtils.cpp
//     :165-180）计算阻挡，>=MAX_LIGHT_LEVEL(15) 即满阻挡（facesHaveOcclusion 两端 opacity=15 + 完整面遮挡
//     返回 true → getLightBlockInto 返回 16>15）→ isSnowyConditions 返回 false → 退化成泥土。
//   - 满足 isSnowyConditions 时取草方块上方光照 max(skyLight,blockLight)>=GRASS_SPREAD_LIGHT_THRESHOLD(9)
//     （:75-79）才尝试向4个随机位置（3×5×3 范围）的泥土蔓延（:83-108）。
//   - 蔓延目标须为 DIRT 且 isSnowyAndNotUnderwater（:146-165，再判 isSnowyConditions + 上方无流体）。
// GRASS_SPREAD_LIGHT_THRESHOLD=9（Constants.hpp:60）。
//
// ============================ 确定性方案：调高 randomTickSpeed ============================
// 草蔓延由随机刻驱动。与作物光照门槛测试同源（见 CropLightThresholdTests.ts 与记忆
// randomtick-threshold-test-via-gamerule-speedup）。默认 randomTickSpeed=3 时草方块格每 tick 被选中
// 概率仅 3/4096≈0.073%，短时间命中概率极低，蔓延无从验证。测试开头用
// SimulatedPlayer.chat("/gamerule randomTickSpeed 1000") 调高使草方块每 tick 命中概率≈24.4%，
// 120 tick 内至少命中一次概率≈100%。草方块命中后每次向4个随机位置尝试蔓延，3×5×3=45 格范围内单格
// 命中概率 4/45≈8.9%/次，多次命中后周围泥土被蔓延概率→1。
//
// MinecraftStructurePlacer 为测试结构区域加 forced chunk ticket，chunk 常驻，tickEnvironment 必遍历到
// 测试 chunk，randomTick 覆盖草方块格。
//
// chat 返回值不 assert：Cubium chat 返回 int（命令返回值），基岩 BDS chat 返回 void（发消息语义不执行
// 命令），两端 chat 语义不同，用 chat 执行 /gamerule 的测试基岩侧 one-sided（同 WeatherSkyDarkeningTests）。
// 测试1（光照≥9 蔓延）泥土变草方块证明 randomTickSpeed 调高生效，反证测试3（黑暗不蔓延）的泥土不变
// 是门槛拦截而非没命中。
//
// ============================ 测试设计（light_box 7×7×7 封顶盒，skyLight=0）============================
// light_box 内部 x,z∈[1,5] y∈[1,5] air，y=0/y=6 stone。封顶隔绝天空光，blockLight 是唯一光源。
// (3,1,3) 草方块（setBlockType 强放存活，light_box 地板 stone，草方块在 stone 上非真实支撑但强放绕过
//   isValidPosition；SnowyDirtBlock::updatePostPlacement 只同步 SNOWY 状态无自毁，放置稳定）。
// 周围4格泥土 (2,1,3)/(4,1,3)/(3,1,2)/(3,1,4)（蔓延目标，3×5×3 范围内）。
//
// 测试1 grass_spreads_to_dirt_in_light（光照≥9 蔓延）：
//   草方块正上方 (3,2,3) 放 glowstone(15) 提供方块光，opacity=0 不触发退化，草方块上方光照=15≥9。
//   泥土上方距 glowstone 1格水平，光照衰减1→14≥9 且无遮挡。调高 randomTickSpeed 后等待足够 tick，
//   断言周围4格中至少1格变 grass_block（蔓延成功）。
//
// 测试2 grass_decays_when_covered（被遮挡退化为泥土）：
//   草方块正上方 (3,2,3) 放 stone（opacity=15 完整方块下表面遮挡），isSnowyConditions 判满阻挡返回
//   false → randomTick 退化成 dirt。调高 randomTickSpeed 后等待，断言 (3,1,3) 变 dirt（退化）。
//
// 测试3 grass_does_not_spread_in_dark（黑暗不蔓延，门槛上界）：
//   light_box 封顶无光源，草方块上方光照=0<9 不蔓延，但草方块自身满足 isSnowyConditions（上方 air 无
//   遮挡）不退化。周围4格泥土调高 randomTickSpeed 后等待，断言全为 dirt（不变草方块）。
//   配对测试1作门槛旁证：测试1蔓延成功证明 randomTickSpeed 调高生效，反证测试3的泥土不变是光照<9
//   门槛拦截而非没命中。
//   守卫：断言草方块 (3,1,3) 仍为 grass_block（非退化）+ 周围泥土 blockLight===0 确认黑暗（仅 Cubium 侧）。
//
// ============================ 排除项（不写测试）============================
// - 雪层覆盖（一层雪蔓延条件/多层雪退化）：需精确控制雪层数 LAYERS state，且 Cubium SnowBlock 的雪层
//   state 体系与基岩存在差异，价值有限，跳过。TODO: 待需要时补充雪层覆盖场景。
// - 上方水/熔岩退化（JE 亮度<9+水源）：需布置流体且控制光照，与基岩 BE 行为不一致（BE 不要求亮度<9），
//   按准则不为 JE/BE 不一致行为写测试，跳过。
// - 蔓延越过空隙到远处泥土：3×5×3 范围内无连接要求是 vanilla 行为，但远距离确定性构造复杂，且核心
//   「光照≥9 蔓延」已由测试1覆盖，跳过。
//
// ============================ 跨服务端对比 ============================
// - /gamerule randomTickSpeed、SimulatedPlayer.chat、BlockPermutation 在 Cubium 侧可用。基岩 BDS
//   SimulatedPlayer.chat 是发消息语义（void，不执行命令），本组用 chat 执行 /gamerule 的测试基岩侧无法跑
//   （one-sided，同 WeatherSkyDarkeningTests.ts）。
// - blockLight/skyLight 是 Cubium 专有（基岩 Block 无此属性），黑暗环境断言仅 Cubium 侧判定。
// - 草方块/泥土 typeId 两端一致，蔓延（泥土→草方块）与退化（草方块→泥土）行为两端一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_草方块.txt#扩散（3×5×3范围4次尝试，草上方光照≥9，泥土上方无遮挡无水）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_草方块.txt#转变（被遮挡形状完整方块覆盖退化为泥土）
// Ref: SpreadableSnowyDirtBlock.cpp:66-111（randomTick 蔓延/退化）、:113-144（isSnowyConditions 退化判定）
// Ref: LightEngineUtils.cpp:165-180（getLightBlockInto）、:102-143（facesHaveOcclusion 两端 opacity=15 完整遮挡）
// Ref: BaseBlocks.cpp:207-212（grass_block/dirt 注册，EARTH 材质不透明）、:287-288（glowstone GLASS 材质 lightLevel15 opacity0）
// Ref: Constants.hpp:60（GRASS_SPREAD_LIGHT_THRESHOLD=9）、:55（MAX_LIGHT_LEVEL=15）
// Ref: CropLightThresholdTests.ts（randomTickSpeed 调高确定性方案范式）、WeatherSkyDarkeningTests.ts（chat 执行命令 one-sided）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// 草方块及周围4格泥土（light_box 内部坐标）。
const GRASS = { x: 3, y: 1, z: 3 };
const DIRT_NEIGHBORS = [
    { x: 2, y: 1, z: 3 },
    { x: 4, y: 1, z: 3 },
    { x: 3, y: 1, z: 2 },
    { x: 3, y: 1, z: 4 },
];
// 草方块正上方位置（放光源或遮挡方块）。
const ABOVE = { x: 3, y: 2, z: 3 };

// 调高 randomTickSpeed 使草方块格在数 tick 内被随机刻确定性命中。
// 1000 使单格每 tick 命中概率≈24.4%，120 tick 内至少命中一次概率≈100%。light_box 石墙隔离 +
// 内部仅 grass_block/dirt/glowstone/stone/air，randomTick 副作用可控。
const HIGH_RANDOM_TICK_SPEED = "1000";

// 读取方块 typeId，缺失返回 undefined。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string | undefined {
    const block = test.getBlock(pos);
    return block?.typeId;
}

// 统计周围4格泥土中有多少已变 grass_block（蔓延成功数）。
function countSpreadGrass(test: Test): number {
    let count = 0;
    for (const pos of DIRT_NEIGHBORS) {
        if (getTypeId(test, pos) === "minecraft:grass_block") {
            ++count;
        }
    }
    return count;
}

// 布置草方块 + 周围4格泥土（light_box 地板为 stone，强放绕过支撑检查）。
function placeGrassAndDirt(test: Test): void {
    test.setBlockType("minecraft:grass_block", GRASS);
    for (const pos of DIRT_NEIGHBORS) {
        test.setBlockType("minecraft:dirt", pos);
    }
}

// 调高 randomTickSpeed（SimulatedPlayer 创造模式权限2 执行 /gamerule）。不 assert chat 返回值。
function raiseRandomTickSpeed(test: Test): void {
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "farmer");
    player.chat(`/gamerule randomTickSpeed ${HIGH_RANDOM_TICK_SPEED}`);
}

// 光照≥9 时草方块向周围泥土蔓延（wiki 扩散：草上方光照≥9 才蔓延）。
// 草方块正上方放 glowstone(15) 提供方块光，opacity=0 不触发退化，草上方光照=15≥9。调高 randomTickSpeed
// 后等待，断言周围4格中至少1格变 grass_block（蔓延成功）。
function grassSpreadsToDirtInLight(test: Test): void {
    placeGrassAndDirt(test);
    test.setBlockType("minecraft:glowstone", ABOVE);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 蔓延成功：周围至少1格泥土变 grass_block。
            return countSpreadGrass(test) >= 1;
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 160,
            onTimeout: () => {
                const count = countSpreadGrass(test);
                test.assert(
                    false,
                    `grass spread in light: expected >=1 neighbor dirt->grass, got ${count}/4 ` +
                        `(grass=${getTypeId(test, GRASS)} above=${getTypeId(test, ABOVE)}; ` +
                        `if 0, randomTickSpeed may not be raised or GRASS_SPREAD_LIGHT_THRESHOLD>=9 check missing)`,
                );
            },
        },
    );
}

// 被下表面遮挡形状完整的方块覆盖时草方块退化为泥土（wiki 转变：被遮挡退化）。
// 草方块正上方放 stone（opacity=15 完整方块），isSnowyConditions 判满阻挡返回 false → randomTick 退化。
// 调高 randomTickSpeed 后等待，断言 (3,1,3) 变 dirt（退化）。
function grassDecaysWhenCovered(test: Test): void {
    placeGrassAndDirt(test);
    test.setBlockType("minecraft:stone", ABOVE);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 退化成功：草方块格变 dirt。
            return getTypeId(test, GRASS) === "minecraft:dirt";
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 120,
            onTimeout: () => {
                const type = getTypeId(test, GRASS);
                test.assert(
                    false,
                    `grass decay when covered: expected grass->dirt, got ${type} ` +
                        `(above=${getTypeId(test, ABOVE)} should be stone; ` +
                        `if still grass_block, isSnowyConditions 满阻挡判定 may not trigger decay)`,
                );
            },
        },
    );
}

// 黑暗中草方块不向周围泥土蔓延（光照门槛上界，光照0<9 不蔓延）。
// light_box 封顶无光源，草方块上方光照=0<9 不蔓延，但草方块自身上方 air 无遮挡满足 isSnowyConditions 不退化。
// 调高 randomTickSpeed 后等待，断言周围4格泥土全为 dirt（不变草方块）+ 草方块仍 grass_block（不退化）。
// 守卫：草方块处 blockLight===0 确认黑暗环境（仅 Cubium 侧判定）。配对测试1作门槛旁证。
function grassDoesNotSpreadInDark(test: Test): void {
    placeGrassAndDirt(test);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 黑暗环境守卫：草方块处光照必须为0（仅 Cubium 侧 blockLight/skyLight 可读，基岩侧跳过）。
            const grassBlock = test.getBlock(GRASS);
            const blockLight = (grassBlock as unknown as { blockLight?: number })?.blockLight;
            const skyLight = (grassBlock as unknown as { skyLight?: number })?.skyLight;
            if (typeof blockLight === "number" && typeof skyLight === "number") {
                if (blockLight !== 0 || skyLight !== 0) {
                    return false; // 环境非黑暗，等待光照重算稳定（不应发生，保守轮询）
                }
            }
            // 蔓延门槛拦截：周围4格泥土全为 dirt（无蔓延），且草方块自身不退化仍为 grass_block。
            return countSpreadGrass(test) === 0 && getTypeId(test, GRASS) === "minecraft:grass_block";
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 140,
            onTimeout: () => {
                const count = countSpreadGrass(test);
                const grassType = getTypeId(test, GRASS);
                const block = test.getBlock(GRASS);
                const blockLight = (block as unknown as { blockLight?: number })?.blockLight ?? -1;
                const skyLight = (block as unknown as { skyLight?: number })?.skyLight ?? -1;
                test.assert(
                    false,
                    `grass no-spread in dark: expected 0 neighbors grass + grass_block intact, ` +
                        `got ${count}/4 spread, grass=${grassType} ` +
                        `blockLight=${blockLight} skyLight=${skyLight} (both should be 0; ` +
                        `if count>0, GRASS_SPREAD_LIGHT_THRESHOLD>=9 check may be missing in randomTick)`,
                );
            },
        },
    );
}

export function registerGrassSpreadTests(): void {
    GameTest.register("BlockBehaviorTests", "grass_spreads_to_dirt_in_light", grassSpreadsToDirtInLight)
        .structureName("gametests:light_box")
        .maxTicks(280);
    GameTest.register("BlockBehaviorTests", "grass_decays_when_covered", grassDecaysWhenCovered)
        .structureName("gametests:light_box")
        .maxTicks(200);
    GameTest.register("BlockBehaviorTests", "grass_does_not_spread_in_dark", grassDoesNotSpreadInDark)
        .structureName("gametests:light_box")
        .maxTicks(240);
}
