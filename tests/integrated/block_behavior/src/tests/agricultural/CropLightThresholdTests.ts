// 作物光照门槛集成测试：验证 CropBlock 的 CROP_GROWTH_LIGHT_THRESHOLD=9 光照门槛（对齐 wiki 亮度文档）。
//
// wiki 亮度「内部光照的影响#方块」表（tech_亮度.txt 行595-660）Wheat Crops 行：
//   光照 0-7 级：接收方块更新时掉落为物品；
//   光照 8 级：停止生长；
//   光照 9-15 级：生长。
// wiki 作物机制（block_作物机制.txt#作物生长 行26）：
//   "作物的生长由随机刻驱动。每次随机刻时，游戏会先判定作物方块的亮度等级；若亮度至少为9，
//    作物有 1/floor(1+25/g) 的概率提升一级生长等级"。
//
// Cubium 实现（CropBlock.cpp:146-164 randomTick）：
//   - isMaxAge 提前返回（成熟不生长）。
//   - if (world.getLightSubtracted(pos, 0) < CROP_GROWTH_LIGHT_THRESHOLD) return;  // <9 不生长
//   - 否则按 1/(25/growthChance+1) 概率 age+1。
// CROP_GROWTH_LIGHT_THRESHOLD=9（Constants.hpp:64）。
//
// ============================ 确定性方案：调高 randomTickSpeed ============================
// 作物生长由随机刻驱动，randomTick 对每个已加载 chunk section 选 randomTickSpeed 个随机位置
// （ServerWorld.cpp:1319-1386 tickEnvironment，默认 speed=3）。section 4096 格，默认 speed=3 时
// 单格每 tick 被选中概率仅 3/4096≈0.073%，短时间命中概率极低——age 不变多半因没被 tick 到，
// 而非门槛拦截，无法有效验证门槛。
//
// 解决：测试开头用 SimulatedPlayer.chat("/gamerule randomTickSpeed 1000") 调高随机刻速度
// （与 WeatherSkyDarkeningTests.ts 用 chat 执行 /weather 同范式）。GameRuleCommand.cpp 需 OP≥2，
// SimulatedPlayer 创造模式权限等级2满足（SimulatedPlayer.cpp:187 isCreative()?2:0）。GameRules.cpp:118
// randomTickSpeed 默认3，registerInteger 经 setFromString 改值。
// speed=1000 时单格每 tick 命中概率 1-(1-1000/4096)^1≈24.4%，120 tick 内至少命中一次概率
// >1-2.6e-15≈100%，门槛验证转为确定性。
//
// MinecraftStructurePlacer 为每个测试结构区域加 forced chunk ticket（MinecraftStructurePlacer.cpp
// :112 forceChunk），测试期间 chunk 常驻加载，tickEnvironment 的 forEachLoadedChunk 必遍历到
// 测试结构所在 chunk，randomTick 覆盖作物格。
//
// chat 返回值不 assert：Cubium chat 返回 int（命令返回值），基岩 BDS chat 返回 void（发消息语义），
// 两端 chat 语义不同（基岩 chat 不执行命令），故本组测试基岩侧 one-sided（同 WeatherSkyDarkeningTests）。
// 命令是否生效通过测试结果间接验证：测试2（充足光照生长）age>0 证明 randomTickSpeed 调高生效
// （默认 speed=3 时 160 tick 内单株生长概率不足）；测试1（黑暗不生长）配对测试2，同一调高机制。
//
// ============================ 测试设计 ============================
// light_box（7×7×7 封顶实心盒，内部 x,z∈[1,5] y∈[1,5]，skyLight=0 隔绝天空光）。
// (3,0,3) farmland 支撑（light_box 地板 stone，setBlockPermutation flags=3 强制放 farmland 存活；
//   FarmlandBlock::canSustainPlant Crop 分支只认 FARMLAND（Block.cpp:685-687），故必须用 farmland 而非
//   dirt 支撑作物；BushBlock canSustain 经 canSustainPlant 委托，Crop 类型只认 FARMLAND）。
// (3,1,3) wheat age=0（BlockPermutation.resolve）。
//
// farmland 退化风险已排除：FarmlandBlock::randomTick moisture=0+无水无雨时，若上方无
// MAINTAINS_FARMLAND 标签方块才 turnToDirt（FarmlandBlock.cpp:165 hasCrops）。wheat 在
// maintains_farmland 标签内（BlockTags.cpp:2674-2679），上方有作物时 hasCrops=true，farmland 不退化，
// 作物支撑稳定。farmland 在 stone 上非真实支撑，但 flags=3 强制放存活，本测试只验证作物光照门槛。
//
// 测试1 crop_does_not_grow_in_dark（门槛上界，光照0<9 不生长）：
//   light_box 封顶无光源，作物处 blockLight=0 skyLight=0 光照=0<9。调高 randomTickSpeed 后等待
//   ~120 tick（足够多 randomTick 命中），断言 age===0。门槛正确→每次命中都 return age 恒0；
//   门槛 bug→命中时按概率 age+1，多次命中后 age 几乎必然>0。
//   守卫：断言作物处 blockLight===0 && skyLight===0 确认黑暗环境成立（仅 Cubium 侧判定，基岩侧
//   blockLight/skyLight 不可读跳过）。注意 Cubium 脚本 block.blockLight 调 world.getBlockLight(pos)
//   返回该位置环境方块光（非方块自身 lightLevel）；实测 wheat 处读环境光正确，但 air 处恒0且光源距2格
//   经 air 间隙传播归0（见排除项「光照8级」说明）。本守卫读 wheat 处（无光源）应0，黑暗成立可判。
//
// 测试2 crop_grows_in_light（门槛下界，光照≥9 生长）：
//   (3,1,3) 作物旁 (4,1,3) 放 glowstone(15) 提供方块光，作物处光照=14≥9。调高 randomTickSpeed 后
//   等待足够 tick，断言 age>0（已生长）。单株干燥耕地生长速度 g=2（CropBlock.cpp:248-297
//   getGrowthChance 初始1+下方干燥耕地1），生长概率 1/(25/2+1)=1/13，调高 randomTickSpeed 后多次命中，
//   age 增长概率→1。验证"≥9 生长"门槛。同时作为测试1 randomTickSpeed 生效的旁证。
//   注：作物成熟 maxAge=7 后 isMaxAge 提前 return 不再生长，断言 age>0（非 age===7）避免过度等待。
//
// ============================ 排除项（不写测试）============================
// - 低光照「接收方块更新掉落」（wiki 0-7级掉落）：经核查 Cubium 存在两处与 vanilla 偏差，致该行为不可测：
//   ① CropBlock 未重写 updatePostPlacement 接入光照检查（BushBlock.cpp:67-93 仅在 facing==Down 查
//      canSustain 耕地，不查光照，且限定 Down 方向）；而 vanilla VegetationBlock.updateShape 任意方向
//      邻居变化都调 canSurvive（CropBlock.canSurvive 含 getRawBrightness>=8 光照检查，CropBlock.java:144-150）。
//   ② 即便补上光照检查，Cubium 自毁链路也不掉落：updatePostPlacement 返回 AIR 后，ServerWorld::setBlockState
//      （ServerWorld.cpp:914-917）只是静默写入 AIR，不调 Block::dropResources/spawnAfterBreak/getDrops/lootTable
//      （ServerWorld.cpp 全文零掉落调用，onBlockRemoved 默认空操作 Block.cpp:516-523）。这是项目级缺陷——
//      所有 BushBlock 子类（花/作物/树苗等）自毁均静默消失不掉落，非作物独有。UPDATE_SUPPRESS_DROPS(32)
//      在此链路亦未读取（死定义）。掉落需走 scheduleBlockTick + tick() 内显式 dropResources 再 setBlockState(AIR)
//      的模式（参照 BigDripleafStemBlock.cpp:145-151）。
//   按「不为 Cubium 与 vanilla 不一致行为写测试」准则跳过。
//   TODO: 待 Cubium 修复①CropBlock updatePostPlacement 接入 isValidPosition 光照检查（对齐 vanilla canSurvive
//   CROP_SURVIVAL_LIGHT_THRESHOLD=8，任意方向）+②自毁链路接入 dropResources 掉落（项目级，参照
//   BigDripleafStemBlock tick 模式）后，补充低光照（<8）接收方块更新时作物掉落为物品的测试。
// - 光照 8 级「停止生长」边界（区分存活门槛8 与生长门槛9）：本次实测发现 Cubium 方块光传播存在缺陷——
//   光源距1格能传播（glowstone15→14、crying_obsidian10→9），但距2格经 air 间隙传播归0（air 格光照数据
//   未正确重算/存储）。作物处 blockLight 只能取「相邻光源-1」，无光等级=9 的方块可构造 blockLight=8
//   （光等级10距1格→9 生长；光等级8距1格→7 不存活）。故无法在 Cubium 构造稳定的 blockLight=8 环境，
//   边界测试不可行。此属 Cubium StarLight 与 vanilla BFS 传播的系统性偏差之一（同 skylight 散射偏差，
//   不为偏差写测试）。门槛9 已由测试1(光照0<9不生长)/测试2(光照14≥9生长)覆盖上界与下界。跳过8级边界。
//   TODO: 待 Cubium 修复方块光穿 air 间隙传播（对齐 vanilla BFS 衰减 -1/格）后，可用 crying_obsidian(10)
//   距作物2格构造 blockLight=8 补「存活但不生长」边界测试，验证 CROP_SURVIVAL_LIGHT_THRESHOLD(8) <
//   CROP_GROWTH_LIGHT_THRESHOLD(9) 的精确区分。
//
// ============================ 跨服务端对比 ============================
// - /gamerule randomTickSpeed、SimulatedPlayer.chat、BlockPermutation.resolve、getState("age") 在
//   Cubium 侧可用。基岩 BDS SimulatedPlayer.chat 是发消息语义（void，不执行命令），故本组用 chat
//   执行 /gamerule 的测试基岩侧无法跑（one-sided，同 WeatherSkyDarkeningTests.ts）。
// - blockLight/skyLight 是 Cubium 专有（基岩 Block 无此属性），黑暗环境断言仅 Cubium 侧判定。
// - 作物 age state 名两端一致，光照门槛行为两端一致（randomTick 检查亮度≥9 才生长）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#内部光照的影响#方块（Wheat Crops 0掉落/8停止/≥9生长）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_作物机制.txt#作物生长（随机刻亮度≥9 才有概率生长）
// Ref: CropBlock.cpp:146-164（randomTick 光照门槛 CROP_GROWTH_LIGHT_THRESHOLD=9）、:248-297（getGrowthChance）
// Ref: FarmlandBlock.cpp:151-173（randomTick 退化条件 hasCrops 守卫）、:277-292（canSustainPlant Crop 只认 FARMLAND）
// Ref: BlockTags.cpp:2673-2680（maintains_farmland 标签含 wheat/carrots/potatoes/beetroots/瓜果茎）
// Ref: GameRuleCommand.cpp（/gamerule <rule> <value> 命令 OP≥2）、GameRules.cpp:118（randomTickSpeed 默认3）
// Ref: WeatherSkyDarkeningTests.ts（SimulatedPlayer.chat 执行命令范式，基岩 one-sided）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { BlockPermutation } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const FARMLAND = { x: 3, y: 0, z: 3 };
const CROP = { x: 3, y: 1, z: 3 };
const GLOWSTONE = { x: 4, y: 1, z: 3 };

