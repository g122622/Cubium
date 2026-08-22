// /worldborder 命令 GameTest：控制世界边界。
//
// 覆盖 wiki 命令章节核心行为（对齐 MC 1.21.11 WorldBorderCommand，Ref: Java
// net/minecraft/server/commands/WorldBorderCommand.java）：
//   - /worldborder set <size>：设置边界直径（立即）
//   - /worldborder add <distance>：在当前大小基础上增减（立即）
//   - /worldborder center <x> <z>：设置边界中心
//   - /worldborder damage amount <value>：设置每格伤害
//   - /worldborder damage buffer <value>：设置伤害缓冲距离
//   - /worldborder warning time <seconds>：设置警告时间
//   - /worldborder warning distance <blocks>：设置警告距离
//
// 脚本侧此前无 worldBorder 读取绑定，无法断言。本批测试伴随新增 world.getWorldBorder() 方法
// （Cubium 扩展，对齐基岩 world.getWorldBorder(): WorldBorder）：返回 WorldBorder JS 对象，opaque 持
// 主世界 IWorld*（非拥有），属性 size/center/damagePerBlock/damageSafeZone/warningBlocks/warningTime
// 每次从 IWorld::worldBorder() 取最新值。WorldBorder 是 common 层类型，IWorld::worldBorder() 是 common
// 层虚函数，故脚本绑定可直接调 getter，无需 ScriptWorldAccessor 值快照桥接（区别于 server 层的
// BossBar/WorldSpawn）。
//
// 设计要点：
//   1. 世界边界是世界级单例（ServerWorld::m_worldBorder 全局唯一），跨测试持久化不自动重置。故每个
//      测试独占 batch 串行 + runOnFinish 恢复初始边界（测试开始时读 getWorldBorder() 各属性保存，结束
//      时 chat 设回），防污染后续测试。
//   2. 默认 size=6.0E7（6000万），不测精确默认值（vanilla 默认 59999968 与 Cubium 6.0E7 有微差，
//      属已知偏差不在此测），只测"set 后读回等于设置值"。
//   3. /worldborder set <size> 无 time 参数立即生效，getSize 返回 targetSize（无渐变）。
//   4. /worldborder add <distance> 在当前 size 基础上加，可为负。测试先 set 100 再 add 50 断言 150。
//   5. size/center/damage 是浮点，用 === 比较（set 100 存 100.0，读回 100，JS number === 100 成立）。
//      warningTime/warningBlocks 是整数，createInt32 读回 JS number。
//   6. SimulatedPlayer::chat permLevel 固定=4（≥2 满足 worldborder 权限）。
//   7. cmd_arena 9×7×9：玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_世界边界.txt（世界边界命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// WorldBorder 快照（Cubium 扩展 world.getWorldBorder() 返回对象的属性）。
interface WorldBorderSnap {
    size: number;
    center: { x: number; z: number };
    damagePerBlock: number;
    damageSafeZone: number;
    warningBlocks: number;
    warningTime: number;
}

// 读世界边界快照（Cubium 扩展，TS 无类型用 as unknown as）。
function getBorder(): WorldBorderSnap {
    return (world as unknown as { getWorldBorder(): WorldBorderSnap }).getWorldBorder();
}

// 保存当前边界全部属性（用于 runOnFinish 恢复）。
function snapshotBorder(): WorldBorderSnap {
    return getBorder();
}

// 用 chat 命令恢复边界到快照值（runOnFinish 调用）。
function restoreBorder(player: { chat: (s: string) => void }, snap: WorldBorderSnap): void {
    player.chat(`/worldborder set ${snap.size}`);
    player.chat(`/worldborder center ${snap.center.x} ${snap.center.z}`);
    player.chat(`/worldborder damage amount ${snap.damagePerBlock}`);
    player.chat(`/worldborder damage buffer ${snap.damageSafeZone}`);
    player.chat(`/worldborder warning time ${snap.warningTime}`);
    player.chat(`/worldborder warning distance ${snap.warningBlocks}`);
}

// /worldborder set <size> 设置边界直径（立即，无 time）。
// set 100 后 getSize 应===100。
// Ref: Java WorldBorderCommand（setBorderSize，无 time 立即 setSize）
function worldborderSetSize(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const initial = snapshotBorder();

    player.chat("/worldborder set 100");

    const b = getBorder();
    test.assert(b.size === 100, `size should be 100, got ${b.size}`);

    test.runOnFinish(() => restoreBorder(player, initial));
    test.succeed();
}

