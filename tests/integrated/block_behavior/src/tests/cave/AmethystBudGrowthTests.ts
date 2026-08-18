// 紫水晶母岩生长紫晶芽集成测试：验证 BuddingAmethystBlock::randomTick 的紫晶芽生成与升级
// （对齐 wiki 紫水晶母岩生长章节）。
//
// wiki tech_紫水晶母岩.txt#生长（:35）："紫水晶母岩接收到随机刻时，它的某个表面会有20%概率长出
// 紫晶芽，若已有紫晶芽连接至该紫水晶母岩，则会有20%的概率生长到下一阶段。只有接触到水或空气的
// 表面才有可能长出紫晶芽。调整游戏规则 randomTickSpeed 会改变紫晶芽的生长速度。"
//
// Cubium 实现（BuddingAmethystBlock.cpp:47-96 randomTick）：
//   - GROWTH_CHANCE=5，random.nextInt(5)!=0 即 1/5=20% 概率通过门限（:51-53），与 wiki 20% 一致。
//   - 随机选6方向之一（:56），邻居是 air 且非水源 → 生成 small_amethyst_bud（:67-72）。
//   - 邻居是 small/medium/large bud → 升级为下一级（:76-82）；amethyst_cluster 最高级不升级（growInto=nullptr）。
//   - 生长的芽 facing 设为生长方向（:86），水源处设 WATERLOGGED（:88-92）。
// 升级链：small_amethyst_bud → medium_amethyst_bud → large_amethyst_bud → amethyst_cluster（:71-81）。
//
// ============================ 确定性方案：调高 randomTickSpeed ============================
// 紫晶芽生长由随机刻驱动（同作物光照门槛、草蔓延、铜氧化测试，见记忆
// randomtick-threshold-test-via-gamerule-speedup）。默认 randomTickSpeed=3 时母岩格每 tick 被选中
// 概率仅 3/4096≈0.073%，生长无从验证。测试开头用 SimulatedPlayer.chat("/gamerule randomTickSpeed 1000")
// 调高使母岩格每 tick 命中概率≈24.4%。
//
// 母岩生成小芽：每次 randomTick 命中后 1/5 门限 × 6/6（6方向均 air 可生长）≈ 20%/命中。
// speed=1000 时生成期望≈4.88%/tick，120 tick 内至少生成一个 small_bud 概率≈99.7%。
// 小芽升级：手动放 small_bud 在母岩某方向，每次命中 1/5 门限 × 1/6（命中该方向）≈ 3.3%/命中，
// speed=1000 时升级期望≈0.81%/tick，200 tick 内升级概率≈80%。配 maxTick 320 + 轮询确定性足够。
//
// MinecraftStructurePlacer 为测试结构区域加 forced chunk ticket，chunk 常驻，randomTick 覆盖母岩格。
//
// chat 返回值不 assert：Cubium chat 返回 int，基岩 BDS chat 返回 void（发消息语义不执行命令），两端
// chat 语义不同，用 chat 执行 /gamerule 的测试基岩侧 one-sided（同 WeatherSkyDarkeningTests）。
// 测试1（生成小芽）证明 randomTickSpeed 调高生效。
//
// ============================ 测试设计（light_box 7×7×7 封顶盒）============================
// light_box 文件布局：y=0/y=6 全 stone，y=1..5 内部 x,z∈[1,5] air。但 GameTest 结构放置存在
// 坐标偏移（helper (x,y,z) 对应文件 (x,y+1,z)，结构原点使 helper 整体下移1层），运行时 helper
// 坐标的 air 区域为 y∈[2,4] x,z∈[1,5]（y=1 为 stone 地板层，y=0 为结构外 bedrock 填充）。
// 经诊断实测确认：helper (3,2,3)=air，(2,1,3)=stone。草蔓延/作物测试在 (3,1,3) 强放方块覆盖 stone
// 地板层可行（不依赖初始 air），但紫晶芽生长需邻居初始为 air，故母岩须放在 air 区域中心。
// 母岩 BuddingAmethystBlock 无 isValidPosition/canSurvive 重写（继承 AmethystBlock: Block），
// 放置稳定，无支撑/光照依赖，可强放任意位置。
// (3,3,3) 母岩 budding_amethyst（air 区域中心）。其6邻居全在 air 区域内：
//   (3,2,3)/(3,4,3)/(2,3,3)/(4,3,3)/(3,3,2)/(3,3,4) = air（可生长）。
//
// 测试1 budding_amethyst_grows_small_bud（母岩长出小紫晶芽）：
//   (3,3,3) 放 budding_amethyst，调高 randomTickSpeed 后等待，断言6个 air 邻居中至少1个变
//   small_amethyst_bud（20%概率×6方向，生成成功）。
//
// 测试2 amethyst_bud_grows_to_next_stage（小芽升级为中芽）：
//   (3,3,3) 放 budding_amethyst，(3,4,3) 手动放 small_amethyst_bud（facing=Down，朝向母岩）。
//   调高 randomTickSpeed 后等待，断言 (3,4,3) 变 medium_amethyst_bud（升级成功）。
//   注：升级需 randomTick 命中母岩 + 1/5门限 + 选中小芽方向(3,4,3即Up方向, 1/6)，综合概率较低，
//   maxTick 设较大（320）保证确定性。
//
// ============================ 排除项（不写测试）============================
// - amethyst_cluster 最高级不升级：与铜氧化 oxidized 不氧化同类（最高级稳定），价值有限，跳过。
//   TODO: 待需要时可补 amethyst_cluster 不升级测试。
// - 水表面生成小芽（WATERLOGGED）：Cubium 实现中水源处 air 不生成小芽（:70 !isSource 才生成），
//   与 wiki"接触水的表面可能长出"略有偏差，按准则不为偏差写测试，跳过。
// - 母岩被活塞移动破坏：需活塞推动 + 母岩破坏判定，活塞机制复杂且与红石测试重叠，跳过。
//
// ============================ 跨服务端对比 ============================
// - /gamerule randomTickSpeed、SimulatedPlayer.chat 在 Cubium 侧可用。基岩 BDS SimulatedPlayer.chat 是
//   发消息语义（void，不执行命令），本组用 chat 执行 /gamerule 的测试基岩侧无法跑（one-sided，同
//   WeatherSkyDarkeningTests.ts）。
// - 紫晶芽 typeId（small/medium/large_amethyst_bud、amethyst_cluster、budding_amethyst）两端一致，
//   20%生长概率与升级行为两端一致（1.17 引入的 1.21.11 特性，JE/BE 共有）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_紫水晶母岩.txt#生长（:35 20%概率长芽/升级，air或水表面）
// Ref: BuddingAmethystBlock.cpp:47-96（randomTick 1/5门限 + 6方向 + air生成small_bud + 芽升级）
// Ref: CaveBlocks.cpp:117-167（budding_amethyst/small/medium/large_bud/amethyst_cluster 注册）
// Ref: AmethystClusterBlock.cpp:78-92（FACING+WATERLOGGED state，默认 facing=Up）
// Ref: CropLightThresholdTests.ts / GrassSpreadTests.ts / CopperOxidationTests.ts（randomTickSpeed 调高范式）
// Ref: WeatherSkyDarkeningTests.ts（chat 执行命令 one-sided）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { BlockPermutation } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// 母岩位置（light_box 运行时 air 区域中心，见文件头坐标偏移说明）。
const BUDDING = { x: 3, y: 3, z: 3 };
// 母岩6个 air 邻居（全在 air 区域内）。用于检测小芽生成。
const AIR_NEIGHBORS = [
    { x: 3, y: 2, z: 3 },
    { x: 3, y: 4, z: 3 },
    { x: 2, y: 3, z: 3 },
    { x: 4, y: 3, z: 3 },
    { x: 3, y: 3, z: 2 },
    { x: 3, y: 3, z: 4 },
];
// 测试2 中手动放置小芽的位置（母岩上方，facing=Down 朝向母岩）。
const BUD_POS = { x: 3, y: 4, z: 3 };

