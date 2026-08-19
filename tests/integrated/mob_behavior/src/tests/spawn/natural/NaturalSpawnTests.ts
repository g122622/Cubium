// 自然生成（NaturalSpawner）行为 GameTest。
//
// 覆盖 wiki tech_怪物.txt#自然生成 / tech_动物.txt#自然生成 核心机制：玩家存在门控、距离门控（24-128 格）、
// 光照门槛（怪物 getMaxLocalRawBrightness<=7，动物 >7）、群系 SpawnEntry。
//
// 测试手段：test.spawnNaturalAt(category, pos[, biome])——项目独有单点生成入口，对齐 Java @VisibleForDebug
// NaturalSpawner.spawnCategoryForPosition(MobCategory, ServerLevel, BlockPos)（vanilla /debugmobspawning
// 命令背后的调试入口）。给定精确坐标做一次完整条件检查 + 生成，绕过 tick 的随机区块/区块内选址，
// 消除小结构 footprint 命中率极低的随机性（tick 路径在 49 force 区块随机选位，结构仅 3 区块命中率 ~6%，
// 端到端不可稳定测试）。单点入口仍做 ±5 抖动 + 随机选 entry + 距离/光照/放置/碰撞检查 + finalizeSpawn，
// 仅省略 cap/SpawnCosts（对齐 vanilla 3 参版恒真 predicate），故能精确验证条件判定逻辑。
//
// biome 注入（第三参）：GameTest 结构固定放世界原点 (0,-59,0)，原点 biome 由世界种子决定不可控
// （GameTestServer 默认 seed=0，原点 biome=ColdOcean(46) 海洋，无陆地动物 SpawnEntry）。故 animal
// 测试传 "plains" 强制用 plains biome 取 SpawnEntry，绕过世界真实 biome——测试语义为"给定 plains biome +
// 该坐标，NaturalSpawner 能否生成动物"，精确隔离"biome→SpawnEntry 选择"与"世界 biome 生成"两个正交
// 关注点。monster 测试不传 biome（ColdOcean 怪物池非空，海洋也刷怪）。光照/距离/放置/finalizeSpawn 仍走
// 真实世界路径，biome 注入仅覆盖 SpawnEntry 选择一步，不破坏条件判定语义。
//
// C++ 链路：
//   - ScriptTestHelper.cpp spawnNaturalAt 绑定 → helper->world().asServerWorld() 取 ServerWorld
//     → NaturalSpawner::spawnCategoryForPosition(world, classification, pos, biomeOverride)（static 单点入口）。
//   - 单点入口复用 tick 路径的 _getRandomSpawnEntry（biomeOverride 非 0 时跳过 chunk biome 查询）/
//     _canSpawnAt（含 _checkLightLevel 用 getMaxLocalRawBrightness 含 skyDarkening 时间衰减）/
//     _trySpawnAt（含 finalizeSpawn）。
//
// 关键事实（核查 Java 1.21.11 NaturalSpawner.java:145-232 + Cubium 实现确认）：
//   1. 单点入口需世界中存在玩家（getClosestPlayer 非空）+ 距离门控（抖动位距最近玩家 24-128 格）。
//      故玩家位置精确控制：站结构一端，生成位在另一端，距离 30+ 格满足门控。
//   2. 光照门槛（_checkLightLevel 对齐 vanilla Monster.isDarkEnoughToSpawn）：用 getMaxLocalRawBrightness
//      （含 skyDarkening 时间衰减）。night batch 夜晚 skyDarkening 使露天 skyLight 衰减到 ~4<=7 怪物通过；
//      day batch 白天 skyDarkening=0 露天 skyLight=15>7 怪物拒绝、动物 >7 通过。
//   3. plains 群系 SpawnEntry：怪物 zombie/skeleton/creeper/spider/slime/enderman/witch/zombie_villager；
//      动物 sheep/pig/cow/chicken/horse/donkey。ColdOcean（原点真实 biome）怪物池非空、动物池空。
//   4. 单点入口 ±5 抖动可能落结构外（worldgen 石头），_canSpawnAt 拒；pollUntilSucceed 多次调用，
//      命中结构内 air 腔的概率远高于 tick 随机选址（每次 3 轮抖动，生成位居结构中心区降低外泄）。
//   5. night batch 避免亡灵白天燃烧（夜晚亡灵不燃，生成后存活）；day batch 动物白天露天光照充足。
//
// 结构选择（均露天无封顶 + skyAccess(true) 使生成位 air 腔 skyLight 受 dayTime 衰减控制）：
//   - dark_cavern（41×7×9 露天石地围栏）：怪物自然生成测试。skyAccess 清空上方 worldgen 使生成位
//     skyLight 受 dayTime 衰减（night 衰减到 ~4<=7 怪物通过）。玩家 x=2 端，生成位 x=35 居结构中心区
//     （距玩家 33 格，24-128 门控内；±5 抖动后 x∈[30,40] 全在结构内 air 腔）。
//   - open_grass_hall（41×7×9 露天草地长廊）：动物自然生成测试。skyAccess 露天 skyLight=15 满足动物 >7，
//     grass_block 地板支撑，day batch 白天光照充足。玩家 x=2 端，生成位 x=35（距 33 格门控内）。
//     biome 注入 "plains"（原点真实 biome 是 ColdOcean 海洋，无陆地动物 SpawnEntry）。
//
// 判定手段：getEntities 区域计数遍历 plains 怪物/动物 type 列表，断言任一出现。
//   必须区域限定（批内并行 tick + 不清场，全维度 getEntities 跨测试污染）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_怪物.txt#自然生成
// Ref: NaturalSpawner.cpp（spawnCategoryForPosition 单点入口/距离门控/_canSpawnAt 光照门槛/_trySpawnAt finalizeSpawn）
// Ref: NaturalSpawner.java:145-232（vanilla spawnCategoryForPosition @VisibleForDebug 单点入口）
// Ref: Monster.java:84-98（vanilla isDarkEnoughToSpawn 用 getMaxLocalRawBrightness 含 skyDarkening）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// dark_cavern / open_grass_hall 结构尺寸均为 41×7×9（helper 相对坐标 x∈[0,40], y∈[0,6], z∈[0,8]）。
const HALL_FROM = { x: 0, y: 0, z: 0 };
const HALL_VOLUME = { x: 41, y: 7, z: 9 };

