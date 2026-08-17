// 雪层融化集成测试：验证 SnowBlock::randomTick 的方块光照门槛融化行为
// （对齐 wiki 雪融化章节）。
//
// wiki tech_雪.txt#融化（:105-110）：
//   "附近有加热块或者方块亮度不低于12都会导致积雪融化。"（:106）
//   "Java 版：除8层雪不会融化外，其他层数的雪会在收到随机刻时若方块亮度不低于12则全部融化。
//    基岩版：任何层数的雪在收到随机刻时会逐层融化。"（:108）
//   "雪融化时不生成液相，而是直接消失并转化为空气。"（:110）
//
// Cubium 实现（SnowBlock.cpp:100-127 randomTick）：
//   - 仅检查方块光照：blockLight > MELT_LIGHT_LEVEL(11) 即 blockLight >= 12 时融化（:108-110），
//     与 wiki "方块亮度不低于12" 一致；不考虑天空光照（阳光不直接融化雪）。
//   - 融化时按层数 LAYERS 掉落对应数量雪球（:113-123），最后 setBlockState 为 air（:124-125）。
//   - 无概率门限：randomTick 命中 + blockLight >= 12 即确定性融化（比草蔓延/铜氧化更干净，无 1/5 或
//     5.69% 门限）。
// ticksRandomly() 返回 true（SnowBlock.hpp:83），雪层格纳入随机刻候选。
//
// ============================ 确定性方案：调高 randomTickSpeed ============================
// 雪融化由随机刻驱动（同作物光照门槛、草蔓延、铜氧化、紫晶芽、蘑菇蔓延测试，见记忆
// randomtick-threshold-test-via-gamerule-speedup）。默认 randomTickSpeed=3 时雪层格每 tick 被选中
// 概率仅 3/4096≈0.073%，短时间命中概率极低。测试开头用 SimulatedPlayer.chat("/gamerule randomTickSpeed 1000")
// 调高使雪层格每 tick 命中概率≈24.4%，数 tick 内必然命中。
//
// 雪融化无概率门限：randomTick 命中 + blockLight>=12 即融化变 air。speed=1000 下命中期望≈24.4%/tick，
// 命中即融化，约 5 tick 内（startTick=40 留光照重算与玩家注册稳定时间）确定性融化。
// 低光照不融化：blockLight<12 时 randomTick 命中后门槛拦截直接 return（确定性逻辑，无概率）。调高
// randomTickSpeed 后多次命中都被门槛拦截，断言"不融化"确定性成立。配对测试1证明 randomTickSpeed
// 调高生效，反证测试2的"不融化"是光照门槛而非没命中。
//
// MinecraftStructurePlacer 为测试结构区域加 forced chunk ticket，chunk 常驻，randomTick 覆盖雪层格。
//
// chat 返回值不 assert：Cubium chat 返回 int，基岩 BDS chat 返回 void（发消息语义不执行命令），两端
// chat 语义不同，用 chat 执行 /gamerule 的测试基岩侧 one-sided（同 WeatherSkyDarkeningTests、GrassSpreadTests）。
//
// ============================ 测试设计（light_box 7×7×7 封顶盒，skyLight=0）============================
// light_box 文件布局：y=0/y=6 全 stone，y=1..5 内部 x,z∈[1,5] air。但 GameTest 结构放置存在
// Y 坐标偏移（helper (x,y,z) 对应文件 (x,y+1,z)，结构原点使 helper 整体下移1层），运行时 helper
// 坐标的 air 区域为 y∈[2,4] x,z∈[1,5]（helper y=1 为 stone 地板层，y=0 为结构外 bedrock 填充）。
// 见记忆 light-box-structure-placement-y-offset。
//
// 雪层需下方支撑（SnowBlock::isValidPosition 要求下方 isFaceFull(Up)）。light_box 地板 helper y=1
// 全 stone（完整方块，isFaceFull(Up)=true 满足支撑），故雪层放 helper y=2（air 区域第一层，紧贴
// stone 地板），下方 y=1 stone 稳定支撑。
// 雪层 setBlockType 强放（flags=3 绕过 isValidPosition），但即便走支撑检查，stone 地板也满足，放置稳定。
// SnowBlock 无 onBlockAdded 重写，放置自身不立即自毁（同 SnowTests.ts 经验）。
//
// 雪层位置 SNOW=(3,2,3)。光源 GLOWSTONE=(4,2,3)（雪层水平邻居，air 区域内）。
// glowstone 光照等级15、不透明度0，方块光水平传播1格衰减1→雪层处 blockLight=14>=12 融化。
//
// 测试1 snow_melts_in_block_light（方块光>=12 融化变 air）：
//   雪层 (3,2,3) + 光源 glowstone (4,2,3)。雪层 blockLight=14>=12。调高 randomTickSpeed 后等待，
//   断言雪层变 air（融化成功）。
//   守卫：雪层处 blockLight>=12 确认亮环境（仅 Cubium 侧判定）。
//
// 测试2 snow_does_not_melt_in_low_light（方块光<12 不融化）：
//   雪层 (3,2,3) 无光源（light_box 封顶黑暗，blockLight=0<12）。调高 randomTickSpeed 后等待，
//   断言雪层仍为 snow（不融化）。配对测试1证明 randomTickSpeed 调高生效，反证此处不融化是光照门槛。
//   守卫：雪层处 blockLight<12 确认暗环境（仅 Cubium 侧判定）。
//
// ============================ 排除项（不写测试）============================
// - 8层雪不融化（JE 独有）：wiki :108 明确 JE "除8层雪不会融化"，BE "任何层数逐层融化"，两端不一致，
//   按准则不为 JE/BE 不一致行为写测试，跳过。
// - 掉落雪球数量：wiki :74 JE 每层1个雪球，BE 1-3层1个/4-5层2个/6-7层3个/8层4个，两端不一致，跳过。
//   本测试仅断言方块变 air（核心融化行为），掉落物不断言。
// - 加热块融化：加热块（heatBlock）是基岩/教育版独有方块，Cubium 主世界无此方块，跳过。
// - 干燥生物群系融化（BE）：wiki :106 "BE 无论亮度如何，雪都会在干燥的生物群系融化"，BE 独有且依赖
//   生物群系温度系统，跳过。
// - 逐层融化（BE）：wiki :108 BE 逐层融化（layers 递减），JE 全部融化（直接变 air）。Cubium 实现 JE
//   语义（直接变 air）。1层雪两端都变 air（JE 全部融化=air，BE 1层融完=air），本测试用1层雪两端一致。
//   多层雪 JE 直接变 air / BE 逐层递减不一致，不为多层雪写测试。
//
// ============================ 跨服务端对比 ============================
// - /gamerule randomTickSpeed、SimulatedPlayer.chat、blockLight 在 Cubium 侧可用。基岩 BDS
//   SimulatedPlayer.chat 是发消息语义（void，不执行命令），本组用 chat 执行 /gamerule 的测试基岩侧
//   无法跑（one-sided，同 WeatherSkyDarkeningTests.ts、GrassSpreadTests.ts）。
// - blockLight/skyLight 是 Cubium 专有（基岩 Block 无此属性），亮/暗环境守卫仅 Cubium 侧判定。
// - 雪层 typeId（snow）两端一致（1.21.11 JE/BE 统一为 snow，见 wiki :167 历史 1.13 扁平化更名）。
//   1层雪方块光>=12 融化变 air、方块光<12 不融化两端一致（1.21.11 特性）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_雪.txt#融化（:106 方块亮度>=12融化；:108 JE全部融化/BE逐层融化，8层JE不融化；:110 直接变air）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_雪.txt#破坏（:74 JE每层1雪球/BE分层掉落，两端不一致故不测掉落）
// Ref: SnowBlock.cpp:100-127（randomTick blockLight>11即>=12融化变air，无概率门限，掉落雪球后setBlockState air）
// Ref: SnowBlock.hpp:83（ticksRandomly 返回 true，纳入随机刻候选）
// Ref: SnowBlock.cpp:129-159（isValidPosition 下方 isFaceFull(Up) 支撑判定，stone 地板满足）
// Ref: GrassSpreadTests.ts / MushroomSpreadTests.ts / CopperOxidationTests.ts（randomTickSpeed 调高范式 + light_box 坐标偏移）
// Ref: WeatherSkyDarkeningTests.ts（chat 执行命令 one-sided）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// 雪层位置（light_box 运行时 air 区域第一层 y=2，紧贴 y=1 stone 地板满足支撑）。
const SNOW = { x: 3, y: 2, z: 3 };
// 光源位置（雪层水平邻居，air 区域内，glowstone 光照15 传播1格衰减1→雪层 blockLight=14>=12）。
const GLOWSTONE = { x: 4, y: 2, z: 3 };

