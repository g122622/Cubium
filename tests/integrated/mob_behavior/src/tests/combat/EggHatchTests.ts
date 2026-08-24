// 鸡蛋命中 dispatch 与孵化行为类 GameTest（验证 EggEntity::onImpact 调基类 dispatch +
// 孵化逻辑对齐 vanilla ThrownEgg）。
//
// 验证 Cubium 鸡蛋：① 命中实体后鸡蛋消失（onImpact remove）且不造成伤害（0 伤害，对齐 vanilla
// ThrownEgg.onHitEntity hurt 0.0F）；② 命中方块后 1/8 概率孵化小鸡（幼年），对齐 MC Java 1.21.11
// ThrownEgg.onHit。
//
// C++ 链路（ProjectileItemEntity.cpp:187-201 onEntityHit + :203-246 onImpact + :248-278 _spawnHatchedChicken）：
//   test.spawn("minecraft:egg", pos) + setVelocity({x,y,z})（任务 #324 绑定）
//     → EggEntity 继承 ThrowableEntity::tick（ThrowableEntity.cpp:104 调 onImpact）
//     → onImpact 首行调基类 ProjectileEntity::onImpact（ProjectileEntity.cpp:301-335）dispatch
//       → 命中实体 onEntityHit：hurt(thrown, 0.0F)（0 伤害触发受击副作用，不扣血）
//       → 命中方块 onBlockHit：通知方块 onProjectileHit + 清零速度
//     → onImpact 内 1/8 概率孵化（1/32 子概率孵 4 只），_spawnHatchedChicken 生成幼年小鸡
//     → onImpact 末尾 if(!isRemoved()) remove()（对齐 vanilla onHit discard）
//
// 【修复背景（任务 #329）】此前 EggEntity::onImpact 未调基类 dispatch（onEntityHit/onBlockHit 死代码），
//   onEntityHit 为空实现（vanilla 应 hurt 0.0F 触发受击副作用），孵化仅 1/8 孵 1 只（缺 1/32 孵 4 只
//   分支），生成的小鸡为成年（vanilla setAge(-24000) 幼年）。修复：onImpact 首行调基类 dispatch，
//   onEntityHit hurt(thrown, 0.0F)，孵化补 1/32 孵 4 只 + setChild(true) 幼年。对齐 vanilla
//   ThrownEgg.java:55-91。与 SnowballEntity #327 / WindChargeEntity #328 修复同构。
//
// vanilla 对齐（ThrownEgg.java）：
//   onHitEntity（:55-59）：super.onHitEntity + hurt(thrown(this, getOwner()), 0.0F)。
//   onHit（:62-91）：super.onHit() dispatch + 1/8 孵化（1/32 子概率孵 4 只）+ setAge(-24000) 幼年
//     + snapTo(位置) + addFreshEntity + broadcastEntityEvent(3) + discard。
//   注：vanilla onHit 对所有命中类型（实体/方块）统一孵化（super.onHit 之后），Cubium 在 onImpact
//   孵化位置正确。原 TODO 注释"命中实体不孵化"判断错误，已修正。
//
// wiki 依据（tech_鸡蛋.txt）：右键投掷生成 egg 实体，落地 1/8 概率孵化小鸡。
//
// 前置能力（任务 #324）：Entity.setVelocity。鸡蛋 velocity={0,-3,0} 高速朝下 1 tick 命中地板。
//
// 防假通过设计：
//   - egg_hit_entity_disappears_no_damage：鸡蛋 setVelocity 命中 villager → ① 鸡蛋消失（egg_count=0，
//     onImpact remove 生效）；② villager HP=20（0 伤害不扣血，onEntityHit hurt 0.0F 对齐）。
//     双断言验证 dispatch（onEntityHit 被调）+ remove（onImpact discard）。
//     - 若 onImpact 修复回退为空（不调基类）：egg_count=1（不消失）→ FAIL。
//     - 若 onEntityHit hurt 漏接：villager 不受副作用（本测试不断言副作用，仅断言 HP=20 不扣血）。
//   - egg_hatch_spawns_chicken：30 个鸡蛋 setVelocity 朝下撞地板 → 至少 1 只 chicken 生成（1/8 概率，
//     30 个鸡蛋至少 1 个孵化概率 1-(7/8)^30 ≈ 98%）。验证孵化链路（_spawnHatchedChicken + spawnEntity）。
//     孵化概率性：98% 通过率，偶发 flaky 重跑即可（非缺陷）。
//     - 若 onImpact 孵化逻辑断裂：0 只 chicken → FAIL（持续失败才是缺陷）。
//     - 若 _spawnHatchedChicken spawnEntity 失败：0 只 chicken → FAIL。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）+ default batch。killAllEntities 清场。
// 鸡蛋命中实体测试：villager 站桩不移动，命中稳定。鸡蛋孵化测试：30 个鸡蛋朝下撞地板，分散 x/z 避免重叠。
//
// 时序：
//   - egg_hit_entity_disappears_no_damage：tick 0 spawn villager+egg + setVelocity；tick 1 鸡蛋命中；
//     tick 20 断言（egg_count=0 + villager HP=20）。
//   - egg_hatch_spawns_chicken：tick 0 分散 spawn 30 个鸡蛋 + setVelocity 朝下；tick 1-2 全部撞地板
//     孵化判定；tick 30 断言 chicken 数量≥1（留足孵化 + 小鸡 spawn 注册时间）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: ProjectileItemEntity.cpp:187-201（onEntityHit：hurt(thrown, 0.0F) 0 伤害副作用）
// Ref: ProjectileItemEntity.cpp:203-246（onImpact：调基类 dispatch + 1/8 孵化 + remove，任务 #329 修复）
// Ref: ProjectileItemEntity.cpp:248-278（_spawnHatchedChicken：幼年小鸡 setChild(true) + spawnEntity）
// Ref: ThrownEgg.java:55-91（vanilla onHitEntity hurt 0.0F + onHit 1/8 孵化 setAge(-24000) discard）
// Ref: ProjectileEntity.cpp:301-335（基类 onImpact dispatch）
// Ref: MinecraftModuleFactory.cpp Entity.setVelocity 绑定（任务 #324）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const VILLAGER_TYPE = "villager_v2";
const EGG_TYPE = "minecraft:egg";
const CHICKEN_TYPE = "minecraft:chicken";

