// 冰融化集成测试：验证 IceBlock::randomTick 的方块光照门槛融化行为
// （对齐 wiki 储水章节「方块光使冰融化变水源」）。
//
// wiki tech_冰.txt#储水（:57-60）：
//   "正下方为固体方块或液体方块的冰被玩家使用非精准采集的工具破坏时会产生水源。"
//   "由方块产生的、亮度等级高于11的光照射在冰的任意面上...会使之融化而变成水源，
//    且无论冰下方是否存在方块都会如此，但阳光并不会使冰融化。"（:58）
//   "冰在下界不会产生水。"（:60）
//
// 注：wiki 文字「亮度等级高于11」（>11 即 >=12）与 vanilla 代码门槛存在描述粒度差异。
// vanilla IceBlock.randomTick（net.minecraft.world.level.block.IceBlock:52-56）实际条件为
// getBrightness(BLOCK, pos) > 11 - getLightBlock()。冰继承 HalfTransparentBlock，
// getLightBlock 默认为 1（非 solidRender 且非 useShapeForLightOcclusion）→ 门槛 > 10 即 >= 11。
// Cubium IceBlock::randomTick（IceBlock.cpp:135-150）用 blockLight > MELT_LIGHT_LEVEL(11) - opacity，
// 冰 Material::ICE.opaque(false)（Material.cpp:159-162）使 Block::getOpacity（Block.cpp:401-419）
// 对完整冰方块返回 1 → 门槛同样 > 10 即 >= 11。**Cubium 与 vanilla 门槛一致（blockLight >= 11）**。
// 本测试用 bl13（远高于门槛，融化）/ bl0（远低于门槛，不融化）两端实测值，避开边界，不依赖
// "11 还是 12" 的文字粒度。
//
// ============================ 确定性方案：调高 randomTickSpeed ============================
// 冰融化由随机刻驱动（同雪融化、作物光照门槛、草蔓延、铜氧化测试，见记忆
// randomtick-threshold-test-via-gamerule-speedup）。默认 randomTickSpeed=3 时冰格每 tick 被选中
// 概率仅 3/4096≈0.073%，短时间命中概率极低。测试开头用 SimulatedPlayer.chat("/gamerule randomTickSpeed 1000")
// 调高使冰格每 tick 命中概率≈24.4%，数 tick 内必然命中。
//
// 冰融化无概率门限：randomTick 命中 + blockLight>=11 即融化变水。speed=1000 下命中期望≈24.4%/tick，
// 命中即融化，约 5 tick 内（startTick=40 留光照重算与玩家注册稳定时间）确定性融化。
// 低光照不融化：blockLight<11 时 randomTick 命中后门槛拦截直接 return（确定性逻辑，无概率）。调高
// randomTickSpeed 后多次命中都被门槛拦截，断言"不融化"确定性成立。配对测试1证明 randomTickSpeed
// 调高生效，反证测试2的"不融化"是光照门槛而非没命中。
//
// MinecraftStructurePlacer 为测试结构区域加 forced chunk ticket，chunk 常驻，randomTick 覆盖冰格。
//
// chat 返回值不 assert：Cubium chat 返回 int，基岩 BDS chat 返回 void（发消息语义不执行命令），两端
// chat 语义不同，用 chat 执行 /gamerule 的测试基岩侧 one-sided（同 SnowMeltTests、WeatherSkyDarkeningTests）。
//
// ============================ 测试设计（light_box 7×7×7 封顶盒，skyLight=0）============================
// light_box 文件布局：y=0/y=6 全 stone，y=1..5 内部 x,z∈[1,5] air。GameTest 结构放置存在 Y 坐标偏移
// （helper (x,y,z) 对应文件 (x,y+1,z)，结构原点使 helper 整体下移1层），运行时 helper 坐标的 air 区域
// 为 y∈[2,4] x,z∈[1,5]（helper y=1 为 stone 地板层，y=0 为结构外 bedrock 填充）。见记忆
// light-box-structure-placement-y-offset。
//
// 冰放 helper y=2（air 区域第一层），下方 y=1 为 stone 地板（完整固体方块）。冰融化变水时，meltIce
// 在主世界（非 ultraWarm）调 getWaterState() 返回水源（IceBlock.cpp:90-94）。融化为 randomTick 路径，
// 不依赖"正下方为方块"（那是 playerDestroy 破坏路径 handleIceBreak 的条件，IceBlock.cpp:96-110）。
// 融化路径直接变水源，与下方是否有方块无关（wiki :58 "无论冰下方是否存在方块都会如此"）。
//
// 冰位置 ICE=(3,2,3)。光源 GLOWSTONE=(4,2,3)（冰水平邻居，air 区域内）。
// glowstone 光照等级15、不透明度0，方块光水平传播1格衰减1→冰处 blockLight=14>=11 融化。
// （诊断已实测 ice(3,2,3) blockLight=13，因 glowstone 在对角时衰减更多，但 >=11 满足门槛。本测试
// glowstone 与冰正交相邻，blockLight=14，仍 >=11。）
//
// 测试1 ice_melts_in_block_light（方块光>=11 融化变 water）：
//   冰 (3,2,3) + 光源 glowstone (4,2,3)。冰 blockLight=14>=11。调高 randomTickSpeed 后等待，
//   断言冰格变 water（融化成功）。
//   守卫：冰处 blockLight>=11 确认亮环境（仅 Cubium 侧判定）。
//
// 测试2 ice_does_not_melt_in_low_light（方块光<11 不融化）：
//   冰 (3,2,3) 无光源（light_box 封顶黑暗，blockLight=0<11）。调高 randomTickSpeed 后等待，
//   断言冰格仍为 ice（不融化）。配对测试1证明 randomTickSpeed 调高生效，反证此处不融化是光照门槛。
//   守卫：冰处 blockLight<11 确认暗环境（仅 Cubium 侧判定）。
//
// ============================ 排除项（不写测试）============================
// - 破坏变水（playerDestroy/handleIceBreak）：wiki :52/58 破坏路径需玩家用非精准采集工具挖掘，
//   GameTest 脚本难以精确模拟玩家挖掘工具附魔，且破坏路径与融化路径（randomTick）是两套独立逻辑。
//   本测试聚焦融化（randomTick）路径，破坏变水留待玩家交互测试体系。
// - 下界不产生水（ultraWarm）：meltIce 在 isUltraWarm() 时变 air（IceBlock.cpp:92）。需切到下界维度
//   跑测试，GameTest 结构在主世界，跨维度测试成本高，且 wiki :60 明确下界由环境属性控制，跳过。
// - 加热块融化：加热块（heatBlock）是基岩/教育版独有方块（wiki :58 {{only|be|ee}}），Cubium 主世界
//   无此方块，跳过。
// - 阳光不融化：wiki :58 "阳光并不会使冰融化"。Cubium randomTick 仅查 blockLight 不查 skyLight，
//   阳光（skyLight）不影响融化门槛——但 skyLight 进入冰格会通过 getMaxLocalRawBrightness 影响霜冰
//   （FrostedIceBlock），普通冰 randomTick 仅 blockLight。light_box 封顶 skyLight=0，本测试不涉及
//   skyLight 融化判定（冰 randomTick 本就不查 skyLight）。阳光不融化是"冰不因 skyLight 融化"的推论，
//   难以在 light_box（无天空光）中直接验证，跳过。
// - 冰弹结冰（BE/EE 独有，wiki :41-42）：基岩/教育版独有方块冰弹，Cubium 无此物品，跳过。
// - 结冰生成（水源结冰变冰）：wiki :30-38 水源在寒冷群系 blockLight<10 结冰。这是"水→冰"反向过程，
//   与冰融化（冰→水）不同，且依赖生物群系温度系统，属另一测试主题，不在本融化测试范围。
//
// ============================ 跨服务端对比 ============================
// - /gamerule randomTickSpeed、SimulatedPlayer.chat、blockLight 在 Cubium 侧可用。基岩 BDS
//   SimulatedPlayer.chat 是发消息语义（void，不执行命令），本组用 chat 执行 /gamerule 的测试基岩侧
//   无法跑（one-sided，同 SnowMeltTests.ts、WeatherSkyDarkeningTests.ts）。
// - blockLight/skyLight 是 Cubium 专有（基岩 Block 无此属性），亮/暗环境守卫仅 Cubium 侧判定。
// - 冰 typeId（ice）与水 typeId（water）两端一致（1.21.11 JE/BE 统一）。冰方块光>=11 融化变水、
//   方块光<11 不融化两端一致（1.21.11 特性）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_冰.txt#储水（:58 亮度>11的光使冰融化变水源，阳光不融化；:60 下界不产生水）
// Ref: IceBlock.cpp:135-150（randomTick blockLight>11-opacity即>=11融化变水，无概率门限，meltIce主世界变water）
// Ref: IceBlock.cpp:90-94（meltIce isUltraWarm?air:water，主世界变水源）
// Ref: IceBlock.hpp:75（ticksRandomly 返回 true，纳入随机刻候选）
// Ref: net.minecraft.world.level.block.IceBlock#randomTick（getBrightness(BLOCK)>11-getLightBlock，冰 getLightBlock=1，门槛 >=11）
// Ref: SnowMeltTests.ts（randomTickSpeed 调高范式 + light_box 坐标偏移 + blockLight 守卫）
// Ref: WeatherSkyDarkeningTests.ts（chat 执行命令 one-sided）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// 冰位置（light_box 运行时 air 区域第一层 y=2，紧贴 y=1 stone 地板）。
const ICE = { x: 3, y: 2, z: 3 };
// 光源位置（冰水平邻居，air 区域内，glowstone 光照15 传播1格衰减1→冰 blockLight=14>=11）。
const GLOWSTONE = { x: 4, y: 2, z: 3 };

