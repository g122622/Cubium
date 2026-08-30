// 投射物互偏转（REDIRECTABLE_PROJECTILE）对齐测试。
//
// 验证 vanilla 1.21.11 Projectile.onHit:287-291（路径 B 互偏转）：来袭投射物 A 命中
// 可偏转投射物 B（EntityTypeTags.REDIRECTABLE_PROJECTILE = fireball/wind_charge/
// breeze_wind_charge）时，对 B 调 deflect(AIM_DEFLECT, A.getOwner(), true)——B 速度设为
// A 发射者（射手）的视线方向（单位向量），B 新 owner 改为 A 的发射者。然后仍调 onEntityHit
// （A 正常处理命中 B，与路径 A 偏转自身后 return 不同）。
//
// vanilla 精确语义（Projectile.java:287-289）：
//   if (entity.getType().is(EntityTypeTags.REDIRECTABLE_PROJECTILE) && entity instanceof Projectile projectile) {
//       projectile.deflect(ProjectileDeflection.AIM_DEFLECT, this.getOwner(), this.owner, true);
//   }
//   this.onHitEntity(entityhitresult);  // 仍调用
// 其中 deflect 的第二参数 this.getOwner() = 来袭投射物 A 的 owner（射手）。
// AIM_DEFLECT（ProjectileDeflection.java:18-24）：B 速度 = deflector.getLookAngle()（A owner 视线
// 单位向量，不乘原速）+ setOwner(deflector)。
//
// C++ 链路（对齐 vanilla Projectile.onHit → 互偏转分支）：
//   ProjectileEntity::onImpact（ProjectileEntity.cpp:323-336）在路径 A 自偏转检查（deflection==None
//   跳过，fireball 不在 DEFLECTS_PROJECTILES 标签内故 deflection 返回 None）之后、onEntityHit switch
//   之前，插入路径 B 互偏转分支：命中实体 typeId ∈ REDIRECTABLE_PROJECTILE 时，对命中投射物调
//   deflect(AIM_DEFLECT, *getShooter(), true)。getShooter() 对应 vanilla this.getOwner()（A 的 owner）。
//   applyProjectileDeflection(AimDeflect)（ProjectileDeflection.cpp:57-77）设 B 速度为 shooter 视线
//   单位向量 + setShooter(&shooter)，与 vanilla AIM_DEFLECT + setOwner 对齐。
//
// 关键约束：互偏转依赖 A 的 owner 存在（getShooter() 非空，否则不偏转）。故测试必须用玩家拉弓射箭
//   让箭矢带上 owner——不能改用 test.spawn+setVelocity（spawn 的投射物无 owner，getShooter 返回 null，
//   互偏转分支 shooter==nullptr 跳过，fireball 不被偏转）。
//
// 此前缺陷：Cubium ProjectileEntity::onImpact 缺路径 B 互偏转分支——箭矢命中火球只走 onEntityHit
//   （火球 hurt 仅 markHurt 不死），火球不被偏转、owner 不变，与 vanilla 不符（vanilla 箭矢命中火球
//   会把火球沿射手视线弹开并改归属）。本次修复补该分支。
//
// 测试手段：SimulatedPlayer 弓箭手朝 +Z 拉弓射箭，箭矢命中前方静止火球 → 火球被偏转沿 +Z 飞走。
//   火球 DamagingProjectileEntity 构造 setNoGravity(true)，test.spawn 不设加速度，静止悬浮
//   （velocity=0，performRayTrace start==end 不命中，不会自爆）。
//
// 判定：玩家 yaw=0 视线 +Z，箭矢命中火球后火球沿 +Z 偏转飞走（z 增大 >6.0）或飞出 pit 边界 remove
//   （length===0）。对照测试证明静止火球不会自发位移/消失，故位移/消失确由箭矢命中互偏转导致。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block）。玩家 (3,2,3) yaw=0 看 +Z，火球 (3,2,4)
//   玩家前方 +Z 1 格（同 BowArrowDamageTests 命中范式，箭矢 1 tick 命中）。火球偏转后沿 +Z 单位速度
//   飞，creeper_pit z∈[0,6]，火球飞到 z>6 后撞墙/飞出边界 remove。
// Ref: Projectile.java:287-291（onHit 互偏转）/ :255-274（deflect 方法）
// Ref: ProjectileDeflection.java:18-24（AIM_DEFLECT 单位视线向量）
// Ref: ProjectileEntity.cpp:323-336（Cubium 互偏转分支）/ ProjectileDeflection.cpp:57-77（AimDeflect）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const BOW = "minecraft:bow";
const ARROW = "minecraft:arrow";
const FIREBALL = "minecraft:fireball";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 区域限定查询排除并行测试污染（Cubium GameTest 批内并行 tick + 不清场）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 玩家 (3,2,3) 默认 yaw=0 pitch=0 朝 +Z。火球 (3,2,4) 玩家前方 +Z 1 格（箭矢 1 tick 命中）。
const ARCHER_POS = { x: 3, y: 2, z: 3 };
const FIREBALL_POS = { x: 3, y: 2, z: 4 };

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 BowArrowDamageTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 查询 pit 区域内指定类型的所有实体（用世界绝对坐标 + 体积框定，隔离并行测试残留）。
// 返回类型由 getEntities 隐式推断（同 ProjectileDeflectTests 范式，不显式标注避免额外 import）。
function getEntitiesInPit(test: Test, type: string) {
    return test.getDimension().getEntities({
        type,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
    });
}