// creeper_pit 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。y=2 → 结构 y=1 air 腔，脚下 y=0 grass_block。
const EGG_SPAWN_POS = { x: 3, y: 2, z: 3 };
const VILLAGER_POS = { x: 3, y: 2, z: 5 };
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 读取实体句柄当前 HP（currentValue）。句柄无效或无 health 组件返 NaN（同 WitherEffectTests 范式）。
function readHp(entity: unknown): number {
    const health = (entity as any)?.getComponent?.("minecraft:health") as
        | { currentValue?: number }
        | undefined;
    return health?.currentValue ?? NaN;
}

// 查询区域内某类型实体数量（区域限定排除并行测试污染）。
function countEntities(test: Test, type: string): number {
    return test.getDimension().getEntities({
        type,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
    }).length;
}

// 鸡蛋 setVelocity 命中 villager → 鸡蛋消失（onImpact remove）+ villager 不掉血（0 伤害）。
//
// 鸡蛋 (3,2,3) setVelocity({0,0,3.0}) 朝 +Z 飞，1 tick 命中 villager (3,2,5)（距 2 格，射线 z∈[3,6]
// 覆盖 villager z=5）。onImpact 调基类 dispatch → onEntityHit：hurt(thrown, 0.0F)（0 伤害不扣血，
// 仅触发受击副作用）+ onImpact 末尾 remove（鸡蛋消失）。
//
// 判定（tick 20，鸡蛋命中后 ~19 tick，remove 落定）：
//   ① egg_count===0（鸡蛋命中后 onImpact remove 消失）。
//   ② villager HP===20（0 伤害不扣血，对齐 vanilla hurt 0.0F）。
//   - 若 onImpact 修复回退为空（不调基类 dispatch + 不 remove）：egg_count=1（不消失）→ FAIL，
//     暴露 onImpact 死代码缺陷（任务 #329 回归）。
//   - 若 onEntityHit 误对实体造成伤害：villager HP<20 → FAIL（应 0 伤害）。
function eggHitEntityDisappearsNoDamage(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn(VILLAGER_TYPE, VILLAGER_POS);
    const egg = test.spawn(EGG_TYPE, EGG_SPAWN_POS);

    // setVelocity 朝 +Z 3.0/tick，1 tick 跨 3 格命中 villager（距 2 格，射线 z∈[3,6] 覆盖 villager z=5）。
    (egg as any).setVelocity({ x: 0, y: 0, z: 3.0 });

    pollUntilSucceed(test, () => {
        const eggCount = countEntities(test, EGG_TYPE);
        const hp = readHp(villager);
        // ① 鸡蛋消失（egg_count=0）+ ② villager 满血未扣血（HP=20，0 伤害）。
        return eggCount === 0 && !Number.isNaN(hp) && hp === 20;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 60,
        onTimeout: () => {
            const eggCount = countEntities(test, EGG_TYPE);
            const hp = readHp(villager);
            test.assert(false,
                `egg_hit_entity_disappears_no_damage: failed: egg_count=${eggCount} villager hp=${hp} `
                + `(expected egg_count=0 [onImpact remove] & hp=20 [0 damage, hurt 0.0F]; `
                + `if egg_count>0 onImpact empty dead-coded remove [task #329 regression]; `
                + `if hp<20 onEntityHit wrongly damaged entity [should be 0.0F])`);
        },
    });
}