// 玩家位置（x=2 端，y=2 站结构内 air 腔地板上方一格，z=4 居中）。
// 生成位 SPAWN_POS（x=35 居结构中心区，距玩家 33 格满足 24-128 门控；±5 抖动后 x∈[30,40] 全在结构内）。
// helper y=2 = 文件 y=1 = 世界 y=-57（结构内 air 腔，stone 地板文件 y=0 上方第一格 air，合法生成位）。
//
// 注：spawnNaturalAt 接收世界坐标（NaturalSpawner::spawnCategoryForPosition 用世界坐标查 chunk/biome/光照），
// 故调用前用 test.worldLocation(SPAWN_POS) 把 helper 相对坐标转世界坐标。
const PLAYER_POS = { x: 2, y: 2, z: 4 };
const SPAWN_POS = { x: 35, y: 2, z: 4 };

// plains 群系怪物 SpawnEntry。
const MONSTER_TYPES = [
    "zombie", "zombie_villager", "skeleton", "creeper",
    "spider", "slime", "enderman", "witch",
];

// plains 群系动物 SpawnEntry。
const ANIMAL_TYPES = ["sheep", "pig", "cow", "chicken", "horse", "donkey"];

// 区域内统计任一指定类型实体数（区域限定排除并行测试污染）。
function countAny(test: Test, types: string[]): number {
    let total = 0;
    for (const type of types) {
        total += test.getDimension().getEntities({
            type,
            location: test.worldLocation(HALL_FROM),
            volume: HALL_VOLUME,
        }).length;
    }
    return total;
}

