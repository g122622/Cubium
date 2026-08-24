// 风爆（Wind Burst）附魔自弹行为类 GameTest。
//
// 验证 Cubium 重锤风爆附魔运行时触发链路（Player::attack → _applyWindBurstEffect）正确接入，
// 对齐 MC Java 1.21.11 ExplodeEffect（ExplodeEffect.java:62-84）+ ServerExplosion.hurtEntities
// （ServerExplosion.java:175-199）。
//
// 风爆机制（对齐 vanilla Enchantments.java:1216-1252 wind_burst 注册数据）：
//   POST_ATTACK 效果 = ExplodeEffect(
//     attributeToUser=false,          // 爆炸源为 null（不归属攻击者）
//     damageType=empty,               // 无伤害（TRIGGER 不破坏方块也不造伤害）
//     knockbackMultiplier=[1.2,1.75,2.2],  // 每级击退乘数
//     offset=Vec3.ZERO,               // 爆炸中心 = 攻击者位置
//     radius=3.5,                     // 爆炸半径
//     blockInteraction=TRIGGER)       // 不破坏方块
//   触发条件：攻击者 fallDistance>=1.5 且非滑翔（LootItemEntityPropertyCondition moving.fallDistance>=1.5）。
//
// 连跳原理（vanilla ServerExplosion.hurtEntities:175）：
//   level.getEntities(this.source=null, box) —— source=null 不过滤任何实体，玩家自身被包含在
//   击退列表中。对玩家自身用 getEyePosition（position.y + 1.62）算方向：
//     vec31 = eyePosition.subtract(center).normalize() = (0,1.62,0).normalize() = (0,1,0)
//   玩家被沿 (0,1,0) 即向上弹起，落地后再触发下一次砸地——实现重锤连跳。
//
// 修复（任务 #312）：Cubium Player::_applyWindBurstEffect 此前有两处偏差：
//   1. getEntitiesInAABB(searchBox, this)：except=this 把玩家自身排除出击退列表，
//      致风爆无法弹起攻击者自身，重锤连跳完全失效。改为 nullptr 对齐 vanilla source=null。
//   2. 方向计算用 entity->position()：玩家自身 position 与 burstPos 重合，差为零向量走"随机方向"
//      分支，连跳方向随机（多向下/水平），无法稳定向上。改为 eyePosition（对齐 vanilla
//      ServerExplosion.java:178 PrimedTnt?position:eyePosition）。
//
// 验证手段（任务 #312 同步补全脚本 API）：
//   基岩官方 @minecraft/server Entity.getVelocity(): Vector3 是标准 API，Cubium 此前未绑定
//   （脚本系统缺陷），致风爆弹起等依赖速度断言的行为无法测试。本次补全 getVelocity 绑定
//   （MinecraftModuleFactory.cpp Entity 类），使风爆弹起可经玩家 Y 速度>0 验证。
//
// 测试设计（fall_tower 7×16×7，中心 (3,*,3) 1×1 玻璃管）：
//   - attacker 持 Wind Burst III 重锤，spawn 管顶 y=12 自由下落。
//   - victim（villager）站管内 y=6（setBlockType y=5 支撑面）。
//   - 轮询 attacker Y 位置：下落到 y<10.5（fallDistance>1.5，canSmash=true）且仍在空中（y>7）
//     时，发起【单次】attackEntity(victim)。attackEntity 不检查距离/碰撞（直接调 Player::attack），
//     故 attacker 在 y≈10 攻击 y=6 的 victim 仍能命中。单次攻击避免连续攻击污染 victim 无敌帧
//     （此前设计每 2 tick 攻击致首次非下落攻击命中施 20 tick 无敌帧，后续 canSmash=true 的下落
//     攻击全被无敌帧吞掉 hurt 返回 false，风爆块在 if(attacked) 内被跳过——纯测试设计缺陷）。
//   - 风爆触发后玩家获向上 Y 速度（addVelocity(0,+impact,0)）。
//   - 后续轮询 attacker velocity.y >0.1 即 succeed。
//   - fall_tower 管底 y=0 stone，attacker 不会摔死（风爆弹起后 applyPostImpulseGraceTime +
//     ignoreFallDamageFromCurrentImpulse 免疫冲量坠落伤害）。
//
// 防假通过设计（正反对照）：
//   - wind_burst_launches_attacker_upward：Wind Burst III 重锤下落攻击，玩家 Y 速度 >0.1（向上）。
//     修复前（except=this）：玩家被排除，Y 速度为下落负值 ∉ >0.1 → 超时 FAIL。
//     修复前（position 随机方向）：即使纳入玩家，方向随机，Y 可能向下 ∉ >0.1 → 超时 FAIL。
//   - no_wind_burst_no_upward_velocity：无附魔重锤下落攻击，玩家 Y 速度 ≤0（继续下落，无风爆）。
//     交叉验证：有风爆向上 vs 无风爆向下/零 = 风爆自弹机制正确。
//
// 独立 batch（wind_burst_solo）：避免并行污染（外来 player/villager 污染区域查询）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: Player.cpp:1931（getEntitiesInAABB nullptr 对齐 vanilla source=null）
// Ref: Player.cpp:1964-1968（eyePosition 方向对齐 vanilla ServerExplosion.java:178）
// Ref: Player.cpp:2033（addVelocity 向上弹起玩家自身）
// Ref: MinecraftModuleFactory.cpp（Entity.getVelocity 绑定，补全脚本 API 缺陷）
// Ref: ExplodeEffect.java:62-84（attributeToUser=false → source=null）
// Ref: ServerExplosion.java:175-199（getEntities(null) + eyePosition + push 玩家）
// Ref: Enchantments.java:1216-1252（wind_burst POST_ATTACK ExplodeEffect 注册）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。
// 中心 (3,*,3) 为 1×1 垂直玻璃管落管：y=0 满铺 cobblestone，y=1..14 中心柱 air，y=15 封顶。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

