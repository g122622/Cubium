// 僵尸破门（BreakDoorGoal）行为 GameTest。
//
// 覆盖 wiki tech_僵尸.txt#破门 + tech_木门.txt#破坏：困难难度下具备破门能力的僵尸会破坏木门追击目标；
// 普通难度僵尸只开门不破门（仅 Hard 难度破门）。
//
// C++ 链路：
//   - ZombieEntity::setBreakDoorsAbility(true)（ZombieEntity.cpp:177-203）动态 add BreakDoorGoal
//     （优先级1）+ 同步 navigator->setCanOpenDoors(true)。破门难度谓词用 zombieDoorBreakDifficultyPredicate()
//     （仅 Hard，对齐 Java 1.21.11 Zombie.java:88 DOOR_BREAKING_PREDICATE）。
//   - BreakDoorGoal::shouldExecute（BreakDoorGoal.cpp:65-92）：DoorInteractGoal::shouldExecute 前置
//     （collidedHorizontally + 路径节点/正上方找到木门）+ mobGriefing + 难度谓词 + 门未开。
//   - BreakDoorGoal::tick（:148-198）：每 20 tick 播音效+挥臂，m_breakTime++，达 getDoorBreakTime()=240
//     调 setBlockState(m_doorPos, air, 3) 破门；DoorBlock::updatePostPlacement 联动清除另一半门。
//   - DoorInteractGoal::shouldExecute（DoorInteractGoal.cpp:49-117）：collidedHorizontally +
//     navigator->canOpenDoors() + 路径节点 point->y()+1 或生物正上方 entityPos.y+1 命中木门
//     （僵尸脚 y=2 → abovePos y=3 门上半，isWooden 对上下半均成立）。
//
// 关键事实（核查 Java 1.21.11 源码 + Cubium 实现确认）：
//   1. 难度门控：Java Zombie.java:88 DOOR_BREAKING_PREDICATE 仅 Hard。Cubium 此前误用
//      defaultDoorBreakDifficultyPredicate()(Normal+Hard) 致 Normal 难度僵尸也破门（偏差），
//      已修复改用 zombieDoorBreakDifficultyPredicate()（仅 Hard）。
//   2. 破门能力来源：test.spawn 不走 finalizeSpawn，僵尸默认 m_canBreakDoors=false 无破门 goal。
//      用 test.spawn 生成僵尸（坐标可靠 + AI 完整，registerGoals 在构造期调用，已验证 test.spawn
//      僵尸能追玩家/村民并移动）+ /data merge entity {CanBreakDoors:1b} 确定性赋予破门能力：
//      DataCommand::_mergeEntity→EntityDataAccessor::mergeData→Entity::readFromNBT
//      →readAdditionalSaveData 读 CanBreakDoors（ZombieEntity.cpp:749-752）→setBreakDoorsAbility(true)
//      注册 BreakDoorGoal + navigator->setCanOpenDoors(true)。
//   3. 破门时间 240 tick（12 秒，BreakDoorGoal DEFAULT_DOOR_BREAK_TIME，getDoorBreakTime 取 max(240,自定义)）。
//   4. 门 facing=NORTH 时碰撞箱 m_shapes[3]=cube(0,0,13,16,16,16)=z∈[13/16,1] 薄层垂直 Z 轴，
//      僵尸沿 Z 轴移动撞门触发 collidedHorizontally。
//   5. m_doorPos 落在门上半 y=3（僵尸撞门停脚 z=3 时 abovePos=(3,3,3)），破门 setBlockState 作用于 y=3 上半，
//      DoorBlock::updatePostPlacement 联动清除 y=2 下半（GameTest 真实世界 setBlockState flags=3 触发邻居更新，
//      整门消失）。
//   6. WalkNodeProcessor：关闭木门 + canOpenDoors + canEnterDoors → WalkableDoor（可通行），
//      路径节点指向门格本身（z=3），僵尸沿 +Z 寻路撞门薄层触发 collidedHorizontally。
//   7. 目标锁定用村民诱饵：僵尸 NearestAttackableTargetGoal<Player>（优先级2，checkSight=true，ZombieEntity.cpp:526）
//      需视线，门挡视线致僵尸看不到门后玩家不选目标。改用村民诱饵——僵尸对村民
//      NearestAttackableTargetGoal<AbstractVillagerEntity>（优先级3，checkSight=false，ZombieEntity.cpp:529-531）
//      不查视线，僵尸直接锁定门后村民为目标沿 +Z 寻路撞门。对齐 vanilla 僵尸破门追村民场景。
//
// 结构选择（关键，曾致僵尸原地不动）：
//   用 creeper_pit（7×5×7 开放坑，文件 y=0 grass_block 地板，y=1..4 全 air 无围墙），不用 glass_pit。
//   glass_pit 有玻璃边墙（文件 y=1,2 边墙 glass），实测致 test.spawn 僵尸原地不动（AI 不触发移动），
//   根因未明（疑似边墙碰撞/寻路 NodeProcessor 交互问题）。creeper_pit 无围墙，实测僵尸能正常追击移动。
//   走廊围墙由测试自建（setBlockType 玻璃墙），完全可控，不依赖结构原墙。
//
// 坐标映射（参照 ZombieReinforcementTests 注释）：
//   GameTestServer gridStartY=-59（origin.y）。helper 相对 y=N → 世界 y=origin.y+N=-59+N → 结构文件 y=N-1。
//   creeper_pit 文件 y=0 grass_block 地板。故 helper y=2 → 世界 y=-57 → 文件 y=1（air 腔），下方文件 y=0
//   grass_block 支撑僵尸站稳。
//
// 几何设计（creeper_pit 7×5×7，helper 相对坐标 x,z∈[0,6], y∈[0,4]）：
//   走廊沿 Z 轴 x=3 列（1 格宽）。自建玻璃墙封 x=2 和 x=4（z=0..6, y=2..4）夹逼走廊，
//   两端 z=0/z=6 放玻璃（x=3, y=2..4）封死防绕出。文件 y=0 grass_block 地板已支撑，无需再铺地板。
//   门 facing=NORTH 下半 (3,2,3) setBlockType + 上半 (3,3,3) setBlockWithStates("half=upper")。
//   僵尸 (3,2,1) z 小侧，村民 (3,2,5) z 大侧门后。僵尸沿 +Z 追村民撞门 z=3 薄层。
//   命令源玩家 (0,2,6) 结构角落（创造模式，门 z=3 挡射线致僵尸看不到玩家，不被选为目标；村民 checkSight=false 优先锁定）。
//
// 判定手段：破门完成后门下半 (3,2,3) 变 air（updatePostPlacement 联动清除整门）。
//   pollUntilSucceed 轮询 getBlock(3,2,3).typeId === "minecraft:air"。
//
// 时序：test.spawn 僵尸+村民（tick 0）+ /data merge 赋能（tick 5）+ 僵尸锁定村民寻路接近门（~40-80 tick）
//   + 破门 240 tick + 余量。maxTick 留充裕余量。GameTestServer tick 脱钩墙钟极速推进，实体 AI 每 tick
//   完整执行，650 tick 足够僵尸寻路+240 tick 破门。
//
// 难度世界级污染隔离：/difficulty 是世界级命令跨测试持久化不自动重置。Hard/Normal 测试各用独占 batch
// （night_hard_door / night_normal_door，前缀 night 自动获夜晚避免亡灵白天燃烧干扰破门判定），
// 批次间串行 + runOnFinish 恢复 normal 难度，防污染同批/后续批次。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_僵尸.txt#破门（困难难度破坏木门）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_木门.txt#破坏（僵尸困难难度破门）
// Ref: BreakDoorGoal.cpp（shouldExecute 难度谓词 + tick 240 tick 破门）
// Ref: DoorInteractGoal.cpp:49-117（collidedHorizontally + 路径/正上方门检测）
// Ref: ZombieEntity.cpp:177-203（setBreakDoorsAbility 注册 BreakDoorGoal）
// Ref: ZombieEntity.cpp:529-531（对村民 NearestAttackableTargetGoal checkSight=false，不查视线）
// Ref: ZombieEntity.cpp:749-752（readAdditionalSaveData 读 CanBreakDoors→setBreakDoorsAbility）
// Ref: DataCommand.cpp _mergeEntity + DataAccessor.cpp EntityDataAccessor::mergeData（/data merge entity 链路）
// Ref: Java Zombie.java:88（DOOR_BREAKING_PREDICATE 仅 Hard）
// Ref: ZombieReinforcementTests.ts（坐标映射 gridStartY=-59 + @e[type=zombie,limit=1,sort=nearest] 选择器范式）
// Ref: SpiderTests.ts spiderAttacksPlayerAtNight（creeper_pit 开放坑 + Survival 玩家诱饵范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 区域限定查询（Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities 跨测试污染）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 门位置（facing=NORTH，下半 y=2 / 上半 y=3，走廊沿 Z 轴 x=3 列）。
const DOOR_LOWER = { x: 3, y: 2, z: 3 };
const DOOR_UPPER = { x: 3, y: 3, z: 3 };
// 僵尸（z 小侧）与村民诱饵（z 大侧门后），沿 Z 轴走廊分列门两侧。
const ZOMBIE_POS = { x: 3, y: 2, z: 1 };
const VILLAGER_POS = { x: 3, y: 2, z: 5 };
// 命令源玩家（创造模式，结构角落，远离僵尸视线）。
const PLAYER_POS = { x: 0, y: 2, z: 6 };

