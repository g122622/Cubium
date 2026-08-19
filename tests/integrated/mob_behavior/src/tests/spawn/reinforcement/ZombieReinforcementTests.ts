// 僵尸增援（ZombieEntity::trySummonReinforcements）行为 GameTest。
//
// 覆盖 wiki tech_僵尸.txt#增援：僵尸在 Hard 难度被攻击时，概率召唤增援僵尸（同类型），
// 增援生成在攻击目标附近（偏移 7-40 格），不查光照，查 doMobSpawning。
//
// C++ 链路：
//   - ZombieEntity::hurt（ZombieEntity.cpp）：MonsterEntity::hurt 成功后调
//     trySummonReinforcements(source.getEntity())，用伤害来源实体作为增援的显式攻击目标。
//   - trySummonReinforcements：Hard 难度门控（canZombieReinforce）+
//     概率门（attributes().getValue(ZOMBIE_SPAWN_REINFORCEMENTS)，random>=spawnChance 拒绝）+
//     目标解析（explicitTarget||attackTarget）→ _trySpawnReinforcement。
//   - _trySpawnReinforcement：查 doMobSpawning → 50 次随机偏移尝试（各轴 offset=
//     nextInt(7,40)*nextInt(-1,1)，offset 可为 0）→ EntitySpawnPlacementRegistry::canSpawnEntity
//     OnGround 放置检查（脚底 isSolidSide(Up) + 生成位/上方位 _isValidSpawnBlock 可通行）→
//     7 格内无存活玩家检查（getClosestPlayer(spawnPos,7.0)）→ 创建僵尸+setPosition+
//     碰撞检查+finalizeSpawn+setAttackTarget+spawnEntity → caller/callee charge 修饰符。
//
// 关键事实（核查 Java 1.21.11 源码 + Cubium 实现确认）：
//   1. ZOMBIE_SPAWN_REINFORCEMENTS 属性默认值 0.0。test.spawn（GameTestHelper::spawnEntity）只
//      type->create + enablePersistence + setPosition + spawnEntity，不走 finalizeSpawn，故 test.spawn
//      的僵尸增援属性保持 0.0，trySummonReinforcements 概率门 random>=0.0 恒 true → 拒绝 → 增援永不触发。
//      故必须用 /attribute 命令强制设增援属性为 1.0（必触发概率门）。
//   2. /attribute <target> zombie.spawn_reinforcements base set <value>（AttributeCommand _setBaseValue）
//      调 setBaseValue 覆盖属性。权限 2（创造玩家 permLevel=2，SimulatedPlayer）。
//      目标用 @e[type=zombie,limit=1,sort=nearest] 选择器（limit=1 规避"多实体"错误，即便有自然生成
//      残留也只取距玩家最近 1 只）。distance 从命令源（玩家）位置计算。
//   3. canMonsterSpawnInLightPredicate 是 no-op（EntitySpawnPlacementRegistry 恒返 true，光照检查
//      在 NaturalSpawner 中进行）——增援不查光照，露天亮处仍生成。
//   4. 7 格内无存活玩家约束：增援生成位 7 格球内不能有存活玩家。玩家需近战攻击僵尸（attackEntity
//      在 ENTITY_INTERACTION_RANGE 内约 4.5 格），但攻击瞬间玩家在僵尸旁 2 格，部分小 offset（7）
//      生成位 7 格内有玩家被拒；大 offset（>=15）生成位距玩家 >=13 格通过。50 次尝试中大 offset
//      合法位存在即成功。
//   5. doMobSpawning 必须 true（_trySpawnReinforcement 查此规则），GameTestServer 默认 true，显式确认。
//   6. attackEntity 已实现（ScriptSimulatedPlayer 转发 Player::attack），用 DamageSources::playerAttack
//      （EntitySource）→ ZombieEntity::hurt 的 source.getEntity() 返回玩家 → 显式目标。
//   7. Player::attack 对 Creative 玩家也走正常 hurt（baseDamage=1.0），无 Java 创造秒杀分支（Cubium 偏差，
//      无害——1.0 伤害触发 hurt 成功即触发增援）。本测试用 Survival 玩家攻击以贴近原版伤害链路。
//   8. /attribute 与 /difficulty 需 permLevel=2（创造模式），attackEntity 需 Survival 造伤害——
//      先创造执行命令设属性+难度，再切 Survival 攻击（命令在切之前执行生效即可）。
//
// === 坐标映射（根因关键，曾致 place=33 全失败）===
// GameTestServer gridStartY=-59（origin.y，结构方块位置）。MinecraftStructurePlacer 把结构文件 y=0
// 放在 placeOrigin.y=origin.y+1=-58。helper 相对坐标原点=origin（结构方块），故：
//   helper 相对 y=N  →  世界 y = origin.y + N = -59 + N  →  结构文件 y = N-1
// reinforce_arena 结构文件布局：y=0 grass_block 地板，y=1..6 air/glass 墙。
// OnGround 放置检查要求：生成位是 air，脚下是 solid（isSolidSide(Up)）。
//   - 唯一合法生成位 = 文件 y=1（air，脚下 y=0 grass_block 支撑）→ 世界 y=-57 → helper 相对 y=2。
//   - 文件 y=0 是 grass_block（非 air，place 拒）；文件 y>=2 air 但脚下 air 无支撑（OnGround 拒）。
// 故僵尸必须站在文件 y=1（helper 相对 y=2，世界 y=-57），使 baseY=-57；增援 offsetY=0 生成位
// spawnY=-57=文件 y=1（air，脚下 y=0 grass_block）通过 OnGround。
//   - 曾错设 ZOMBIE_POS.y=1 → 世界 y=-58=文件 y=0（grass_block），僵尸卡在地板实体内，
//     baseY=-58；增援 offsetY=0 生成位 spawnY=-58=文件 y=0（grass_block 非 air）→ place 全失败。
//
// === 隔离自然生成污染（skyAccess 露天）===
// NaturalSpawner 不查 doMobSpawning gamerule（与原版偏差），关闭自然生成只能靠光照/距离。
// reinforce_arena 默认埋在 gridStartY=-59 地下 worldgen 石头中，结构上方是石头 → 内部 skyLight=0
// 黑暗 → NaturalSpawner 每 tick 在玩家周围 8 chunk 随机选位尝试生成怪物，night batch 黑暗结构内
// 会持续自然生成 zombie 污染 countZombies（假阳性满足 zombie>=2）。
// 故必须 .skyAccess(true)：MinecraftStructurePlacer 清空结构 footprint 正上方至 MAX_BUILD_HEIGHT
// 所有 worldgen 方块，制造露天列使 skyLight=15。NaturalSpawner 怪物光照门槛 max(skyLight,blockLight)
// <=7，露天 15>7 拒绝 → 隔离自然生成，只增援生成（增援 no-op 谓词不查光）。
//   - .setupTicks(20)：清空上方方块后光照变更入队 m_lightQueue，需若干世界 tick 由 ServerWorld::tick
//     批量重算 skyLight 达 15。setupTicks 阶段让世界先 tick 20 次让光照稳定，再正式跑测试体。
//   - night batch + skyAccess 露天：night batch dayTime=18000（夜晚），但 Cubium getSkyLight 返回
//     原始天空光不减 skyDarkening，故露天 skyLight 仍=15（NaturalSpawner 怪物拒绝）；night batch 仅
//     影响亡灵白天燃烧判定（夜晚亡灵不燃，僵尸+增援僵尸存活）。
//
// 结构选择：reinforce_arena（81×7×81 露天草地大场地）。
//   - 中心 (40,2,40) helper 放僵尸+玩家（世界 (40,-57,40)/(42,-57,40)），增援偏移 7..40 各轴落在
//     [1,79] helper 范围内（结构内）。
//   - grass_block 地板（文件 y=0）满足 OnGround 脚底支撑。
//   - skyAccess 露天 skyLight=15 隔离自然生成；增援 no-op 谓词不查光仍生成。
//
// 判定手段：getEntities 区域计数 zombie>=2（原僵尸+增援）。区域限定 reinforce_arena 全场。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_僵尸.txt#增援
// Ref: ZombieEntity.cpp trySummonReinforcements/_trySpawnReinforcement
// Ref: AttributeCommand.cpp _setBaseValue（/attribute base set）
// Ref: EntitySpawnPlacementRegistry canMonsterSpawnInLightPredicate（no-op，增援不查光照）
// Ref: MinecraftStructurePlacer.cpp（gridStartY=-59 结构埋地下 + skyAccess 清空上方制露天）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// reinforce_arena 结构尺寸 81×7×81（helper 相对坐标 x,z∈[0,80], y∈[0,6]）。
const ARENA_FROM = { x: 0, y: 0, z: 0 };
const ARENA_VOLUME = { x: 81, y: 7, z: 81 };