// 调高 randomTickSpeed 使母岩格在数 tick 内被随机刻确定性命中。
// 1000 使单格每 tick 命中概率≈24.4%。小芽生成期望≈4.07%/tick（1/5×5/6），120 tick 内概率≈99.3%。
// light_box 石墙隔离 + 内部仅母岩/芽/stone/air，randomTick 副作用可控。
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

// 统计母岩6个 air 邻居中有多少已变 small_amethyst_bud（小芽生成数）。
function countSmallBuds(test: Test): number {
    let count = 0;
    for (const pos of AIR_NEIGHBORS) {
        if (getTypeId(test, pos) === "minecraft:small_amethyst_bud") {
            ++count;
        }
    }
    return count;
}

// 母岩在随机刻下于 air 表面长出小紫晶芽（wiki 生长：20%概率长芽，air或水表面）。
// (3,3,3) 放 budding_amethyst，调高 randomTickSpeed 后等待，断言6个 air 邻居中至少1个变 small_amethyst_bud。
function buddingAmethystGrowsSmallBud(test: Test): void {
    test.setBlockType("minecraft:budding_amethyst", BUDDING);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 生成成功：至少1个 air 邻居变 small_amethyst_bud。
            return countSmallBuds(test) >= 1;
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 160,
            onTimeout: () => {
                const count = countSmallBuds(test);
                test.assert(
                    false,
                    `budding_amethyst grow small bud: expected >=1 small_amethyst_bud, got ${count}/6 ` +
                        `(budding=${getTypeId(test, BUDDING)}; if 0, randomTickSpeed may not be raised or ` +
                        `GROWTH_CHANCE 1/5 + air-surface generation logic issue)`,
                );
            },
        },
    );
}

