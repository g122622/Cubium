// 实体类命令 GameTest：/summon /kill 等。
//
// 覆盖 wiki 命令章节核心行为：
//   - /summon：生成实体到指定坐标或执行者位置（Ref: wiki summon.txt）
//   - /kill：清除实体（含 @e 选择器按 type 过滤）（Ref: wiki kill.txt）
//
// 设计要点：
//   1. /summon <entity> <x> <y> <z> 用世界绝对坐标（worldLocation 转换）。
//   2. /kill @e[type=zombie,distance=..10] 按选择器过滤清除；必须加 distance 区域限定，
//      因 @e 无距离谓词时是全维度无范围门控选择器，会误杀同批并行测试的 zombie（详见
//      killRemovesByTypeSelector 注释）。volume 谓词限定本结构范围，与 getEntities 区域计数一致。
//   3. 实体生成/清除非当 tick 生效（spawn 经 finalizeSpawn、kill 经 remove + 下一 tick 出列），
//      用 pollUntilSucceed 轮询区域计数。
//   4. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家站 (5,1,5)。
//      区域限定用 (1,1,1)..(7,5,7) 全空气腔计数，排除 SimulatedPlayer（按 type 过滤）。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const PLAYER_POS = { x: 5, y: 1, z: 5 };
// 区域计数范围：cmd_arena 内部空气腔 (1,1,1)..(7,5,7)。
const AREA_FROM = { x: 1, y: 1, z: 1 };
const AREA_VOLUME = { x: 7, y: 5, z: 7 };

/** 区域内指定 type 实体数量（排除 SimulatedPlayer，因 player 是独立 type）。 */
function countEntities(test: Test, type: string): number {
    return test.getDimension().getEntities({
        type,
        location: test.worldLocation(AREA_FROM),
        volume: AREA_VOLUME,
    }).length;
}

function worldCoords(test: Test, rel: { x: number; y: number; z: number }): string {
    const w = test.worldLocation(rel);
    return `${Math.floor(w.x)} ${Math.floor(w.y)} ${Math.floor(w.z)}`;
}

// /summon 在指定坐标生成实体。
// Ref: wiki summon.txt（summon <entity> [pos] 生成实体到指定坐标）
function summonSpawnsEntity(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    player.chat(`/summon minecraft:zombie ${worldCoords(test, { x: 3, y: 2, z: 3 })}`);

    // /summon 经 chat 命令同步生成（spawnEntity 同步入 EntityManager），但 chat 命令从 JS 发出到 C++
    // 处理存在 tick 延迟（命令包队列 + 下一 tick 处理）。全量跑高负载下该延迟偶发拉长，故用密集检查点
    // （startTick=5、interval=5、maxTick=100，共 20 个检查点）替代默认稀疏检查点（4 个：10/30/50/60），
    // 消除"检查点稀疏 + chat 时序抖动"导致的偶发超时。summon 功能正常时 zombie 必在数 tick 内出现，
    // maxTick=100 不掩盖失效（失效时 100 tick 仍 0 zombie 照样 FAIL）。
    pollUntilSucceed(test, () => countEntities(test, "zombie") >= 1, {
        startTick: 5,
        interval: 5,
        maxTick: 100,
        onTimeout: () => test.assert(false, "summon did not spawn zombie"),
    });
}

// /summon 在指定坐标生成实体，验证生成位置正确（实体坐标应在目标格附近）。
// Ref: wiki summon.txt（实体生成在指定坐标）
function summonSpawnsAtPosition(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const targetRel = { x: 2, y: 2, z: 2 };
    player.chat(`/summon minecraft:cow ${worldCoords(test, targetRel)}`);

    pollUntilSucceed(test, () => {
        const cows = test.getDimension().getEntities({
            type: "cow",
            location: test.worldLocation(AREA_FROM),
            volume: AREA_VOLUME,
        });
        if (cows.length < 1) return false;
        // 验证牛生成在目标格附近（实体坐标与目标世界坐标差距 < 2 格）。
        const target = test.worldLocation(targetRel);
        const c = cows[0];
        return Math.abs(c.location.x - target.x) < 2 && Math.abs(c.location.z - target.z) < 2;
    }, {
        maxTick: 60,
        onTimeout: () => test.assert(false, "summon cow not at target position"),
    });
}