// 调高 randomTickSpeed 使雪层格在数 tick 内被随机刻确定性命中。
// 1000 使单格每 tick 命中概率≈24.4%。雪融化无概率门限，命中即融化，约 5 tick 内确定性融化。
const HIGH_RANDOM_TICK_SPEED = "1000";

// 读取方块 typeId，缺失返回 undefined。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string | undefined {
    const block = test.getBlock(pos);
    return block?.typeId;
}

// 调高 randomTickSpeed（SimulatedPlayer 创造模式权限2 执行 /gamerule）。不 assert chat 返回值。
// 玩家放 air 区域 (1,2,1)（运行时 air 区域 y∈[2,4]），避免卡在 stone 地板层。
function raiseRandomTickSpeed(test: Test): void {
    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    player.chat(`/gamerule randomTickSpeed ${HIGH_RANDOM_TICK_SPEED}`);
}

// 方块光照不低于12时雪层融化变空气（wiki 融化：方块亮度>=12融化，直接变air）。
// 雪层 (3,2,3) + 光源 glowstone (4,2,3)。雪层 blockLight=14>=12。调高 randomTickSpeed 后等待，
// 断言雪层变 air（融化成功）。
// 守卫：雪层处 blockLight>=12 确认亮环境（仅 Cubium 侧判定）。
function snowMeltsInBlockLight(test: Test): void {
    test.setBlockType("minecraft:snow", SNOW);
    test.setBlockType("minecraft:glowstone", GLOWSTONE);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 亮环境守卫：雪层处 blockLight 必须>=12（仅 Cubium 侧 blockLight 可读，基岩侧跳过）。
            const block = test.getBlock(SNOW);
            const blockLight = (block as unknown as { blockLight?: number })?.blockLight;
            if (typeof blockLight === "number") {
                if (blockLight < 12) {
                    return false; // 环境未达融化门槛，等待光照重算稳定（不应发生，保守轮询）
                }
            }
            // 融化成功：雪层格变 air。
            return getTypeId(test, SNOW) === "minecraft:air";
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 140,
            onTimeout: () => {
                const type = getTypeId(test, SNOW);
                const block = test.getBlock(SNOW);
                const blockLight = (block as unknown as { blockLight?: number })?.blockLight ?? -1;
                test.assert(
                    false,
                    `snow melt in block light: expected snow->air, got ${type} ` +
                        `(glowstone=${getTypeId(test, GLOWSTONE)} blockLight=${blockLight} should be >=12; ` +
                        `if still snow, randomTickSpeed may not be raised or ` +
                        `MELT_LIGHT_LEVEL blockLight>11 check may be missing in randomTick)`,
                );
            },
        },
    );
}