// 夜晚露天场地自然生成怪物（wiki tech_怪物.txt#自然生成：黑暗环境 + 玩家附近生成怪物）。
//
// dark_cavern 露天无封顶 + skyAccess：skyAccess 清空结构上方 worldgen 石头，使生成位 skyLight 受 dayTime
// 衰减。night batch 夜晚 skyDarkening 使露天 skyLight 衰减到 ~4，getMaxLocalRawBrightness<=7 怪物光照
// 门槛通过。玩家 x=2 端，生成位 x=35（距 33 格门控内）。
//
// 单点入口 spawnNaturalAt("monster", SPAWN_POS) 每检查点触发一次判定（3 轮 ±5 抖动 + 随机选 entry +
// 光照/距离/放置检查 + finalizeSpawn）。pollUntilSucceed 多次调用，命中生成后 countAny 检测到即 succeed。
//
// 注：slime 在 plains 仅特定高度/区块生成（slime chunk），可能不出，但 zombie/skeleton/creeper 权重高
// 应出现。断言任一怪物（8 类并集）放宽单类随机性。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_怪物.txt#自然生成（黑暗环境生成怪物）
function naturalSpawnMonsterInDarkCavern(test: Test): void {
    test.spawnSimulatedPlayer(PLAYER_POS, "observer");

    pollUntilSucceed(test, () => {
        // 触发一次单点自然生成判定（怪物分类）。worldLocation 把 helper 相对坐标转世界坐标。
        test.spawnNaturalAt("monster", test.worldLocation(SPAWN_POS));
        return countAny(test, MONSTER_TYPES) >= 1;
    }, {
        maxTick: 600,
        onTimeout: () => test.assert(false,
            `no monster naturally spawned in dark cavern (monsters=${countAny(test, MONSTER_TYPES)})`),
    });
}

// 无玩家时 NaturalSpawner 不生成（players.empty() 早退 / getClosestPlayer 返 null）。
//
// 对照测试：dark_cavern 内不 spawn 玩家，每检查点调 spawnNaturalAt（单点入口内部 getClosestPlayer 返 null
// 直接返回 0 不生成）。等若干检查点后断言无怪物生成。
//
// 负向断言：若玩家门控失效，夜晚露天会生成怪物，monster>0 暴露 bug。
// Ref: NaturalSpawner.cpp spawnCategoryForPosition players.empty() 早退
function naturalSpawnRequiresPlayer(test: Test): void {
    // 不 spawn 玩家。多次触发单点生成判定，断言始终无怪物。
    pollUntilSucceed(test, () => {
        test.spawnNaturalAt("monster", test.worldLocation(SPAWN_POS));
        // 负向测试：条件恒为 false（不 succeed），靠 onTimeout 断言无怪物。
        return false;
    }, {
        maxTick: 200,
        onTimeout: () => {
            const monsters = countAny(test, MONSTER_TYPES);
            test.assert(monsters === 0,
                `monster spawned without player (monsters=${monsters})`);
            test.succeed();
        },
    });
}