// 已连接的小紫晶芽在母岩随机刻下升级为中紫晶芽（wiki 生长：已有芽则20%概率升级到下一阶段）。
// (3,3,3) 放 budding_amethyst，(3,4,3) 手动放 small_amethyst_bud（facing=Down 朝母岩）。调高 randomTickSpeed
// 后等待，断言 (3,4,3) 变 medium_amethyst_bud（升级成功）。
function amethystBudGrowsToNextStage(test: Test): void {
    test.setBlockType("minecraft:budding_amethyst", BUDDING);
    // 手动放 small_amethyst_bud，facing=Down（朝向下方母岩，与自然生长方向一致）。
    const perm = BlockPermutation.resolve("minecraft:small_amethyst_bud", { facing: "down" }) as any;
    (test as unknown as {
        setBlockPermutation: (blockData: unknown, blockLocation: { x: number; y: number; z: number }) => void;
    }).setBlockPermutation(perm, BUD_POS);

    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 升级成功：small_amethyst_bud → medium_amethyst_bud。
            return getTypeId(test, BUD_POS) === "minecraft:medium_amethyst_bud";
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 320,
            onTimeout: () => {
                const type = getTypeId(test, BUD_POS);
                test.assert(
                    false,
                    `amethyst bud upgrade: expected medium_amethyst_bud, got ${type} ` +
                        `(if still small_amethyst_bud, randomTickSpeed may not be raised or ` +
                        `upgrade logic (neighbor small_bud → medium) may be missing; ` +
                        `upgrade needs hit budding + 1/5 gate + select bud direction)`,
                );
            },
        },
    );
}

export function registerAmethystBudGrowthTests(): void {
    GameTest.register("BlockBehaviorTests", "budding_amethyst_grows_small_bud", buddingAmethystGrowsSmallBud)
        .structureName("gametests:light_box")
        .maxTicks(260);
    GameTest.register("BlockBehaviorTests", "amethyst_bud_grows_to_next_stage", amethystBudGrowsToNextStage)
        .structureName("gametests:light_box")
        .maxTicks(420);
}