// 调高 randomTickSpeed 使作物格在数 tick 内被随机刻确定性命中。
// 1000 使单格每 tick 命中概率≈24.4%，120 tick 内至少命中一次概率≈100%。light_box 石墙隔离 +
// 内部仅 farmland/wheat/glowstone/stone/air，farmland 有作物时不退化，randomTick 副作用可控。
const HIGH_RANDOM_TICK_SPEED = "1000";

// 放置 farmland 支撑 + age=0 指定作物，返回放置是否成功（getBlock 非空且为该作物）。
// setBlockPermutation flags=3 强制放置存活（不调 isValidPosition，绕过光照<8 检查）。
function placeCrop(test: Test, cropType: string): boolean {
    test.setBlockType("minecraft:farmland", FARMLAND);
    const perm = BlockPermutation.resolve(cropType, { age: 0 }) as any;
    (test as unknown as {
        setBlockPermutation: (blockData: unknown, blockLocation: { x: number; y: number; z: number }) => void;
    }).setBlockPermutation(perm, CROP);
    const block = test.getBlock(CROP);
    return block !== undefined && block.typeId === cropType;
}

// 读取作物 age（number），若读取失败返回 undefined。
function getCropAge(test: Test): number | undefined {
    const block = test.getBlock(CROP);
    if (block === undefined) {
        return undefined;
    }
    const age = block.permutation?.getState("age");
    return typeof age === "number" ? age : undefined;
}