// 亮处（glowstone 提 blockLight=15）不自然生成怪物（光照门槛 getMaxLocalRawBrightness<=7 拒绝）。
//
// 反向测试：dark_cavern 生成腔（x=30..40 区域）地板满铺 glowstone 提 blockLight=15，怪物光照门槛
// getMaxLocalRawBrightness=max(blockLight=15, skyLight 衰减后)=15 >7 拒绝生成。
// 玩家 x=2 端激活，单点入口在亮处生成位判定，光照门槛持续拒绝。
//
// glowstone 覆盖范围 x∈[30,40] z∈[0,8]（dark_cavern 全 z 范围 0..8）：单点入口 ±5 抖动后 z∈[-1,9]，
// 其中 z=0..8 全在 glowstone 覆盖内（blockLight=15 拒怪物），仅 z=-1/9 越界落结构外 worldgen（_canSpawnAt
// 放置检查拒）。若 glowstone 仅铺 z∈[1,7]，抖动命中 z=0/8 时该位无 glowstone，blockLight=0 夜晚露天
// skyLight 衰减后 brightness≤7 怪物通过——致测试 flaky（单测偶过、批次多轮命中失败）。
//
// 注：glowstone 放生成腔地板层 helper y=1（=文件 y=0 stone 地板），用 glowstone 覆盖 stone 地板提
// blockLight=15 向上传播覆盖生成区（生成位 helper y=2 = glowstone 上方第一格 air）。glowstone 占文件
// y=0 后该列生成位文件 y=1 仍 air，但 blockLight=15 拒绝。night batch 夜晚露天 skyLight 衰减 ~4，
// 但 max(15,4)=15>7 仍拒绝。
//
// 关 doMobSpawning 隔离 tick 路径：countAny 查询整个结构（41×7×9），但 glowstone 仅铺 x∈[30,40]，
// 结构内 x∈[0,29] 无 glowstone 保护的 air 腔可被 NaturalSpawner::tick 路径命中合规生成怪物（玩家 x=2，
// x∈[27,29] 距玩家 25-27 格在门控内），被 countAny 捕获致 monsters>0 假失败。本测试验证的是【单点入口
// spawnNaturalAt 的光照门槛】，故关 doMobSpawning（NaturalSpawner 遵守该规则，对齐 vanilla
// ServerChunkCache.tickChunks:376）隔离 tick 路径，仅留单点入口做光照判定。runOnFinish 恢复 true
// 防污染后续测试（PASSED/FAILED/TIMEOUT 三态均触发）。批次用 night_lit 独占，避免与 distance_gate
// 同设 doMobSpawning=false 同批竞态。
// Ref: NaturalSpawner _checkLightLevel（怪物 getMaxLocalRawBrightness<=7）
// Ref: vanilla ServerChunkCache.tickChunks:376（doMobSpawning 门控自然生成）
function naturalSpawnNoMonsterWhenLit(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "observer");

    // 生成腔地板层 helper y=1（=文件 y=0）满铺 glowstone（x∈[30,40] z∈[0,8] 全 z 范围），提 blockLight=15
    // 覆盖 ±5 抖动全部 z 命中位。
    for (let x = 30; x <= 40; x++) {
        for (let z = 0; z <= 8; z++) {
            test.setBlockType("minecraft:glowstone", { x, y: 1, z });
        }
    }

    // 关停 tick 路径，隔离单点入口的光照门槛判定（见上方注释）。
    player.chat("/gamerule doMobSpawning false");
    test.runOnFinish(() => {
        player.chat("/gamerule doMobSpawning true");
    });

    pollUntilSucceed(test, () => {
        // 生成位 SPAWN_POS（x=35,y=2,z=4）已被 glowstone 照亮（blockLight=15），怪物光照门槛拒绝。
        test.spawnNaturalAt("monster", test.worldLocation(SPAWN_POS));
        // 负向测试：条件恒 false，靠 onTimeout 断言无怪物。
        return false;
    }, {
        maxTick: 200,
        onTimeout: () => {
            const monsters = countAny(test, MONSTER_TYPES);
            test.assert(monsters === 0,
                `monster spawned when lit (monsters=${monsters})`);
            test.succeed();
        },
    });
}

// 露天草地白天自然生成动物（wiki tech_动物.txt#自然生成：动物在光照充足的草地上生成）。
//
// open_grass_hall 露天 skyAccess + day batch 白天 skyLight=15 满足动物 getMaxLocalRawBrightness>7 门槛，
// grass_block 地板支撑。玩家 x=2 端，生成位 x=35（距 33 格门控内）。
//
// biome 注入 "plains"：GameTestServer 默认 seed=0，结构原点真实 biome=ColdOcean(46) 海洋，无陆地动物
// SpawnEntry（海洋只生水生生物）。注入 plains 强制用 plains 的 Creature 池（sheep/pig/cow/chicken/
// horse/donkey），绕过世界 biome 不可控约束。测试语义为"给定 plains biome + 露天草地白天坐标，
// NaturalSpawner 能否生成动物"——精确验证 plains SpawnEntry + 光照 + 距离 + 放置判定链路。
//
// 单点入口 spawnNaturalAt("creature", SPAWN_POS, "plains") 每检查点触发一次动物生成判定。pollUntilSucceed
// 多次调用，命中生成后 countAny 检测到动物即 succeed。
//
// 注：动物 SpawnEntry 在 plains 权重低于怪物，单点入口 ±5 抖动 + 随机选 entry 命中动物需多轮，
// maxTick 留足周期。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_动物.txt#自然生成
function naturalSpawnAnimalOnGrassDaytime(test: Test): void {
    test.spawnSimulatedPlayer(PLAYER_POS, "observer");

    pollUntilSucceed(test, () => {
        // 第三参 "plains" 强制用 plains biome 取动物 SpawnEntry（绕过原点 ColdOcean 真实 biome）。
        test.spawnNaturalAt("creature", test.worldLocation(SPAWN_POS), "plains");
        return countAny(test, ANIMAL_TYPES) >= 1;
    }, {
        maxTick: 800,
        onTimeout: () => test.assert(false,
            `no animal naturally spawned on grass (animals=${countAny(test, ANIMAL_TYPES)})`),
    });
}