// 搭建破门测试走廊：玻璃墙夹逼 1 格宽 Z 轴走廊 + 木门（facing=NORTH）。
//
// creeper_pit 是开放坑（无原墙），走廊围墙由本函数自建。走廊 x=3 列。
// 玻璃墙封 x=2 和 x=4（z=0..6, y=2..4）夹逼走廊；两端 z=0/z=6 放玻璃（x=3, y=2..4）封死防绕出。
// 文件 y=0 grass_block 地板已支撑僵尸+村民，无需再铺地板。
// 门下半 setBlockType（half=Lower 默认，facing 默认 NORTH），上半 setBlockWithStates("half=upper")
// （见 DoorTests 范式）。门 facing=NORTH 碰撞薄层 z∈[3.81,4] 垂直 Z 轴，僵尸沿 +Z 撞命中。
function buildDoorCorridor(test: Test): void {
    // 玻璃墙夹逼走廊：x=2 和 x=4 两列满铺 z=0..6, y=2..4。
    for (let z = 0; z <= 6; z++) {
        for (const y of [2, 3, 4]) {
            test.setBlockType("minecraft:glass", { x: 2, y, z });
            test.setBlockType("minecraft:glass", { x: 4, y, z });
        }
    }
    // 走廊两端封死：z=0 和 z=6 放玻璃（x=3, y=2..4），防僵尸/村民走出走廊。
    for (const y of [2, 3, 4]) {
        test.setBlockType("minecraft:glass", { x: 3, y, z: 0 });
        test.setBlockType("minecraft:glass", { x: 3, y, z: 6 });
    }
    // 木门 facing=NORTH：下半 (3,2,3) setBlockType（half=Lower 默认，facing 默认 NORTH），
    // 上半 (3,3,3) setBlockWithStates("half=upper")。setBlockType 只放默认 state，上半须显式设。
    test.setBlockType("minecraft:oak_door", DOOR_LOWER);
    test.setBlockWithStates("minecraft:oak_door", DOOR_UPPER, "half=upper");
}

