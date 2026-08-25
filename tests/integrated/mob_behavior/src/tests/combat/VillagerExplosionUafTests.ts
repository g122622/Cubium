// 村民爆炸伤害 UAF 回归测试类 GameTest。
//
// 验证任务 #272 修复：苦力怕爆炸伤害村民后，村民 Brain 的 HurtBySensor::update 取攻击者时不
// 解引用已析构苦力怕的悬垂指针（UAF 段错误）。
//
// 缺陷背景（修复前）：
//   苦力怕爆炸伤害村民后，村民 LivingEntity::m_lastDamageSource（clone 自爆炸 EntityDamageSource）
//   持苦力怕裸指针（getTrueSource 返回 m_source=苦力怕）。苦力怕 explode() 末尾 remove()
//   （m_removed=true），后续 EntityManager graveyard 延迟 1 tick 析构后，村民 Brain 的
//   HurtBySensor::update（Sensors.cpp:192）经 lastDamageSource->getTrueSource() 拿到悬垂苦力怕
//   指针，attacker->isAlive() 解引用已释放内存 UAF 段错误（崩溃栈 HurtBySensor::update ←
//   Brain::tick ← _tickBrains）。所有「爆炸伤害带 Brain 的 mob」（村民等）真实游戏场景崩溃。
//
// 修复方案（任务 #272，对齐 vanilla HurtBySensor + getLastDamageSource 40 tick 过期）：
//   1. LivingEntity::actuallyHurt 设置 m_lastDamageSource 时同步捕获 m_lastDamageSourceTrueId
//      （同步上下文真凶必活，安全取 id）+ m_lastDamageStamp（时间戳）。
//   2. LivingEntity::lastDamageSource() 加 40 tick 过期守卫（对齐 vanilla getLastDamageSource:
//      1391-1397，超 40 tick 置空 m_lastDamageSource），缩小悬垂窗口。
//   3. 新增 LivingEntity::lastDamageSourceTrueId() 返回捕获的 id（同受 40 tick 过期约束）。
//   4. HurtBySensor::update 改用 lastDamageSourceTrueId() 经 IWorld::getEntity(id) 安全校验取
//      attacker，绕开 lastDamageSource->getTrueSource() 的悬垂裸指针。id 永不悬垂
//      （EntityInstanceId 单调递增不复用，析构后 getEntity 返 nullptr）。
//
// 本测试验证修复后行为：
//   - 村民被苦力怕爆炸伤害后存活（HP > 0，钻石套减伤保证不死）。
//   - 爆炸后等待 >60 tick（苦力怕完全析构 + HurtBySensor 多次 update 触及 UAF 窗口），服务端不崩。
//   - 修复前：UAF 段错误崩整个 GameTest server 进程，测试无法 succeed（进程异常退出）。
//   - 修复后：村民存活 + 测试 succeed。
//
// 受害者选择：villager（非 cow）。cow 无 Brain 不触发 HurtBySensor（BlastProtectionKnockbackTests
//   用 cow 正是为规避本 UAF，见该文件 :38-41 注释）。本测试专门用 villager 触发 UAF 路径作回归。
//   villager HP 20，穿钻石套（armor 20）紧贴苦力怕（距 1 格）承受 radius 3 爆炸（seenPercent≈1，
//   基础伤害约 33，钻石套减伤后存活）。村民有 AvoidHostileGoal 会逃离苦力怕，须用玻璃围栏封闭村民
//   防逃离（详见 layPlatformAndEnclosure 注释）。
//
// 确定性设计：用打火石 interactWithEntity 点燃苦力怕（同 BlastProtectionKnockbackTests 范式），
//   不依赖苦力怕 AI 自发 swell（NearestAttackableTargetGoal 选目标时机非确定）。
//
// 时序：
//   - tick 0：spawn villager（穿钻石套）+ creeper + 玩家（持打火石）+ 铺黑曜石平台 + 玻璃围栏封闭村民。
//   - tick 30：玩家 interactWithEntity(creeper) 点燃（留 30 tick 让装备同步管线应用钻石套）。
//   - 约 tick 60：苦力怕爆炸，伤害村民（玻璃围栏爆炸瞬间碎，村民仍在原位距苦力怕 1 格受满伤害）。
//   - tick 62-130：pollUntilSucceed 轮询村民存活（HP > 0）。此窗口跨越苦力怕析构
//     （tick ~62 爆炸→remove→tick ~63 入 graveyard→tick ~64 析构）+ 后续 HurtBySensor 多次
//     update（每 tick 调），充分覆盖 UAF 触发窗口。修复前任意一次 update 解引用悬垂指针即崩。
//
// 防假通过：断言村民 HP > 0 且 HP < 满血（20），证明村民确实被爆炸伤害了（非"爆炸没生效"假通过）。
//   若爆炸未伤害村民（HP 仍 20，如村民逃离范围 / 玻璃围栏失效）：HP < 20 断言 FAIL。
//   若 UAF 崩溃：进程退出码 3221225477（0xC0000005），pollUntilSucceed 永不 succeed，超时 FAIL。
//
// night batch + killAllEntities：村民非亡灵不燃烧，但 night 隔离白天自然刷怪干扰；killAllEntities
//   清场防自然刷怪污染区域查询。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// 实际采用 villager_uaf_solo 独立 batch（见 register 注释）替代 night，确保 UAF 崩溃可观察且不
//   拖累其他测试。
// Ref: 任务 #272（爆炸伤害村民 Brain HurtBySensor UAF 既有缺陷）
// Ref: Sensors.cpp:178-218（HurtBySensor::update，修复后用 lastDamageSourceTrueId 经 world 校验）
// Ref: LivingEntity.cpp actuallyHurt（捕获 m_lastDamageSourceTrueId + m_lastDamageStamp）
// Ref: LivingEntity.cpp lastDamageSource/lastDamageSourceTrueId（40 tick 过期守卫）
// Ref: BlastProtectionKnockbackTests.ts:38-41（用 cow 规避本 UAF 的既有注释，本测试用 villager 回归）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const VILLAGER_TYPE = "minecraft:villager";
const CREEPER_TYPE = "minecraft:creeper";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 几何布局（creeper_pit，helper 相对坐标）：
//   苦力怕 (3,2,3) 中心；村民 (4,2,3) 紧贴苦力怕东侧距 1 格（爆炸半径 3 内，seenPercent≈1）；
//   玩家 (3,2,6) 南侧远端持打火石点燃苦力怕。
//
// 村民逃离问题（关键，HP=20 假通过根因）：
//   村民有 AvoidHostileGoal（VillagerEntity::registerGoals priority 1，最高），点燃苦力怕后 fuse
//   30 tick 内村民会主动逃离苦力怕，跑出 2 格距离躲出爆炸有效伤害范围 → HP 不掉 → HP=20 假通过。
//   cow 无此 goal（BlastProtectionKnockbackTests 用 cow 距苦力怕 2 格能受伤）。故本测试须约束村民
//   无法逃离：用玻璃围栏封闭村民所在 1×1×1 空间（除顶部留空让爆炸伤害可达）。
//   玻璃爆炸抗性 1.5 会被苦力怕 radius 3 炸碎，但仅爆炸瞬间碎——fuse 30 tick 期间玻璃完整挡住村民
//   推墙逃离（实体推不动方块），爆炸瞬间玻璃碎、村民仍在原位距苦力怕 1 格受满 seenPercent 伤害。
//   （同 BlastProtectionKnockbackTests:63-65 玻璃围栏约束苦力怕范式，本测试围村民。）
const CREEPER_POS = { x: 3, y: 2, z: 3 };
const VILLAGER_POS = { x: 4, y: 2, z: 3 };
const PLAYER_POS = { x: 3, y: 2, z: 6 };