// 黑暗中作物不生长（光照门槛上界，光照0<9 不生长）：light_box 封顶无光源，作物处光照=0<9，
// 调高 randomTickSpeed 后等待足够 tick，断言 age===0（门槛拦截所有 randomTick 生长尝试）。
// 守卫：作物处 blockLight===0 && skyLight===0 确认黑暗环境成立（仅 Cubium 侧判定）。
function cropDoesNotGrowInDark(test: Test): void {
    test.assert(placeCrop(test, "minecraft:wheat"), "wheat should be placed at (3,1,3)");

    // 调高 randomTickSpeed 使作物格被随机刻确定性命中（SimulatedPlayer 创造模式权限2 执行 /gamerule）。
    // 不 assert chat 返回值：Cubium chat 返回 int，基岩 chat 返回 void 语义不同，基岩侧 one-sided。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "farmer");
    player.chat(`/gamerule randomTickSpeed ${HIGH_RANDOM_TICK_SPEED}`);

    pollUntilSucceed(
        test,
        () => {
            // 黑暗环境守卫：作物处光照必须为0（仅 Cubium 侧 blockLight/skyLight 可读，基岩侧跳过此守卫）。
            const block = test.getBlock(CROP);
            const blockLight = (block as unknown as { blockLight?: number })?.blockLight;
            const skyLight = (block as unknown as { skyLight?: number })?.skyLight;
            if (typeof blockLight === "number" && typeof skyLight === "number") {
                if (blockLight !== 0 || skyLight !== 0) {
                    return false; // 环境非黑暗，等待光照重算稳定（不应发生，保守轮询）
                }
            }
            // 等待足够 tick 后 age 仍===0（光照门槛拦截所有 randomTick 生长尝试）。
            const age = getCropAge(test);
            return age === 0;
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 120,
            onTimeout: () => {
                const age = getCropAge(test);
                const block = test.getBlock(CROP);
                const blockLight = (block as unknown as { blockLight?: number })?.blockLight ?? -1;
                const skyLight = (block as unknown as { skyLight?: number })?.skyLight ?? -1;
                test.assert(
                    false,
                    `wheat in dark: age=${age} expected 0 (light threshold<9 should block growth) ` +
                        `blockLight=${blockLight} skyLight=${skyLight} (both should be 0 in light_box; ` +
                        `if age>0, CROP_GROWTH_LIGHT_THRESHOLD check may be missing in randomTick)`,
                );
            },
        },
    );
}