// 读取门下半方块 typeId（破门后变 "minecraft:air"）。返回空串表示读取失败。
function getDoorTypeId(test: Test): string {
    const block = test.getBlock(DOOR_LOWER) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 生成僵尸并赋予破门能力。
//
// 流程：
//   1. test.spawn 生成僵尸于 ZOMBIE_POS（坐标可靠 + AI 完整，registerGoals 在构造期调用，
//      spiderAttacksPlayerAtNight 范式已验证 test.spawn 怪物能追击移动）。
//   2. 创造玩家（命令源）tick 5 执行 /difficulty + /data merge entity {CanBreakDoors:1b}
//      确定性赋予破门能力（readAdditionalSaveData→setBreakDoorsAbility 注册 BreakDoorGoal）。
//
// 僵尸目标驱动：村民诱饵（test.spawn 生成于 VILLAGER_POS 门后），僵尸对村民
// NearestAttackableTargetGoal<AbstractVillagerEntity> checkSight=false（ZombieEntity.cpp:529-531），
// 不查视线直接锁定门后村民为目标，沿 +Z 寻路撞门。@e[type=zombie,limit=1,sort=nearest] 取距玩家
// （命令源 (0,2,6)）最近的 1 只僵尸（即 test.spawn 那只，距玩家 √(9+0+25)≈5.8 格；limit=1 规避
// "多实体"错误，独占 batch 保证区域内仅此一只僵尸）。
function spawnBreakingZombie(test: Test, player: { chat: (cmd: string) => void }, difficulty: "hard" | "normal"): void {
    // test.spawn 僵尸（不走 finalizeSpawn，m_canBreakDoors 默认 false，需 /data merge 赋能）。
    test.spawn("minecraft:zombie", ZOMBIE_POS);

    test.runAtTickTime(5, () => {
        player.chat(`/difficulty ${difficulty}`);
        // /data merge entity 设 CanBreakDoors:1b：readAdditionalSaveData→setBreakDoorsAbility(true) 注册 BreakDoorGoal。
        player.chat('/data merge entity @e[type=zombie,limit=1,sort=nearest] {CanBreakDoors:1b}');
    });
}

// 诊断信息：门上下半 typeId + 僵尸数量与位置（onTimeout 时打印辅助定位失败原因）。
function diagnose(test: Test): string {
    const zombies = test.getDimension().getEntities({
        type: "minecraft:zombie",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
    });
    const zInfo = zombies.length > 0
        ? `z=${zombies.length},pos=(${zombies[0].location.x.toFixed(1)},${zombies[0].location.y.toFixed(1)},${zombies[0].location.z.toFixed(1)})`
        : `z=0`;
    const doorUpper = test.getBlock(DOOR_UPPER) as unknown as { typeId?: string } | undefined;
    return `doorLower=${getDoorTypeId(test)},doorUpper=${doorUpper?.typeId ?? "?"},${zInfo}`;
}

// 困难难度下具备破门能力的僵尸破坏木门追村民（wiki tech_僵尸.txt#破门：困难难度僵尸破坏木门追击）。
//
// 流程：
//   1. buildDoorCorridor 搭走廊 + 木门。
//   2. 村民 test.spawn 于 VILLAGER_POS（门后 z 大侧）作诱饵。
//   3. 创造玩家 spawn 于 PLAYER_POS 角落作命令源，spawnBreakingZombie 生成破门僵尸。
//   4. 僵尸锁定村民（checkSight=false）沿 +Z 寻路撞门 collidedHorizontally →
//      BreakDoorGoal::shouldExecute 通过（Hard 谓词放行）→ tick 240 tick 破门。
//   5. pollUntilSucceed 轮询门下半变 air（updatePostPlacement 联动清除整门）。
//
// 独占 batch night_hard_door（前缀 night 自动夜晚避免亡灵白天燃烧干扰；独占避免 /difficulty 世界级污染
// 同批其他测试）+ runOnFinish 恢复 normal 难度。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_僵尸.txt#破门（困难难度破坏木门）
function zombieBreaksWoodenDoorOnHard(test: Test): void {
    buildDoorCorridor(test);

    // 村民诱饵（门后 z=5）：僵尸对村民 checkSight=false 直接锁定，沿 +Z 寻路撞门。
    test.spawn("villager_v2", VILLAGER_POS);

    // 创造玩家（默认创造，permLevel=4 可执行管理命令）角落作命令源。门 z=3 挡射线致僵尸看不到玩家
    // （canSee 射线穿过门薄层被阻挡），不被 NearestAttackableTargetGoal<Player> 选为目标。
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "cmd");

    spawnBreakingZombie(test, player, "hard");

    // 恢复默认 normal 难度，防污染后续批次（/difficulty 世界级跨测试持久化不自动重置）。
    test.runOnFinish(() => {
        player.chat("/difficulty normal");
    });

    // 轮询：破门完成后门下半变 air（updatePostPlacement 联动清除整门）。
    // 时序：tick0 spawn + tick5 赋能 + 僵尸锁定村民+寻路接近门（~40-80 tick）+ 破门 240 tick + 余量。maxTick=650。
    pollUntilSucceed(test, () => {
        return getDoorTypeId(test) === "minecraft:air";
    }, {
        startTick: 60,
        interval: 20,
        maxTick: 650,
        onTimeout: () => {
            test.assert(false,
                `zombie did not break door on hard (${diagnose(test)}, expected doorLower=air)`);
        },
    });
}