// attacker spawn 管顶 y=12（管内 air 通道 y=1..14），自由下落。
const ATTACKER_SPAWN_POS = { x: 3, y: 12, z: 3 };
// victim 站立位置 y=6（setBlockType 在 y=5 放方块作支撑面，villager 脚 y=6）。
const VICTIM_POS = { x: 3, y: 6, z: 3 };
const VICTIM_SUPPORT_POS = { x: 3, y: 5, z: 3 };

// 单次下落攻击触发窗口（相对 Y 坐标，经 test.worldLocation 转世界坐标后比较）：
//   - ATTACK_TRIGGER_Y_HIGH=10.5：attacker 下落到此高度以下时已下落 >1.5 格（12-10.5=1.5），
//     fallDistance>1.5，canSmashAttack=true，风爆触发条件满足。
//   - ATTACK_TRIGGER_Y_LOW=7.5：attacker 仍在 victim（y=6）上方，未接近落地（管底 y=0 仍远），
//     保证攻击发生在空中下落中而非落地后（落地后 fallDistance 清零 canSmash=false）。
//   - 窗口 (7.5, 10.5) 覆盖约 3 格下落（约 3-4 tick），轮询间隔 1 tick 必能命中。
//   注意：Entity.location 返回世界绝对坐标（如 gridStartY=-59 时相对 y=12 对应世界 y=-47），
//   故阈值须经 test.worldLocation 转世界坐标后与 location.y 比较，不可直接用相对值。
const ATTACK_TRIGGER_Y_HIGH_REL = 10.5;
const ATTACK_TRIGGER_Y_LOW_REL = 7.5;

// 读取落地区域内玩家的 Entity 包装（非 SimulatedPlayer JS 类）。
// getEntities 取玩家 Entity 包装，可调 location（position）/getVelocity()。
function findTowerPlayer(test: Test): any | null {
    const players = test.getDimension().getEntities({
        type: "minecraft:player",
        location: test.worldLocation(TOWER_FROM),
        volume: TOWER_VOLUME,
    });
    if (players.length === 0) {
        return null;
    }
    return players[0];
}

// 读取落地区域内 villager 的当前血量。
function readVillagerHp(test: Test): number {
    const villagers = test.getDimension().getEntities({
        type: "villager",
        location: test.worldLocation(TOWER_FROM),
        volume: TOWER_VOLUME,
    });
    if (villagers.length === 0) {
        return -1;
    }
    const health = villagers[0].getComponent("minecraft:health");
    return (health as any).currentValue as number;
}

// 构造物品 ItemStack（两份 @minecraft/server 类型分裂，as any 绕过，同 ArmorDamageReductionTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给攻击者主手装备 Wind Burst III 重锤。
function equipWindBurstMace(player: any, level: number): void {
    const mace = makeItem("minecraft:mace");
    if (level > 0) {
        (mace as any).addEnchantment({ type: "minecraft:wind_burst", level });
    }
    player.setItem(mace, 0, true);
}