// 调高 randomTickSpeed 使冰格在数 tick 内被随机刻确定性命中。
// 1000 使单格每 tick 命中概率≈24.4%。冰融化无概率门限，命中即融化，约 5 tick 内确定性融化。
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

// 方块光照不低于11时冰融化变水源（wiki 储水：亮度>11的光使冰融化变水，主世界变水源）。
// 冰 (3,2,3) + 光源 glowstone (4,2,3)。冰 blockLight=14>=11。调高 randomTickSpeed 后等待，
// 断言冰格变 water（融化成功）。
// 守卫：冰处 blockLight>=11 确认亮环境（仅 Cubium 侧判定）。
function iceMeltsInBlockLight(test: Test): void {
    test.setBlockType("minecraft:ice", ICE);
    test.setBlockType("minecraft:glowstone", GLOWSTONE);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 亮环境守卫：冰处 blockLight 必须>=11（仅 Cubium 侧 blockLight 可读，基岩侧跳过）。
            const block = test.getBlock(ICE);
            const blockLight = (block as unknown as { blockLight?: number })?.blockLight;
            if (typeof blockLight === "number") {
                if (blockLight < 11) {
                    return false; // 环境未达融化门槛，等待光照重算稳定（不应发生，保守轮询）
                }
            }
            // 融化成功：冰格变 water（主世界融化变水源）。
            return getTypeId(test, ICE) === "minecraft:water";
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 140,
            onTimeout: () => {
                const type = getTypeId(test, ICE);
                const block = test.getBlock(ICE);
                const blockLight = (block as unknown as { blockLight?: number })?.blockLight ?? -1;
                test.assert(
                    false,
                    `ice melt in block light: expected ice->water, got ${type} ` +
                        `(glowstone=${getTypeId(test, GLOWSTONE)} blockLight=${blockLight} should be >=11; ` +
                        `if still ice, randomTickSpeed may not be raised or ` +
                        `blockLight>11-opacity check may be missing in randomTick)`,
                );
            },
        },
    );
}