// /worldborder add <distance> 在当前大小基础上增减（立即，无 time）。
// 先 set 100 再 add 50，断言 size===150。
// Ref: Java WorldBorderCommand（addBorderSize）
function worldborderAddSize(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const initial = snapshotBorder();

    player.chat("/worldborder set 100");
    player.chat("/worldborder add 50");

    const b = getBorder();
    test.assert(b.size === 150, `size should be 150 after add 50, got ${b.size}`);

    test.runOnFinish(() => restoreBorder(player, initial));
    test.succeed();
}

// /worldborder center <x> <z> 设置边界中心。
// center 10 20 后 center.x===10, center.z===20。
// Ref: Java WorldBorderCommand（setCenter）
function worldborderSetCenter(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const initial = snapshotBorder();

    player.chat("/worldborder center 10 20");

    const b = getBorder();
    test.assert(b.center.x === 10, `center.x should be 10, got ${b.center.x}`);
    test.assert(b.center.z === 20, `center.z should be 20, got ${b.center.z}`);

    test.runOnFinish(() => restoreBorder(player, initial));
    test.succeed();
}

// /worldborder damage amount <value> 设置每格伤害。
// damage amount 3 后 damagePerBlock===3。
// Ref: Java WorldBorderCommand（setDamageAmount）
function worldborderDamageAmount(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const initial = snapshotBorder();

    player.chat("/worldborder damage amount 3");

    const b = getBorder();
    test.assert(b.damagePerBlock === 3, `damagePerBlock should be 3, got ${b.damagePerBlock}`);

    test.runOnFinish(() => restoreBorder(player, initial));
    test.succeed();
}

// /worldborder damage buffer <value> 设置伤害缓冲距离（safeZone）。
// damage buffer 10 后 damageSafeZone===10。
// Ref: Java WorldBorderCommand（setDamageBuffer，基岩属性名 damageSafeZone）
function worldborderDamageBuffer(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const initial = snapshotBorder();

    player.chat("/worldborder damage buffer 10");

    const b = getBorder();
    test.assert(b.damageSafeZone === 10, `damageSafeZone should be 10, got ${b.damageSafeZone}`);

    test.runOnFinish(() => restoreBorder(player, initial));
    test.succeed();
}

// /worldborder warning time <seconds> 设置警告时间。
// warning time 30 后 warningTime===30。
// Ref: Java WorldBorderCommand（setWarningTime）
function worldborderWarningTime(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const initial = snapshotBorder();

    player.chat("/worldborder warning time 30");

    const b = getBorder();
    test.assert(b.warningTime === 30, `warningTime should be 30, got ${b.warningTime}`);

    test.runOnFinish(() => restoreBorder(player, initial));
    test.succeed();
}

// /worldborder warning distance <blocks> 设置警告距离。
// warning distance 20 后 warningBlocks===20。
// Ref: Java WorldBorderCommand（setWarningDistance，基岩属性名 warningBlocks）
function worldborderWarningDistance(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const initial = snapshotBorder();

    player.chat("/worldborder warning distance 20");

    const b = getBorder();
    test.assert(b.warningBlocks === 20, `warningBlocks should be 20, got ${b.warningBlocks}`);

    test.runOnFinish(() => restoreBorder(player, initial));
    test.succeed();
}

export function registerWorldBorderTests(): void {
    // 世界边界是世界级单例，独占 batch 串行 + runOnFinish 恢复初始值防污染。
    GameTest.register("CommandTests", "worldborder_set_size", worldborderSetSize)
        .structureName("gametests:cmd_arena")
        .batch("worldborder_set_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "worldborder_add_size", worldborderAddSize)
        .structureName("gametests:cmd_arena")
        .batch("worldborder_add_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "worldborder_set_center", worldborderSetCenter)
        .structureName("gametests:cmd_arena")
        .batch("worldborder_center_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "worldborder_damage_amount", worldborderDamageAmount)
        .structureName("gametests:cmd_arena")
        .batch("worldborder_dmgamt_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "worldborder_damage_buffer", worldborderDamageBuffer)
        .structureName("gametests:cmd_arena")
        .batch("worldborder_dmgbuf_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "worldborder_warning_time", worldborderWarningTime)
        .structureName("gametests:cmd_arena")
        .batch("worldborder_wtime_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "worldborder_warning_distance", worldborderWarningDistance)
        .structureName("gametests:cmd_arena")
        .batch("worldborder_wdist_solo")
        .maxTicks(60);
}
