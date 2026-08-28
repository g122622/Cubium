// 中毒效果"不能致死"边界行为类 GameTest。
//
// 验证 Cubium 中毒效果（Poison）的"不能致死"特性：中毒每 25>>amplifier tick 扣 1 HP，
// 但 health>1.0 才扣（EffectInstance.cpp:290 守卫 `if (entity.health() > 1.0f)`），故中毒
// 只能把生物 HP 降到 1（半颗心），不会致死。对齐 MC Java 1.21.11 MobEffect Poison。
//
// 现有 EffectImmunityTests.nonUndeadAffectedByPoison 仅验证"中毒扣血"（HP<20），未覆盖
// "不能致死"边界。本测试专项验证 HP 降到 1 停止、生物存活不死亡——若 health>1 守卫失效
// （中毒能致死），villager 会被毒死消失或 HP=0，本测试 FAIL。
//
// 中毒数值（amplifier=0，对齐 wiki other_中毒.txt + EffectInstance.cpp:290）：
//   - interval = 25 >> 0 = 25 tick（每 25 tick 扣 1）
//   - 每次扣 1 HP，health>1.0 才扣
//   - villager HP 20 → 扣 19 次到 HP=1 停止（19*25=475 tick 内完成）
//
// 判定（tick 550，duration=600 留足扣到边界的时间）：
//   - villager 仍存活（未中毒致死消失）。
//   - villager HP == 1（中毒扣到 1 停止，不致死）。
//   若 health>1 守卫失效（中毒能致死）：villager HP 降到 0 死亡消失，或 HP<1 → FAIL。
//   若中毒链路失效（不扣血）：villager HP=20 → FAIL（HP!=1）。
//
// 防假通过设计：
//   - killAllEntities 清场 + 不 spawn 玩家：避免 night 自然刷怪追杀 villager 致 HP 变化干扰。
//   - duration=600（30秒）远超扣到边界所需 475 tick，确保 t=550 时已扣到 HP=1。
//   - villager 被动不反击/不移动，中毒是唯一 HP 变化源。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_中毒.txt（中毒不能致死，降到 1 停止）
// Ref: src\common\entity\effect\EffectInstance.cpp:290（Poison case，health>1.0 守卫）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 读取实体当前血量（HP）。返回 -1 表示组件未就绪或实体已消失。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 中毒不能致死：villager 长时间中毒后 HP 降到 1 停止，存活不死亡（验证 health>1 守卫）。
//
// villager HP 20，tick 5 施加中毒（amplifier=0，duration=600）。每 25 tick 扣 1，扣 19 次到 HP=1
// 停止（475 tick 内完成）。tick 550 断言 villager 存活 + HP==1。
//
// killAllEntities 清场 + night batch：villager 非亡灵不燃，但 night 自然刷怪可能追杀 villager，
// killAllEntities 清当前结构活体避免干扰。不 spawn 玩家避免 villager 受击/逃跑。
function poisonCannotKill(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn("villager", { x: 3, y: 2, z: 3 });

    // tick 5 施加中毒 amplifier=0 duration=600（30秒，远超扣到 HP=1 所需 475 tick）。
    test.runAtTickTime(5, () => {
        (villager as any).addEffect("poison", 600, { amplifier: 0 });
    });

    // tick 550 断言：villager 存活 + HP==1（中毒扣到 1 停止，未致死）。
    // 475 tick 扣到 HP=1，550 tick 留足缓冲确认"停在 1 不继续扣"。
    test.runAtTickTime(550, () => {
        const hp = readHp(villager);
        // villager 仍存活（未中毒致死）：HP 可读且 >0。
        test.assert(hp > 0,
            `poison should not kill villager (health>1 guard), but villager HP=${hp} `
            + `(if HP<=0 or -1, villager died from poison — health>1 guard broken)`);
        // HP 恰好降到 1（中毒扣到边界停止，不致死也不残留更高 HP）。
        // 容忍 ±0.5 浮点误差（HP 组件 currentValue 可能是浮点）。
        test.assert(hp >= 0.5 && hp <= 1.5,
            `poison should reduce villager HP to exactly 1 (cannot kill), hp=${hp} `
            + `(if hp==20 poison not applied; if hp<0.5 poison killed — guard broken; `
            + `if hp in (1.5,20) poison stopped early or duration too short)`);
        test.succeed();
    });
}

export function registerPoisonCannotKillTests(): void {
    GameTest.register("MobBehaviorTests", "poison_cannot_kill_reduces_to_one_hp", poisonCannotKill)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(620);
}