// Wind Burst III 重锤下落攻击触发风爆，玩家被自身风爆向上弹起（核心修复验证）。
//
// attacker 持 Wind Burst III 重锤，spawn y=12 自由下落。victim 站 y=6。
// 轮询 attacker Y 位置：下落到 (7.5, 10.5) 窗口（fallDistance>1.5，canSmash=true，仍在空中）时，
// 用 hasAttacked flag 保证【单次】attackEntity(victim)。命中即触发风爆，玩家被 addVelocity(0,+,0) 弹起。
// 后续轮询玩家 Y 速度 >0.1 即 succeed。
//
// 单次攻击的必要性：连续每 tick 攻击会致首次（canSmash=false）攻击命中 victim 施 20 tick 无敌帧，
// 后续 canSmash=true 的下落攻击全被无敌帧吞（hurt 返回 false），风爆块在 if(attacked) 内被跳过，
// 风爆永不触发——这是纯测试设计缺陷，非 C++ 缺陷。单次攻击保证首次命中即 canSmash=true。
//
// 判定：玩家 Y 速度 >0.1。
//   - 修复前 except=this：玩家被排除出击退列表，Y 速度为下落负值 ∉ >0.1 → 超时 FAIL。
//   - 修复前 position 随机方向：Y 可能向下 ∉ >0.1 → 超时 FAIL。
//   - 修复后 eyePosition+nullptr：Y=(0,1,0)*impact 向上，Y 速度 >0.1 → PASS。
function windBurstLaunchesAttackerUpward(test: Test): void {
    (test as any).killAllEntities();
    // victim 支撑面：y=5 放方块，villager 站 y=6（脚踩 y=5 顶面 y=6.0）。
    test.setBlockType("minecraft:stone", VICTIM_SUPPORT_POS);
    const victim = test.spawn("minecraft:villager", VICTIM_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_SPAWN_POS, "attacker", 0 as any); // 0=Survival

    equipWindBurstMace(attacker, 3); // Wind Burst III

    // 单次攻击控制：hasAttacked flag 保证整个测试只攻击一次，避免连续攻击污染无敌帧。
    // 闭包捕获，runAtTickTime 回调内读写。
    let hasAttacked = false;

    // 攻击触发窗口的世界 Y 阈值（相对阈值经 worldLocation 转换）。
    // Entity.location 是世界绝对坐标，须与世界阈值比较。
    const yHighWorld = (test.worldLocation({ x: 3, y: ATTACK_TRIGGER_Y_HIGH_REL, z: 3 })).y;
    const yLowWorld = (test.worldLocation({ x: 3, y: ATTACK_TRIGGER_Y_LOW_REL, z: 3 })).y;

    // 阶段一：轮询 attacker Y 位置，进入下落攻击窗口时发起单次攻击。
    // tick 6 起每 1 tick 检查（下落窗口约 3-4 tick，1 tick 间隔必命中）。
    // 攻击触发后 velocity.y 立即变正（Player::attack 同步调 _applyWindBurstEffect → addVelocity）。
    for (let tick = 6; tick <= 70; tick += 1) {
        test.runAtTickTime(tick, () => {
            if (hasAttacked) {
                return;
            }
            const player = findTowerPlayer(test);
            if (player === null) {
                return;
            }
            const y = (player.location as any).y as number;
            // 下落攻击窗口：fallDistance>1.5（y<yHighWorld）且仍在空中（y>yLowWorld，未接近落地）。
            if (y < yHighWorld && y > yLowWorld) {
                hasAttacked = true;
                (attacker as any).attackEntity(victim);
            }
        });
    }

    // 阶段二：轮询玩家 Y 速度 >0.1（风爆弹起）。
    // 时序（PHY_DIAG 实测）：攻击在 t≈5 触发风爆，玩家 vel.y 在 t=6（≈1.59）、t=7（≈1.48）为正，
    // t=8 归零（到顶点）。故检查点须从 t=6 起、interval=1 才能抓到 vel.y>0 的 2-tick 窗口。
    // startTick=10/interval=2 会错过窗口（t=10 时玩家已下落 vel.y<0）→ 误判风爆失效。
    // 同时要求 victim HP<20（攻击已命中），双重确认风爆触发链路完整。
    pollUntilSucceed(test, () => {
        const hp = readVillagerHp(test);
        if (hp < 0 || hp >= 20) {
            return false; // 攻击未命中，继续等
        }
        const player = findTowerPlayer(test);
        if (player === null) {
            return false;
        }
        const vel = (player as any).getVelocity() as { x: number; y: number; z: number };
        if (vel === undefined || vel === null) {
            return false;
        }
        return vel.y > 0.1;
    }, {
        startTick: 6,
        interval: 1,
        maxTick: 40,
        onTimeout: () => test.assert(false,
            `Wind Burst III mace smash attack should launch attacker upward (Y velocity >0.1), `
            + `but victim HP=${readVillagerHp(test)}, player Y velocity=${
                (() => {
                    const p = findTowerPlayer(test);
                    if (p === null) return "no player";
                    const v = (p as any).getVelocity();
                    return v ? v.y : "no velocity";
                })()
            }, hasAttacked=${hasAttacked} `
            + `(if HP>=20 attack not landed yet; if HP<20 but Y<=0.1 wind burst not launching attacker `
            + `[task #312 regression: except=this excludes player or position random direction])`),
    });
}