// 僵尸与玩家位置（helper 相对坐标）。
// y=2 → 世界 y=-57 → 结构文件 y=1（air 腔），脚下文件 y=0 grass_block 支撑：
//   - 僵尸自身 OnGround 满足（站在 grass_block 上方 air）。
//   - baseY=-57，增援 offsetY=0 生成位 spawnY=-57=文件 y=1（air，脚下 y=0 grass_block）通过 OnGround。
// 玩家紧邻僵尸 (42,2,40)（距 2 格，attackEntity 在 Survival ENTITY_INTERACTION_RANGE 默认 3.0 内）。
const ZOMBIE_POS = { x: 40, y: 2, z: 40 };
const PLAYER_POS = { x: 42, y: 2, z: 40 };

// 区域内统计 zombie 实体数（区域限定排除并行测试污染 + 自然生成残留）。
function countZombies(test: Test): number {
    return test.getDimension().getEntities({
        type: "zombie",
        location: test.worldLocation(ARENA_FROM),
        volume: ARENA_VOLUME,
    }).length;
}

// 僵尸在 Hard 难度被攻击后召唤增援（wiki tech_僵尸.txt#增援）。
//
// 流程：
//   1. test.spawn 僵尸（增援属性默认 0.0，需 /attribute 强制）。
//   2. 创造玩家执行 /difficulty hard（canZombieReinforce 要求 Hard）。
//   3. 创造玩家执行 /attribute @e[type=zombie,limit=1,sort=nearest] zombie.spawn_reinforcements base set 1.0
//      （强制增援属性 1.0，概率门必通过）。
//   4. 创造玩家执行 /gamerule doMobSpawning true（_trySpawnReinforcement 查此规则，显式确认）。
//   5. 切 Survival（attackEntity 造伤害链路）。
//   6. 循环 attackEntity（玩家攻击冷却约 12.5 tick，每 20 tick 攻击一次保证满冷却；每次 hurt 触发
//      一次 trySummonReinforcements，属性 1.0 概率门必通过，几何合法即生成增援）。
//   7. pollUntilSucceed 断言 zombie>=2（原僵尸+增援）。
//
// 几何：玩家 (42,2,40) 距僵尸 (40,2,40) 2 格，attackEntity 在交互范围内。增援生成位 zombie+
// offset(7..40)，玩家距小 offset（7）生成位 5~9 格（部分 7 内被拒），大 offset（>=15）生成位
// 距玩家 >=13 格（7 外通过）。50 次尝试中大 offset 合法位存在即生成成功。
//
// night batch：避免亡灵白天燃烧（skyAccess 露天，day batch 僵尸会燃烧死亡）。
// maxTick=2000 覆盖多轮攻击（每 20 tick 一次，约 98 轮）+ 增援几何尝试。
//
// 注：合法生成位仅 offsetY=0（文件 y=1 air 脚下 grass_block），概率 1/3；offsetX/offsetZ 大偏移
// （>=7）使生成位距玩家 >7 格通过玩家检查。50 次尝试期望 ~11 次成功（1/3 × 2/3 × 50）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_僵尸.txt#增援（Hard 难度召唤增援）
function zombieSummonsReinforcementOnHurtHard(test: Test): void {
    // 清场：移除世界所有活体实体（GameTestServer 世界已 tick 一段时间，setupTicks 期间自然生成可能
    // 散落 zombie；skyAccess 露天 skyLight=15 理论拒绝怪物自然生成，但结构放置边界/世界原生洞穴可能
    // 引入残留 zombie，污染 @e[type=zombie] 选择器致 /attribute 多实体失败）。killAllEntities 同步 discard
    // 所有活体（含玩家），须在其后重新 spawn 玩家+僵尸。对齐 MobSpawnerTests.spawnerRequiresPlayer 范式。
    (test as any).killAllEntities();

    // 僵尸放 y=2（文件 y=1 air 腔，脚下 y=0 grass_block 支撑），使 offsetY=0 增援生成位 y=2（文件 y=1 air，
    // 脚下 y=0 grass_block）通过 OnGround 放置检查。
    const zombie = test.spawn("minecraft:zombie", ZOMBIE_POS);
    // 创造玩家执行管理命令（permLevel=2）。
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "attacker");

    // 1. 切 Hard 难度（canZombieReinforce 要求 Hard，DifficultyHelper）。
    player.chat("/difficulty hard");
    // 恢复默认 normal 难度，防污染后续批次（difficulty 世界级跨测试持久化不自动重置）。
    test.runOnFinish(() => {
        player.chat("/difficulty normal");
    });
    // 2. 强制僵尸增援属性为 1.0（test.spawn 不走 finalizeSpawn，属性默认 0.0，必须 /attribute 覆盖）。
    //    @e[type=zombie,limit=1,sort=nearest] 精确选中距玩家最近的 1 只僵尸（即 test.spawn 那只，距 2 格；
    //    limit=1 规避"多实体"错误，即便有自然生成残留也只取最近者）。
    player.chat("/attribute @e[type=zombie,limit=1,sort=nearest] zombie.spawn_reinforcements base set 1.0");
    // 3. 确认 doMobSpawning true（_trySpawnReinforcement 查此规则，GameTestServer 默认 true，显式确认）。
    player.chat("/gamerule doMobSpawning true");

    // 4. 切 Survival（attackEntity 造伤害链路；命令已在创造下执行生效）。
    player.chat("/gamemode survival");

    // 5. 循环攻击僵尸（每 20 tick 一次，保证满冷却；每次 hurt 触发 trySummonReinforcements）。
    //    属性 1.0 概率门必通过，几何合法即生成增援。pollUntilSucceed 轮询 zombie>=2。
    let tick = 30;
    while (tick <= 1900) {
        const t = tick;
        test.runAtTickTime(t, () => {
            player.attackEntity(zombie);
        });
        tick += 20;
    }

    pollUntilSucceed(test, () => countZombies(test) >= 2, {
        maxTick: 2000,
        onTimeout: () => test.assert(false,
            `zombie did not summon reinforcement on hurt hard (zombies=${countZombies(test)})`),
    });
}