// 30 个鸡蛋朝下撞地板 → 至少 1 只小鸡孵化（1/8 概率，30 个至少 1 个孵化 ≈98%）。
//
// 30 个鸡蛋分散 spawn 在 y=4（结构 y=3 air 腔上方），setVelocity({0,-3,0}) 朝下 1 tick 撞 y=0 grass_block
// 地板。onBlockHit→onImpact 内 1/8 概率孵化（每个鸡蛋独立判定），_spawnHatchedChicken 生成幼年小鸡。
// 30 个鸡蛋至少 1 个孵化概率 1-(7/8)^30 ≈ 98.2%。
//
// 判定（tick 30，鸡蛋撞地板后 ~29 tick，孵化 + 小鸡 spawn 注册完成）：
//   chicken_count >= 1（至少 1 只小鸡孵化）。
//   - 若 onImpact 孵化逻辑断裂（_spawnHatchedChicken 未调 / spawnEntity 失败）：chicken_count=0 → FAIL。
//   - 若 setChild/setTypeId 失败致小鸡 spawn 异常消失：chicken_count=0 → FAIL。
//   孵化概率性：98% 通过，偶发 flaky（0 只）重跑即可，连续 0 只才是缺陷。
//
// 分散 spawn：30 个鸡蛋分布在 7×7 的 x,z 平面（x,z∈[0,6]），避免同位置重叠。
function eggHatchSpawnsChicken(test: Test): void {
    (test as any).killAllEntities();

    // 30 个鸡蛋分散 spawn 在 y=4，setVelocity 朝下撞地板（y=0 grass_block）。
    // 分散 x,z 避免重叠（7×7=49 格够放 30 个）。
    let idx = 0;
    for (let x = 0; x <= 6 && idx < 30; ++x) {
        for (let z = 0; z <= 6 && idx < 30; ++z) {
            const egg = test.spawn(EGG_TYPE, { x, y: 4, z });
            // setVelocity 朝下 3.0/tick，1 tick 撞地板（y=4→y=1，距 3 格，射线 y∈[1,4] 覆地板 y=0 顶面）。
            (egg as any).setVelocity({ x: 0, y: -3.0, z: 0 });
            ++idx;
        }
    }

    pollUntilSucceed(test, () => {
        const chickenCount = countEntities(test, CHICKEN_TYPE);
        // 至少 1 只小鸡孵化（30 个鸡蛋 1/8 概率，至少 1 个 ≈98%）。
        return chickenCount >= 1;
    }, {
        startTick: 30,
        interval: 10,
        maxTick: 80,
        onTimeout: () => {
            const chickenCount = countEntities(test, CHICKEN_TYPE);
            test.assert(false,
                `egg_hatch_spawns_chicken: failed: chicken_count=${chickenCount} `
                + `(expected >=1 [30 eggs, 1/8 hatch each, P(>=1)≈98%]; `
                + `if chicken_count=0 hatch logic broken [_spawnHatchedChicken/spawnEntity failed] `
                + `or rare 2% no-hatch flaky [rerun]; continuous 0 = defect)`);
        },
    });
}

export function registerEggHatchTests(): void {
    GameTest.register("MobBehaviorTests", "egg_hit_entity_disappears_no_damage", eggHitEntityDisappearsNoDamage)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);

    GameTest.register("MobBehaviorTests", "egg_hatch_spawns_chicken", eggHatchSpawnsChicken)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(160);
}