// /kill @e[type=zombie,distance=..10] 清除本结构内 zombie，cow 保留（选择器 type 过滤）。
//
// 必须加 distance=..10 区域限定：@e[type=zombie] 不带距离谓词时是【全维度无范围门控】选择器
// （对齐 vanilla EntitySelector：无 distance/dx/dy/dz 时 collectAllEntities 走 forEachEntity 遍历
// 该 world 全量实体，仅按 type 过滤，见 EntityResolver.cpp collectAllEntities 的无 AABB 分支）。
// GameTest 同批并行测试共享单一 ServerWorld，结构网格间距仅 32 格（StructureGridSpawner），
// 无区域限定的 /kill @e[type=zombie] 会杀光整个 overworld 所有 zombie——误杀同批并行的
// summon_spawns_entity（tick1 生成的 zombie）、spawner_* 等测试的 zombie，致它们 zombie=0 假失败
// （单独跑通过、全量跑必失败，根因即此跨测试误杀）。
//
// distance=..10：从执行者 player(5,1,5) 欧氏距离 10 格，覆盖整个 cmd_arena 9×7×9 结构（最远角点
// (0,6,0) 距 player √75≈8.66），且远小于结构间距 32，绝不触及相邻结构的实体。cow(4,2,4) 距 player
// √2，本就在范围内但因 type=zombie 不被选中——distance 限定只收紧 zombie 选择集，不影响 cow 存活判定。
// Ref: wiki kill.txt（kill 接受选择器，按 type 过滤）
// Ref: EntityResolver.cpp collectAllEntities（无 AABB 走 forEachEntity 全量遍历）
function killRemovesByTypeSelector(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 预置 zombie + cow。
    test.spawn("zombie", { x: 2, y: 2, z: 2 });
    test.spawn("cow", { x: 4, y: 2, z: 4 });
    // 等实体生成稳定后 kill。
    test.runAtTickTime(5, () => {
        player.chat("/kill @e[type=zombie,distance=..10]");
    });

    pollUntilSucceed(test, () => countEntities(test, "zombie") === 0 && countEntities(test, "cow") >= 1, {
        startTick: 10,
        maxTick: 80,
        onTimeout: () => test.assert(false,
            `kill @e[type=zombie] failed: zombie=${countEntities(test, "zombie")}, cow=${countEntities(test, "cow")}`),
    });
}

// /kill @e[type=zombie,distance=..10] 清除本结构内 zombie（@e 含玩家，但 type=zombie 排除玩家）。
//
// 同 killRemovesByTypeSelector，必须加 distance=..10 区域限定避免全维度 /kill 误杀同批并行测试的
// zombie（详见 killRemovesByTypeSelector 注释）。@e[type=zombie] 已排除玩家，distance 限定不收紧玩家。
// Ref: wiki kill.txt（@e 选择所有实体）
// Ref: EntityResolver.cpp collectAllEntities（无 AABB 走 forEachEntity 全量遍历）
function killAllRemovesEntities(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    test.spawn("zombie", { x: 2, y: 2, z: 2 });
    test.spawn("zombie", { x: 4, y: 2, z: 4 });
    test.runAtTickTime(5, () => {
        // @e[type=zombie,distance=..10] 限定本结构范围，避免误杀同批并行测试的 zombie；
        // type=zombie 排除玩家（玩家 Creative 被 kill 会 respawn，但保险起见限定 type）。
        player.chat("/kill @e[type=zombie,distance=..10]");
    });

    pollUntilSucceed(test, () => countEntities(test, "zombie") === 0, {
        startTick: 10,
        maxTick: 80,
        onTimeout: () => test.assert(false,
            `kill @e failed: zombie=${countEntities(test, "zombie")}`),
    });
}

// /summon 多次生成实体，验证数量累积（每次 summon 生成一个）。
// Ref: wiki summon.txt（每次 summon 生成一个实体）
function summonMultipleAccumulates(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 连续 summon 3 个 chicken 在不同坐标。
    player.chat(`/summon minecraft:chicken ${worldCoords(test, { x: 2, y: 2, z: 2 })}`);
    player.chat(`/summon minecraft:chicken ${worldCoords(test, { x: 3, y: 2, z: 3 })}`);
    player.chat(`/summon minecraft:chicken ${worldCoords(test, { x: 4, y: 2, z: 4 })}`);

    pollUntilSucceed(test, () => countEntities(test, "chicken") >= 3, {
        maxTick: 60,
        onTimeout: () => test.assert(false,
            `summon x3 failed: chicken=${countEntities(test, "chicken")}`),
    });
}