// 玩家距生成点 <24 格时不自然生成（距离下限门控 MIN_SPAWN_DISTANCE_SQ=576）。
//
// 对照测试：dark_cavern 玩家站生成位旁（x=30，生成位 x=35，距 5 格 <24），单点入口 ±5 抖动后生成位
// 仍在玩家 24 格内，距离下限门控拒绝全部候选位。等若干检查点断言无怪物生成。
//
// 关键：必须关闭 doMobSpawning 游戏规则隔离 NaturalSpawner::tick 路径。原因：tick 路径每 tick 在
// 结构内距玩家 25-128 格处合规生成怪物（dark_cavern 41 格长，玩家 x=30，结构内 x∈[0,5] 距玩家 25-30
// 格恰在门控内，tick 路径在此区域合规生成怪物）。countAny 查询整个结构（41×7×9）会捕获到 tick 路径
// 合规生成的怪物，致 monsters>0 假失败。本测试要验证的是【单点入口 spawnNaturalAt 的距离门控】，
// 故用 chat("/gamerule doMobSpawning false") 关停 tick 路径（NaturalSpawner 遵守 doMobSpawning，
// 对齐 vanilla ServerChunkCache.tickChunks:376），仅留单点入口路径做距离门控判定。
//
// doMobSpawning 是世界级规则，GameTest 共享单一 ServerWorld 不自动重置。本测试设 false 须用
// runOnFinish 恢复 true（PASSED/FAILED/TIMEOUT 三态均触发），否则污染后续依赖自然生成的测试。
// 同批 night 批的 natural_spawn_monster/no_monster_when_lit 用单点入口 spawnNaturalAt 不依赖 tick
// 路径，关 doMobSpawning 不影响它们；其余 night 批测试用 test.spawn 不依赖 NaturalSpawner，亦不受影响。
//
// 负向断言：若距离下限门控失效，单点入口会在玩家 24 格内生成怪物，monster>0 暴露 bug。
// Ref: NaturalSpawner MIN_SPAWN_DISTANCE_SQ=24²
// Ref: vanilla ServerChunkCache.tickChunks:376（doMobSpawning 门控自然生成）
function naturalSpawnDistanceGateUnder24(test: Test): void {
    // 玩家站 x=30，生成位 x=35（距 5 格 <24，±5 抖动后 x∈[30,40] 距玩家 0-10 格全 <24）。
    const player = test.spawnSimulatedPlayer({ x: 30, y: 2, z: 4 }, "observer");

    // 关停 tick 路径，隔离单点入口的距离门控判定（见上方注释）。
    player.chat("/gamerule doMobSpawning false");
    test.runOnFinish(() => {
        player.chat("/gamerule doMobSpawning true");
    });

    pollUntilSucceed(test, () => {
        test.spawnNaturalAt("monster", test.worldLocation(SPAWN_POS));
        // 负向测试：条件恒 false，靠 onTimeout 断言无怪物。
        return false;
    }, {
        maxTick: 200,
        onTimeout: () => {
            const monsters = countAny(test, MONSTER_TYPES);
            test.assert(monsters === 0,
                `monster spawned within 24 blocks of player (monsters=${monsters})`);
            test.succeed();
        },
    });
}