// 方块光照低于12时雪层不融化（wiki 融化：方块亮度<12不融化）。
// 雪层 (3,2,3) 无光源（light_box 封顶黑暗，blockLight=0<12）。调高 randomTickSpeed 后等待，
// 断言雪层仍为 snow（不融化）。配对测试1证明 randomTickSpeed 调高生效，反证此处不融化是光照门槛。
// 守卫：雪层处 blockLight<12 确认暗环境（仅 Cubium 侧判定）。
function snowDoesNotMeltInLowLight(test: Test): void {
    test.setBlockType("minecraft:snow", SNOW);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 暗环境守卫：雪层处 blockLight 必须<12（仅 Cubium 侧 blockLight 可读，基岩侧跳过）。
            const block = test.getBlock(SNOW);
            const blockLight = (block as unknown as { blockLight?: number })?.blockLight;
            if (typeof blockLight === "number") {
                if (blockLight >= 12) {
                    return false; // 环境达融化门槛，等待光照重算稳定（不应发生，保守轮询）
                }
            }
            // 光照门槛拦截：雪层仍为 snow（不融化变 air）。
            return getTypeId(test, SNOW) === "minecraft:snow";
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 140,
            onTimeout: () => {
                const type = getTypeId(test, SNOW);
                const block = test.getBlock(SNOW);
                const blockLight = (block as unknown as { blockLight?: number })?.blockLight ?? -1;
                test.assert(
                    false,
                    `snow no-melt in low light: expected snow, got ${type} ` +
                        `(blockLight=${blockLight} should be <12; ` +
                        `if air, MELT_LIGHT_LEVEL blockLight>11 check may be missing or ` +
                        `threshold inverted in randomTick)`,
                );
            },
        },
    );
}

export function registerSnowMeltTests(): void {
    GameTest.register("BlockBehaviorTests", "snow_melts_in_block_light", snowMeltsInBlockLight)
        .structureName("gametests:light_box")
        .maxTicks(220);
    GameTest.register("BlockBehaviorTests", "snow_does_not_melt_in_low_light", snowDoesNotMeltInLowLight)
        .structureName("gametests:light_box")
        .maxTicks(220);
}