// /summon 在和平难度下召唤怪物类实体应被拒绝（对齐 Java SummonCommand.createEntity：
// difficulty==PEACEFUL 且 !isAllowedInPeaceful → 拒绝生成）。
//
// SummonCommand 经 world->difficulty() == Difficulty::Peaceful && !entity::isPeaceful(classification)
// 守卫（对齐 SummonCommand.java:86 与 SpawnEggItem::spawnEntity 同款范式）：zombie 属 Monster 分类，
// isPeaceful(Monster)=false，故 peaceful 难度下 /summon zombie 直接 sendError 返回 0 不生成实体。
// 修复前无此守卫，peaceful 下 zombie 照常生成（与 vanilla 偏差）。
//
// 设 peaceful → /summon zombie → pollUntilSucceed 断言 zombie==0（被守卫拦截未生成）。
// 对照组见 summonAllowedOnPeacefulForPassive（cow 动物类 isPeaceful=true 守卫放行）。
//
// 【并行污染隔离】/difficulty peaceful 是世界级单例状态，GameTest 共享单一 ServerWorld 跨测试持久化不
// 自动重置，且 peaceful 会触发 DespawnManager 清全维度怪物（同 [[gametest-world-state-gamerule-difficulty-batch-isolation]]）。
// 故独占 batch（summon_peaceful_solo）串行执行 + runOnFinish 恢复 normal，防污染同批/后续依赖默认难度
// 或依赖怪物存活的测试。cmd_arena 封顶遮光，亡灵白天不燃烧，无需 night 前缀（对齐 difficulty_clear_solo）。
// Ref: wiki summon.txt（summon 受和平难度限制：怪物类在和平不生成）
// Ref: SummonCommand.cpp（和平难度守卫）、SummonCommand.java:86（vanilla isAllowedInPeaceful 守卫）
function summonBlockedOnPeacefulForMonster(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    // 先设 peaceful 难度（世界级，命令从 chat 发出到 C++ 处理有 tick 延迟，用 runAtTickTime 确保难度
    // 生效后再 summon）。
    player.chat("/difficulty peaceful");
    test.runOnFinish(() => {
        player.chat("/difficulty normal");
    });

    // tick 5 时难度已生效，summon zombie 应被守卫拦截。
    test.runAtTickTime(5, () => {
        player.chat(`/summon minecraft:zombie ${worldCoords(test, { x: 3, y: 2, z: 3 })}`);
    });

    // 轮询断言 zombie 未生成（守卫拦截）。修复前 zombie 会生成致 count>=1。
    // startTick=10 给命令处理 + 难度生效余量，maxTick=80 充分覆盖（守卫拦截后 zombie 恒 0，超时即 FAIL）。
    pollUntilSucceed(test, () => countEntities(test, "zombie") === 0, {
        startTick: 10,
        maxTick: 80,
        onTimeout: () => test.assert(false,
            `summon zombie should be blocked on peaceful but zombie spawned ` +
            `(count=${countEntities(test, "zombie")})`),
    });
}

// /summon 在和平难度下召唤动物类实体应成功（对照 summonBlockedOnPeacefulForMonster）。
//
// cow 属 Creature 分类，isPeaceful(Creature)=true，守卫放行（对齐 vanilla：动物类 allowedInPeaceful=true）。
// 设 peaceful → /summon cow → pollUntilSucceed 断言 cow==1（守卫只拦怪物不误伤动物）。
//
// 同 summonBlockedOnPeacefulForMonster 的并行污染隔离（独占 batch + runOnFinish 恢复 normal）。
// Ref: wiki summon.txt（summon 动物在和平难度可生成）
// Ref: SummonCommand.cpp（和平难度守卫仅拦 !isPeaceful 的怪物类）
function summonAllowedOnPeacefulForPassive(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/difficulty peaceful");
    test.runOnFinish(() => {
        player.chat("/difficulty normal");
    });

    test.runAtTickTime(5, () => {
        player.chat(`/summon minecraft:cow ${worldCoords(test, { x: 3, y: 2, z: 3 })}`);
    });

    // 轮询断言 cow 生成（守卫放行动物类）。
    pollUntilSucceed(test, () => countEntities(test, "cow") >= 1, {
        startTick: 10,
        maxTick: 80,
        onTimeout: () => test.assert(false,
            `summon cow should succeed on peaceful but cow not spawned ` +
            `(count=${countEntities(test, "cow")})`),
    });
}