export function registerNaturalSpawnTests(): void {
    GameTest.register("MobBehaviorTests", "natural_spawn_monster_in_dark_cavern", naturalSpawnMonsterInDarkCavern)
        .structureName("gametests:dark_cavern")
        // skyAccess(true)：结构埋 gridStartY=-59 地下，须清空上方 worldgen 使生成位 skyLight 受 dayTime
        // 衰减控制（night 衰减到 ~4<=7 怪物通过）。详见 naturalSpawnMonsterInDarkCavern 注释。
        // night batch：夜晚 18000 环境（skyDarkening 使露天 skyLight 衰减）。
        //   不再用 night_spawn/loadSpawnChunks——单点入口绕过 tick 随机选址与 cap，无需 force 区块，
        //   无结构外 worldgen 残留污染，故可用标准 night batch（与现有亡灵夜晚测试同批，无污染）。
        //   本测试自带玩家（PLAYER_POS），单点入口 getClosestPlayer 全局查询返回本测试玩家（距生成位
        //   33 格，最近），同批其他测试玩家在 73+ 格外更远，不干扰距离门控。countAny 区域限定 x∈[0,40]
        //   本结构内，相邻结构 x∈[73+] 不重叠，不污染计数。
        // setupTicks(20)：清空上方方块后光照变更入队，需若干世界 tick 重算 skyLight 稳定。
        .batch("night")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(700);

    GameTest.register("MobBehaviorTests", "natural_spawn_requires_player", naturalSpawnRequiresPlayer)
        .structureName("gametests:dark_cavern")
        // night_solo 批次（独占）：本测试验证"世界中无玩家时 NaturalSpawner 不生成"——依赖单点入口
        // getClosestPlayer 返回 null 早退。但 night 批次有几十个含玩家测试并行，全局 getClosestPlayer
        // 会找到那些玩家破坏"无玩家"语义。故须独占一个夜晚批次（night_solo，前缀 night 自动获夜晚环境），
        // 使本批次无任何其他玩家，"无玩家"判定成立。批次间串行，night_solo 独跑不与其他测试并行。
        .batch("night_solo")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(300);

    GameTest.register("MobBehaviorTests", "natural_spawn_no_monster_when_lit", naturalSpawnNoMonsterWhenLit)
        .structureName("gametests:dark_cavern")
        // night_lit 独占批次（前缀 night 自动获夜晚环境）：本测试设 doMobSpawning=false 隔离 tick 路径，
        // 独占批次避免与 distance_gate / 其他 night 测试并行时 doMobSpawning 规则竞态。runOnFinish 恢复
        // true 防污染后续批次。详见测试函数注释。
        .batch("night_lit")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(300);

    GameTest.register("MobBehaviorTests", "natural_spawn_animal_on_grass_daytime", naturalSpawnAnimalOnGrassDaytime)
        // day batch：白天 6000 环境（skyDarkening=0 露天 skyLight=15 满足动物 >7 门槛）。
        .batch("day")
        .structureName("gametests:open_grass_hall")
        // skyAccess(true)：结构埋 gridStartY=-59 地下，须清空上方 worldgen 使生成位露天 skyLight=15。
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(900);

    GameTest.register("MobBehaviorTests", "natural_spawn_distance_gate_under_24", naturalSpawnDistanceGateUnder24)
        .structureName("gametests:dark_cavern")
        // night_dist_gate 独占批次（前缀 night 自动获夜晚环境）：本测试设 doMobSpawning=false 隔离 tick
        // 路径，独占批次避免与 night 批其他测试并行时 doMobSpawning 规则竞态（A 设 false / B 恢复 true
        // 互相干扰）。批次间串行，独占跑不与他人并行。runOnFinish 恢复 true 防污染后续批次。
        // 本测试玩家站 x=30 距生成位 x=35 仅 5 格（<24），单点入口 getClosestPlayer 返回本测试玩家
        // （5 格，最近），距离下限门控拒绝全部候选。
        .batch("night_dist_gate")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(300);
}