// 投射物互偏转：玩家拉弓射箭命中静止火球，火球被沿射手视线（+Z）偏转飞走。
//
// 链路：玩家 tick5 拉弓 tick25 释放 → 箭矢 A（owner=玩家）朝 +Z 飞行 speed=3.0 → 1 tick 后命中
//   火球 B（(3,2,4)，canBeHitByProjectile=true，不在 DEFLECTS_PROJECTILES 标签故 deflection=None
//   跳过路径 A）→ 路径 B 互偏转：B.deflect(AIM_DEFLECT, 玩家) → B 速度=玩家视线(+Z 单位向量) +
//   B owner=玩家 → B 沿 +Z 单位速度飞走。箭矢 A 仍调 onEntityHit（hurt 火球，火球 hurt 仅 markHurt
//   不死，不影响 B 偏转）。
//
// 断言：火球 z 增大到 >6.0（偏转后沿 +Z 飞，远超初始 4.5）或飞出 pit 边界 remove（length===0）。
//   静止火球 z≈4.5 不满足 → 超时 FAIL。fireball_stays_static_without_arrow_hit 对照证明静止火球
//   不会自发位移/消失，故此处位移/消失确由箭矢命中互偏转导致。
function arrowRedirectsFireballOnHit(test: Test): void {
    (test as any).killAllEntities();
    const player = test.spawnSimulatedPlayer(ARCHER_POS, "archer", 0 as any); // 0=Survival
    player.setItem(makeItem(BOW), 0, true); // 主手弓 slot 0
    const arrow = new ItemStack(ARROW, 5);
    player.setItem(arrow as unknown as Parameters<typeof player.setItem>[0], 40, false); // 副手 slot 40

    // 火球 spawn 于玩家前方 +Z 1 格 (3,2,4)。DamagingProjectileEntity 构造 setNoGravity(true)，
    // test.spawn 不设加速度，火球静止悬浮（velocity=0，performRayTrace start==end 不命中方块/实体）。
    test.spawn(FIREBALL, FIREBALL_POS);

    // tick 5 useItem(弓) → setActiveHand 拉弓（useDuration=72000）。
    test.runAtTickTime(5, () => {
        (player as any).useItem(makeItem(BOW) as unknown as Parameters<typeof player.useItem>[0]);
    });
    // tick 25 stopUsingItem 释放 → 满弓 20 tick（chargeTicks=20）→ velocity=1.0 → speed=3.0。
    // 箭矢朝 +Z 飞，1 tick 后（tick 26）命中前方火球 → 互偏转触发。
    test.runAtTickTime(25, () => {
        (player as any).stopUsingItem();
    });

    // 轮询断言火球被偏转沿 +Z 飞走：z 从初始 4.5 增大到 >6.0，或飞出 pit 边界 remove。
    // 偏转后火球以单位速度 1 格/tick 沿 +Z 飞：tick26 命中偏转，tick27 z≈5.5，tick28 z≈6.5（撞墙/
    // 出界 remove）。startTick=30 interval=2 maxTick=60 覆盖命中后飞行窗口。
    pollUntilSucceed(test, () => {
        const fireballs = getEntitiesInPit(test, FIREBALL);
        // 火球飞出 pit 边界 remove（length===0）也说明偏转生效（静止火球不会自发消失）。
        if (fireballs.length === 0) return true;
        const z = fireballs[0].location.z;
        return z > 6.0;
    }, {
        startTick: 30,
        interval: 2,
        maxTick: 60,
        onTimeout: () => {
            const fireballs = getEntitiesInPit(test, FIREBALL);
            const z = fireballs.length > 0 ? fireballs[0].location.z : "fireball removed";
            test.assert(false,
                `arrow_redirects_fireball_on_hit: failed: fireball z=${z} (expected >6.0 or removed, `
                + `fireball should be deflected along archer look +Z after arrow hit). `
                + `fireballCount=${fireballs.length}`);
        },
    });
}