// /summon 在越界坐标（Y 超出 ±20,000,000 可生成高度边界）应被守卫拦截不生成实体
// （对齐 Java SummonCommand.createEntity 首行守卫 SummonCommand.java:83-85：
// !Level.isInSpawnableBounds(BlockPos.containing(pos)) → 抛 INVALID_POSITION）。
//
// 守卫顺序（对齐 vanilla）：边界校验 → peaceful 校验 → 创建。Cubium 修复前无边界守卫，
// 越界坐标会跳过校验直接 setPosition/spawnEntity，可能崩溃或产生越界实体。
//
// 测试用绝对坐标 Y=21,000,000（刚越 isInSpawnableBounds 上界 20,000,000）触发守卫：
// normal 难度（避开 peaceful 守卫干扰，确保是边界守卫而非 peaceful 守卫拦截），
// /summon minecraft:cow 0 21000000 0 → 边界守卫 sendError(commands.summon.invalidPosition) 返回 0，
// cow 不生成。pollUntilSucceed 断言 cow==0。
//
// 对照见 summonAllowedWithinBounds（合法坐标 cow 正常生成，证明守卫不误拦合法坐标）。
// Ref: wiki summon.txt（summon 对越界坐标拒绝生成）
// Ref: SummonCommand.cpp（边界校验守卫 isInSpawnableBounds）、SummonCommand.java:83-85（vanilla 守卫）
function summonRejectedForOutOfBoundsY(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    // normal 难度（默认即 normal，显式声明以排除 peaceful 守卫干扰）。Y=21000000 越上界触发边界守卫。
    // 用绝对坐标 0 21000000 0：X/Z=0 在世界边界内合法，Y=21000000 是唯一越界变量，确保是边界守卫拦截。
    player.chat("/summon minecraft:cow 0 21000000 0");

    // 轮询断言 cow 未生成（边界守卫拦截）。修复前 cow 可能生成（或崩溃）致 count>=1。
    // maxTick=80：守卫同步拦截后 cow 恒 0，超时即 FAIL。
    pollUntilSucceed(test, () => countEntities(test, "cow") === 0, {
        startTick: 5,
        maxTick: 80,
        onTimeout: () => test.assert(false,
            `summon cow should be rejected for out-of-bounds Y but cow spawned ` +
            `(count=${countEntities(test, "cow")})`),
    });
}

// /summon 在合法坐标内正常生成实体（对照 summonRejectedForOutOfBoundsY）。
//
// 守卫 isInSpawnableBounds 对合法坐标（结构内 Y∈[1,5]、X/Z∈[1,7]）放行。normal 难度 + 合法坐标，
// /summon minecraft:cow → 守卫全通过 → 生成 cow。pollUntilSucceed 断言 cow>=1。
// 证明边界守卫只拦越界不误拦合法坐标。
// Ref: wiki summon.txt（summon 合法坐标正常生成）
function summonAllowedWithinBounds(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    // 合法坐标（结构内 (3,2,3)），守卫放行，cow 正常生成。
    player.chat(`/summon minecraft:cow ${worldCoords(test, { x: 3, y: 2, z: 3 })}`);

    pollUntilSucceed(test, () => countEntities(test, "cow") >= 1, {
        startTick: 5,
        maxTick: 80,
        onTimeout: () => test.assert(false,
            `summon cow should succeed within bounds but cow not spawned ` +
            `(count=${countEntities(test, "cow")})`),
    });
}

export function registerEntityCommandTests(): void {
    GameTest.register("CommandTests", "summon_spawns_entity", summonSpawnsEntity)
        .structureName("gametests:cmd_arena")
        .maxTicks(120);

    GameTest.register("CommandTests", "summon_spawns_at_position", summonSpawnsAtPosition)
        .structureName("gametests:cmd_arena")
        .maxTicks(80);

    GameTest.register("CommandTests", "kill_removes_by_type_selector", killRemovesByTypeSelector)
        .structureName("gametests:cmd_arena")
        .maxTicks(100);

    GameTest.register("CommandTests", "kill_all_removes_entities", killAllRemovesEntities)
        .structureName("gametests:cmd_arena")
        .maxTicks(100);

    GameTest.register("CommandTests", "summon_multiple_accumulates", summonMultipleAccumulates)
        .structureName("gametests:cmd_arena")
        .maxTicks(80);

    // 和平难度校验是世界级状态，独占 batch 串行避免并行互相覆盖 + runOnFinish 恢复 normal（见
    // summonBlockedOnPeacefulForMonster 注释的并行污染隔离说明）。
    GameTest.register("CommandTests", "summon_blocked_on_peaceful_for_monster", summonBlockedOnPeacefulForMonster)
        .structureName("gametests:cmd_arena")
        .batch("summon_peaceful_solo")
        .maxTicks(100);

    GameTest.register("CommandTests", "summon_allowed_on_peaceful_for_passive", summonAllowedOnPeacefulForPassive)
        .structureName("gametests:cmd_arena")
        .batch("summon_peaceful_passive_solo")
        .maxTicks(100);

    // 边界校验测试：守卫不修改世界级状态（不改难度/gamerule），可用默认 batch 并行，无需独占。
    GameTest.register("CommandTests", "summon_rejected_for_out_of_bounds_y", summonRejectedForOutOfBoundsY)
        .structureName("gametests:cmd_arena")
        .maxTicks(100);

    GameTest.register("CommandTests", "summon_allowed_within_bounds", summonAllowedWithinBounds)
        .structureName("gametests:cmd_arena")
        .maxTicks(100);
}