// 充足光照下作物生长（光照门槛下界，光照≥9 生长）：作物旁放 glowstone(15) 提供方块光，作物处
// 光照=14≥9，调高 randomTickSpeed 后等待足够 tick，断言 age>0（已生长）。
// 验证 CROP_GROWTH_LIGHT_THRESHOLD=9 门槛下界：光照≥9 时 randomTick 按概率增长 age。
// 同时作为测试1 randomTickSpeed 调高生效的旁证（默认 speed=3 时短时间单株难生长）。
function cropGrowsInLight(test: Test): void {
    test.assert(placeCrop(test, "minecraft:wheat"), "wheat should be placed at (3,1,3)");
    test.setBlockType("minecraft:glowstone", GLOWSTONE);

    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "farmer");
    player.chat(`/gamerule randomTickSpeed ${HIGH_RANDOM_TICK_SPEED}`);

    pollUntilSucceed(
        test,
        () => {
            const age = getCropAge(test);
            // age>0 表示已发生至少一次生长（光照≥9 门槛通过 + 概率命中）。
            return age !== undefined && age > 0;
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 160,
            onTimeout: () => {
                const age = getCropAge(test);
                const block = test.getBlock(CROP);
                const blockLight = (block as unknown as { blockLight?: number })?.blockLight ?? -1;
                test.assert(
                    false,
                    `wheat in light: age=${age} expected >0 (light>=9 should allow growth) ` +
                        `blockLight=${blockLight} (should be >=9 near glowstone; ` +
                        `if age=0, randomTickSpeed may not be raised or growth chance too low)`,
                );
            },
        },
    );
}

export function registerCropLightThresholdTests(): void {
    GameTest.register("BlockBehaviorTests", "crop_does_not_grow_in_dark", cropDoesNotGrowInDark)
        .structureName("gametests:light_box")
        .maxTicks(250);
    GameTest.register("BlockBehaviorTests", "crop_grows_in_light", cropGrowsInLight)
        .structureName("gametests:light_box")
        .maxTicks(250);
}