export function registerZombieReinforcementTests(): void {
    GameTest.register("MobBehaviorTests", "zombie_summons_reinforcement_on_hurt_hard", zombieSummonsReinforcementOnHurtHard)
        .batch("night")
        .structureName("gametests:reinforce_arena")
        // skyAccess(true)：GameTestServer gridStartY=-59 把结构埋在地下 worldgen 石头中，结构上方全是
        // worldgen 方块致内部 skyLight=0 黑暗——NaturalSpawner 会持续自然生成 zombie 污染 countZombies。
        // skyAccess=true 让 MinecraftStructurePlacer 清空结构 footprint 正上方至世界顶部所有方块，制造
        // 露天列使 skyLight=15，NaturalSpawner 怪物光照门槛 max<=7 拒绝 → 隔离自然生成（只增援生成，
        // 增援 no-op 谓词不查光）。详见 ZombieTests.zombie_burns_in_daylight 同款注释。
        .skyAccess(true)
        // setupTicks(20)：清空上方方块后光照变更入队 m_lightQueue，需若干世界 tick 由 ServerWorld::tick
        // 批量重算 skyLight 达 15。setupTicks 阶段让世界先 tick 20 次让光照稳定，再正式跑测试体。
        .setupTicks(20)
        .maxTicks(2100);
}