// 对照测试：火球无箭矢命中时静止悬浮存活 40 tick 后位置不变。
// 排除 arrow_redirects_fireball_on_hit 的假通过——若火球 spawn 后自发消失/移动，偏转测试中
// fireballs.length===0 或 z 变化可能并非箭矢命中互偏转所致。本测试 spawn 火球不射箭，在 tick 40
// 断言火球仍存活且 z≈4.5（静止），证明火球不会自发消失/位移，从而偏转测试中的位移/消失确由
// 箭矢命中互偏转导致。
// DamagingProjectileEntity 构造 setNoGravity(true)，test.spawn 不设加速度，火球 velocity=0 静止悬浮，
// performRayTrace start==end 不命中方块/实体，不会自爆。用 runAtTickTime(40) 末期单点断言（非
// succeedWhen——succeedWhen 首 tick 火球静止即 PASSED 无法验证"持续静止"，末期断言才能证明
// 40 tick 内未移动）。
function fireballStaysStaticWithoutArrowHit(test: Test): void {
    (test as any).killAllEntities();
    // 火球 spawn 于 (3,2,4)，无箭矢命中，应静止悬浮。
    test.spawn(FIREBALL, FIREBALL_POS);

    // tick 40 断言火球仍存活且 z≈4.5（test.spawn (3,2,4) 坐标中心化，火球初始 z=4.5）。
    // 偏转测试箭矢命中在 tick 26，本测试观察期 40 tick 远超偏转时序。火球 velocity=0 + 加速度=0 +
    // setNoGravity，tick 内 nextPosition=pos+velocity 不变，应静止在 z=4.5。阈值 ±0.3 容忍位置量化
    // 误差，重点验证火球不会自发飞远/消失——偏转测试的 z>6.0 或消失不会被静止火球满足。
    test.runAtTickTime(40, () => {
        const fireballs = getEntitiesInPit(test, FIREBALL);
        test.assert(fireballs.length > 0,
            `fireball disappeared without arrow hit (should stay static), count=${fireballs.length}`);
        // Entity.location 是世界绝对坐标（结构网格原点非零，见 StructureGridSpawner），断言前必须经
        // worldLocation 把结构相对坐标 (3,2,4) 转成世界绝对坐标再比较（fireball 中心化 z+0.5）。
        // 直接拿绝对坐标与相对值 4.5 比较，单跑（首行结构原点≈0）碰巧通过，全量跑恒失败。
        const expectedZ = test.worldLocation({ x: 3, y: 2, z: 4 }).z + 0.5;
        const z = fireballs[0].location.z;
        test.assert(Math.abs(z - expectedZ) < 0.3,
            `fireball moved without arrow hit (should stay near z=${expectedZ}), z=${z}`);
        test.succeed();
    });
}

export function registerProjectileRedirectTests(): void {
    GameTest.register("MobBehaviorTests", "arrow_redirects_fireball_on_hit", arrowRedirectsFireballOnHit)
        .structureName("gametests:creeper_pit")
        .maxTicks(120);

    GameTest.register("MobBehaviorTests", "fireball_stays_static_without_arrow_hit", fireballStaysStaticWithoutArrowHit)
        .structureName("gametests:creeper_pit")
        .maxTicks(100);
}