// 无附魔重锤下落攻击不触发风爆，玩家继续下落（Y 速度 ≤0，反向对照防假通过）。
//
// attacker 持无附魔重锤（Wind Burst level=0），下落攻击 victim 命中，但无风爆触发，
// 玩家 Y 速度 ≤0（继续下落或砸地停止）。
//
// 判定：victim HP<20（攻击命中）且 玩家 Y 速度 ≤0.1（无向上弹起）。
//   与 wind_burst_launches_attacker_upward 交叉验证：有风爆 Y>0.1 vs 无风爆 Y≤0.1。
//   若无附魔也 Y>0.1（如砸地反弹），两测试无法区分 → 需重审测试设计。
function noWindBurstNoUpwardVelocity(test: Test): void {
    (test as any).killAllEntities();
    test.setBlockType("minecraft:stone", VICTIM_SUPPORT_POS);
    const victim = test.spawn("minecraft:villager", VICTIM_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_SPAWN_POS, "attacker", 0 as any);

    equipWindBurstMace(attacker, 0); // 无附魔重锤

    let hasAttacked = false;

    const yHighWorld = (test.worldLocation({ x: 3, y: ATTACK_TRIGGER_Y_HIGH_REL, z: 3 })).y;
    const yLowWorld = (test.worldLocation({ x: 3, y: ATTACK_TRIGGER_Y_LOW_REL, z: 3 })).y;

    for (let tick = 6; tick <= 70; tick += 1) {
        test.runAtTickTime(tick, () => {
            if (hasAttacked) {
                return;
            }
            const player = findTowerPlayer(test);
            if (player === null) {
                return;
            }
            const y = (player.location as any).y as number;
            if (y < yHighWorld && y > yLowWorld) {
                hasAttacked = true;
                (attacker as any).attackEntity(victim);
            }
        });
    }

    // 轮询：victim HP<20（攻击命中）且 玩家 Y 速度 ≤0.1（无向上弹起）。
    // 时序对齐正向测试（startTick=6, interval=1）：无附魔时风爆不触发，玩家持续下落 vel.y<0，
    // 落地后 vel.y=0，均满足 ≤0.1。与正向 vel.y>0.1 交叉验证风爆自弹机制。
    pollUntilSucceed(test, () => {
        const hp = readVillagerHp(test);
        if (hp < 0 || hp >= 20) {
            return false;
        }
        const player = findTowerPlayer(test);
        if (player === null) {
            return false;
        }
        const vel = (player as any).getVelocity() as { x: number; y: number; z: number };
        if (vel === undefined || vel === null) {
            return false;
        }
        return vel.y <= 0.1;
    }, {
        startTick: 6,
        interval: 1,
        maxTick: 40,
        onTimeout: () => test.assert(false,
            `no-enchantment mace smash attack should NOT launch attacker (Y velocity <=0.1), `
            + `but victim HP=${readVillagerHp(test)}, player Y velocity=${
                (() => {
                    const p = findTowerPlayer(test);
                    if (p === null) return "no player";
                    const v = (p as any).getVelocity();
                    return v ? v.y : "no velocity";
                })()
            }, hasAttacked=${hasAttacked} `
            + `(if HP>=20 attack not landed; if HP<20 but Y>0.1 unexpected upward without wind burst)`),
    });
}

export function registerWindBurstEnchantmentTests(): void {
    GameTest.register("MobBehaviorTests", "wind_burst_launches_attacker_upward",
        windBurstLaunchesAttackerUpward)
        .batch("wind_burst_solo")
        .structureName("gametests:fall_tower")
        .maxTicks(120);

    GameTest.register("MobBehaviorTests", "no_wind_burst_no_upward_velocity",
        noWindBurstNoUpwardVelocity)
        .batch("wind_burst_solo")
        .structureName("gametests:fall_tower")
        .maxTicks(120);
}