// 普通难度下具备破门能力的僵尸不破坏木门（wiki tech_僵尸.txt#破门：仅困难难度破门，普通只开门）。
//
// 对照测试，验证僵尸破门难度谓词修复（Normal+Hard → 仅 Hard）：
//   - Hard 测试（zombie_breaks_wooden_door_on_hard）破门通过；
//   - 本测试 Normal 难度 + 同样 CanBreakDoors 僵尸 + 木门 + 村民诱饵，僵尸撞门但 BreakDoorGoal
//     难度谓词 _isValidDifficulty()=false（Normal 不满足仅 Hard）拒绝，门保持不破。
//
// 负向断言：若难度谓词修复失效（Normal 仍放行），门会被破坏变 air，测试失败暴露 bug。
//
// 流程同 zombieBreaksWoodenDoorOnHard，仅 /difficulty normal（不切 hard）。
// 等待足够时间（覆盖 Hard 测试的破门时序 240+寻路）后断言门仍为 oak_door（未破）。
//
// 独占 batch night_normal_door。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_僵尸.txt#破门（普通难度不破门）
function zombieDoesNotBreakDoorOnNormal(test: Test): void {
    buildDoorCorridor(test);

    test.spawn("villager_v2", VILLAGER_POS);

    const player = test.spawnSimulatedPlayer(PLAYER_POS, "cmd");

    spawnBreakingZombie(test, player, "normal");

    // 负向断言：等 500 tick（覆盖僵尸锁定村民+寻路接近门 + Hard 测试破门 240 tick 时序）后断言门仍为 oak_door。
    // 用 runAtTickTime 单点判定（负向断言：须等完整破门窗口过才判定，不能用 pollUntilSucceed
    // "条件满足即 succeed"——门未破是常态，poll 会立即 succeed 漏掉破门）。
    test.runAtTickTime(500, () => {
        const t = getDoorTypeId(test);
        test.assert(t === "minecraft:oak_door",
            `zombie broke door on normal (${diagnose(test)}, expected doorLower=oak_door — difficulty predicate regressed)`);
        test.succeed();
    });
}

export function registerZombieBreakDoorTests(): void {
    GameTest.register("MobBehaviorTests", "zombie_breaks_wooden_door_on_hard", zombieBreaksWoodenDoorOnHard)
        .batch("night_hard_door")
        .structureName("gametests:creeper_pit")
        .maxTicks(750);

    GameTest.register("MobBehaviorTests", "zombie_does_not_break_door_on_normal", zombieDoesNotBreakDoorOnNormal)
        .batch("night_normal_door")
        .structureName("gametests:creeper_pit")
        .maxTicks(600);
}