// 村民满血（对齐 vanilla Villager MAX_HEALTH=20）。
const VILLAGER_MAX_HP = 20;

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 BlastProtectionKnockbackTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给村民穿普通钻石套（armor 20 + EPF 减伤，保证被苦力怕 radius 3 爆炸伤害后存活可测 HP）。
function equipDiamondFull(villager: any): void {
    const eq = villager.getComponent("minecraft:equippable");
    eq.setEquipment("Head", makeItem("minecraft:diamond_helmet"));
    eq.setEquipment("Chest", makeItem("minecraft:diamond_chestplate"));
    eq.setEquipment("Legs", makeItem("minecraft:diamond_leggings"));
    eq.setEquipment("Feet", makeItem("minecraft:diamond_boots"));
}

// 铺设基底 + 围栏约束村民防逃离。
//   黑曜石平台（y=1，z=3 整条 x∈[0,6]）：爆炸抗性 1200 不被苦力怕 radius 3 破坏，防爆炸炸毁地板
//     致村民掉落。y=1 是 creeper_pit air 层（y=0 为 grass_block 地板），黑曜石放 y=1 作平台，实体站 y=2。
//   玻璃围栏封闭村民 (4,2,3)：村民四面（x=5 东、z=2 北、z=4 南）+ 苦力怕侧（x=3 已被苦力怕占据不堵）
//     站位层 y=2 放玻璃，堵住村民所有可逃离方向。村民被锁 1×1×1（顶部 y=3 留空不堵，让爆炸伤害可达
//     且不额外降 seenPercent）。爆炸瞬间玻璃碎，村民仍在原位受满伤害。
function layPlatformAndEnclosure(test: Test): void {
    // 黑曜石平台（防掉落）。
    for (let x = 0; x <= 6; x++) {
        test.setBlockType("minecraft:obsidian", { x, y: 1, z: 3 });
    }
    // 玻璃围栏封闭村民 (4,2,3) 防逃离（AvoidHostileGoal）。
    test.setBlockType("minecraft:glass", { x: 5, y: 2, z: 3 }); // 东侧
    test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 2 }); // 北侧
    test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 4 }); // 南侧
    // 西侧 x=3 是苦力怕站位，不堵（村民不会朝苦力怕逃离）。
}