// 方块光照低于11时冰不融化（wiki 储水：方块光不足时不融化）。
// 冰 (3,2,3) 无光源（light_box 封顶黑暗，blockLight=0<11）。调高 randomTickSpeed 后等待，
// 断言冰格仍为 ice（不融化）。配对测试1证明 randomTickSpeed 调高生效，反证此处不融化是光照门槛。
// 守卫：冰处 blockLight<11 确认暗环境（仅 Cubium 侧判定）。
function iceDoesNotMeltInLowLight(test: Test): void {
    test.setBlockType("minecraft:ice", ICE);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 暗环境守卫：冰处 blockLight 必须<11（仅 Cubium 侧 blockLight 可读，基岩侧跳过）。
            const block = test.getBlock(ICE);
            const blockLight = (block as unknown as { blockLight?: number })?.blockLight;
            if (typeof blockLight === "number") {
                if (blockLight >= 11) {
                    return false; // 环境达融化门槛，等待光照重算稳定（不应发生，保守轮询）
                }
            }
            // 光照门槛拦截：冰格仍为 ice（不融化变 water）。
            return getTypeId(test, ICE) === "minecraft:ice";
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 140,
            onTimeout: () => {
                const type = getTypeId(test, ICE);
                const block = test.getBlock(ICE);
                const blockLight = (block as unknown as { blockLight?: number })?.blockLight ?? -1;
                test.assert(
                    false,
                    `ice no-melt in low light: expected ice, got ${type} ` +
                        `(blockLight=${blockLight} should be <11; ` +
                        `if water, blockLight>11-opacity check may be missing or ` +
                        `threshold inverted in randomTick)`,
                );
            },
        },
    );
}

export function registerIceMeltTests(): void {
    GameTest.register("BlockBehaviorTests", "ice_melts_in_block_light", iceMeltsInBlockLight)
        .structureName("gametests:light_box")
        .maxTicks(220);
    GameTest.register("BlockBehaviorTests", "ice_does_not_melt_in_low_light", iceDoesNotMeltInLowLight)
        .structureName("gametests:light_box")
        .maxTicks(220);
}
