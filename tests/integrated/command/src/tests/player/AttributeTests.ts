// /attribute 命令 GameTest：查询/设置实体属性基础值。
//
// 覆盖 wiki 命令章节核心行为：
//   - /attribute <target> <attribute> base set <值>：设置属性基础值（Ref: wiki commands/attribute.txt）
//   - /attribute <target> <attribute> base reset：重置为默认值
//
// 设计要点：
//   1. AttributeCommand 经 LivingEntity::attributes().getInstance(attrName)->setBaseValue(value)
//      修改属性基础值（非占位）。属性名经 AttributeRegistry::isKnown 校验，自动补 "generic." 前缀
//      （输入 "max_health"/"movement_speed"/"generic.max_health" 均可）。@s 选择器经 source.entity()
//      返回 SimulatedPlayer 自身（Player : LivingEntity，是 LivingEntity 子类，属性命令适用）。
//   2. 断言用已绑定的脚本组件读属性值（零新绑定）：
//      - max_health base set → getComponent("minecraft:health").effectiveMax（living->maxHealth() 读 MAX_HEALTH 属性）
//      - movement_speed base set → getComponent("minecraft:movement").currentValue（attributes().getValue(MOVEMENT_SPEED)）
//      health/movement 组件对 SimulatedPlayer 已重绑（ScriptSimulatedPlayer.cpp getComponent，
//      见 [[simulated-player-js-class-no-entity-inheritance]]，SimulatedPlayer JS 类独立注册无 Entity 原型）。
//   3. 属性是世界级实体状态，但 @s 操作自身玩家无跨测试污染（每测试独立 SimulatedPlayer）。
//      仍用独占 batch 串行（同 difficulty/weather 范式）避免同批并行干扰，runOnFinish base reset 恢复默认。
//   4. SimulatedPlayer 默认属性：max_health=20（Player），movement_speed=0.1（Java Player GENERIC_MOVEMENT_SPEED）。
//      base set 后断言新值，base reset 后断言回默认。
//   5. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// 注：浮点属性值断言用容差比较（base set 写入 f64，组件读出 f64，但属性内部可能 float 存储，用 |a-b|<1e-4）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_attribute.txt（属性命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// 玩家默认属性值（对齐 Java Player）。
const DEFAULT_MAX_HEALTH = 20;
const DEFAULT_MOVEMENT_SPEED = 0.1;

// 浮点近似相等断言（属性内部 float 存储，避免精度漂移误判）。
function assertApprox(test: Test, actual: number, expected: number, label: string): void {
    test.assert(
        Math.abs(actual - expected) < 1e-4,
        `${label}: expected ~${expected}, got ${actual}`,
    );
}

// /attribute @s generic.max_health base set 40 设置玩家最大生命基础值为 40，
// 断言 health.effectiveMax≈40。
// runOnFinish base reset 恢复 20。
// Ref: wiki commands/attribute.txt（base set）
function attributeMaxHealthBaseSet(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const health = player.getComponent("minecraft:health") as any;

    test.assert(health !== undefined, "player has no health component");
    assertApprox(test, health.effectiveMax, DEFAULT_MAX_HEALTH, "default max_health");

    player.chat("/attribute @s generic.max_health base set 40");

    assertApprox(test, health.effectiveMax, 40, "after base set 40");

    test.runOnFinish(() => {
        player.chat("/attribute @s generic.max_health base reset");
    });
    test.succeed();
}

// /attribute @s generic.movement_speed base set 0.5 设置玩家移动速度基础值为 0.5，
// 断言 movement.currentValue≈0.5。
// runOnFinish base reset 恢复 0.1。
// Ref: wiki commands/attribute.txt（base set movement_speed）
function attributeMovementSpeedBaseSet(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const movement = player.getComponent("minecraft:movement") as any;

    test.assert(movement !== undefined, "player has no movement component");
    assertApprox(test, movement.currentValue, DEFAULT_MOVEMENT_SPEED, "default movement_speed");

    player.chat("/attribute @s generic.movement_speed base set 0.5");

    assertApprox(test, movement.currentValue, 0.5, "after base set 0.5");

    test.runOnFinish(() => {
        player.chat("/attribute @s generic.movement_speed base reset");
    });
    test.succeed();
}

// base reset 重置为默认值：先 base set 40，再 base reset，断言 effectiveMax 回 20。
// 验证 resetBaseValue 恢复注册表默认值（非仅清零）。
// Ref: wiki commands/attribute.txt（base reset）
function attributeBaseResetRestoresDefault(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const health = player.getComponent("minecraft:health") as any;

    player.chat("/attribute @s generic.max_health base set 40");
    assertApprox(test, health.effectiveMax, 40, "after base set 40");

    player.chat("/attribute @s generic.max_health base reset");
    assertApprox(test, health.effectiveMax, DEFAULT_MAX_HEALTH, "after base reset");

    // 已恢复默认，无需 runOnFinish。
    test.succeed();
}

// 短名属性自动补 generic. 前缀：/attribute @s max_health base set 40（不带 generic. 前缀）
// 同样生效，断言 effectiveMax≈40。验证 AttributeRegistry 自动前缀补全。
// Ref: wiki commands/attribute.txt（属性名可省略 generic. 前缀）
function attributeShortNameAutoPrefix(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const health = player.getComponent("minecraft:health") as any;

    // 短名 "max_health"（不带 generic. 前缀），AttributeRegistry 自动补全为 "generic.max_health"。
    player.chat("/attribute @s max_health base set 35");

    assertApprox(test, health.effectiveMax, 35, "short name max_health base set 35");

    test.runOnFinish(() => {
        player.chat("/attribute @s max_health base reset");
    });
    test.succeed();
}

export function registerAttributeTests(): void {
    // 属性是实体状态，@s 操作自身玩家无跨测试污染，仍独占 batch 串行避免同批并行干扰。
    GameTest.register("CommandTests", "attribute_max_health_base_set", attributeMaxHealthBaseSet)
        .structureName("gametests:cmd_arena")
        .batch("attribute_health_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "attribute_movement_speed_base_set", attributeMovementSpeedBaseSet)
        .structureName("gametests:cmd_arena")
        .batch("attribute_movement_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "attribute_base_reset_restores_default", attributeBaseResetRestoresDefault)
        .structureName("gametests:cmd_arena")
        .batch("attribute_reset_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "attribute_short_name_auto_prefix", attributeShortNameAutoPrefix)
        .structureName("gametests:cmd_arena")
        .batch("attribute_shortname_solo")
        .maxTicks(60);
}