// 读取区域内村民当前血量（HP）。实体句柄由闭包持有，规避区域查询污染。
// 实体死亡/移除后 getComponent("health") 返回 undefined → 返回 -1。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 苦力怕爆炸伤害村民后村民存活（任务 #272 UAF 回归）。
//
// 村民（HP 20，穿钻石套）距苦力怕 2 格，打火石点燃苦力怕爆炸。爆炸伤害村民（HP 20→<20 存活）。
// 爆炸后等待 >60 tick 跨越苦力怕析构 + HurtBySensor 多次 update（UAF 触发窗口），断言村民存活
// 且 HP<满血（证明被伤害）。
//
// 修复前：苦力怕析构后村民 HurtBySensor::update 解引用悬垂苦力怕指针 UAF 段错误，崩 GameTest
//   server 进程，pollUntilSucceed 永不 succeed。
// 修复后：HurtBySensor 经 lastDamageSourceTrueId + world->getEntity(id) 校验，苦力怕析构后返回
//   nullptr，不写 HURT_BY_ENTITY，不解引用悬垂指针，村民存活，测试 succeed。
function villagerSurvivesCreeperExplosionNoUaf(test: Test): void {
    (test as any).killAllEntities();
    layPlatformAndEnclosure(test);

    const villager = test.spawn(VILLAGER_TYPE, VILLAGER_POS);
    equipDiamondFull(villager);
    const creeper = test.spawn(CREEPER_TYPE, CREEPER_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "igniter", 0 as any);

    // 创造玩家主手持打火石。
    const flintAndSteel = new ItemStack("minecraft:flint_and_steel", 1);
    (player as any).setItem(flintAndSteel as any, 0, true);

    // tick 30 点燃苦力怕（留 30 tick 让 detectEquipmentUpdates 应用钻石套减伤）。
    test.runAtTickTime(30, () => {
        (player as any).interactWithEntity(creeper);
    });

    // 爆炸约 tick 60。pollUntilSucceed 轮询村民存活（HP > 0）且 HP < 满血（被伤害）。
    // startTick=62：爆炸后 2 tick 开始查（伤害当 tick 完成）。
    // maxTick=130：跨越苦力怕析构（tick ~64）+ 后续 60+ tick HurtBySensor update，充分覆盖 UAF 窗口。
    //   修复前任意一次 update 解引用悬垂指针即崩进程 → 超时 FAIL。
    //   修复后村民 HP 稳定存活 → 早期 succeed。
    pollUntilSucceed(test, () => {
        const hp = readHp(villager);
        // HP > 0（存活，未崩）且 HP < 满血（被爆炸伤害，防"爆炸没生效"假通过）。
        return hp > 0 && hp < VILLAGER_MAX_HP;
    }, {
        startTick: 62,
        interval: 2,
        maxTick: 130,
        onTimeout: () => {
            const hp = readHp(villager);
            test.assert(false,
                `villager_survives_creeper_explosion_no_uaf: failed: villager HP=${hp} `
                + `(expected 0<HP<${VILLAGER_MAX_HP}; if HP=-1 villager died/removed or server crashed [UAF task #272]; `
                + `if HP=${VILLAGER_MAX_HP} explosion did not damage villager; if process crashed see UAF in HurtBySensor)`);
        },
    });
}

export function registerVillagerExplosionUafTests(): void {
    // 独立 batch（villager_uaf_solo）：避免 night batch 并行污染（外来 villager/creeper 串台干扰
    //   HP 读取与 UAF 观察窗口），同 Breach breach_solo 范式。UAF 崩溃是进程级事件，串行独占 batch
    //   确保崩溃可观察且不拖累其他测试。
    // maxTicks 200：留点燃 + 爆炸 + UAF 窗口轮询 + 余量。
    GameTest.register("MobBehaviorTests", "villager_survives_creeper_explosion_no_uaf", villagerSurvivesCreeperExplosionNoUaf)
        .batch("villager_uaf_solo")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);
}
