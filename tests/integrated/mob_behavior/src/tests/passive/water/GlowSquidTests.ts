// 发光鱿鱼行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 发光鱿鱼受攻击后暗化（停止发光）并喷射荧光墨汁粒子（wiki mob_发光鱿鱼.txt#行为：发光鱿鱼会
// 在被攻击后的 100 内停止发光，并且喷射出一种荧光墨汁粒子）。
//
// C++ 链路：GlowSquidEntity : SquidEntity : WaterMobEntity : CreatureEntity。GlowSquidEntity::hurt
// （GlowSquidEntity.cpp:114-122）先调 SquidEntity::hurt，成功（返回 true）后 setDarkTicks(100)。
// SquidEntity::hurt（SquidEntity.cpp:125-133）：WaterMobEntity::hurt（即 LivingEntity::hurt）成功
// 且 getLastHurtBy()!=nullptr 时 sprayInk + 返回 true。LivingEntity::hurt 扣血后第 8 步调
// setLastHurtBy(attacker)（LivingEntity.cpp:344-349），attacker = playerAttack 的 trueSource（Player*），
// 故玩家攻击后 getLastHurtBy() 非空，SquidEntity::hurt 返回 true，触发 setDarkTicks(100)。
// setDarkTicks（GlowSquidEntity.cpp:93-97）写 DataParameter DATA_DARK_TICKS_REMAINING_PARAM + 镜像成员，
// GlowSquidEntity::tick（:124-146）每 tick darkTicks>0 时递减 1，100 tick 后归 0 恢复发光。
// DARK_TICKS_ON_HURT=100（GlowSquidEntity.hpp:171）对齐 wiki"被攻击后的 100 内停止发光"。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block + y=1..4 air）。陆地空气层，发光鱿鱼 spawn
// 即在陆地（无水）。受攻击暗化与是否在水中无关（hurt 链不检查水），陆地可测。结构放置 +1 抬升：
// helper-y=N → 结构内 y=N-1。发光鱿鱼 (3,2,3)（脚踩结构内 y=0 grass_block），Survival 玩家 (4,2,3)。
//
// 判定手段：读 minecraft:glow_squid_dark_ticks 组件（Cubium 自定义组件绑定，MinecraftModuleFactory.cpp
// getComponent 对 GlowSquidEntity 查 getDarkTicksRemaining()，>0 返 GlowSquidDarkTicksComponent
// 否则 undefined）。受击前未暗化（组件 undefined），受击后暗化（组件非 undefined，value=100 递减）。
// 用 spawn 返回的 glowSquid 引用读组件（暗化是实体自身状态，不依赖坐标查询，引用稳定——同
// creeper_charged_by_lightning 用 creeper 引用读 is_charged 组件模式）。attackEntity 不受距离限制
// （基岩语义），runAtTickTime(8) 留 8 tick 让实体完成 spawn 注册 + 首 tick 稳定后再攻击。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验）——创造/旁观玩家攻击不造成伤害
// （被 LivingEntity::hurt 滤掉），hurt 返回 false 不触发 setDarkTicks。必须 Survival。
// maxTicks=200：attack 在 tick 8，暗化立即=100 递减，tick 108 归 0；succeedWhen 在 attack 后立即满足，
// 200 余量充足吸收并行负载时序偏移。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mob_发光鱿鱼.txt#行为（被攻击后停止发光）
function glowSquidDarkensWhenHurt(test: Test): void {
  const glowSquidType = "glow_squid";

  // 发光鱿鱼 (3,2,3)、Survival 玩家 (4,2,3)，紧邻 1 格。helper-y=2 → 结构内 y=1 空气，脚踩 y=0 grass_block。
  // 玩家用 Survival（gameMode=0）：创造/旁观攻击不造成伤害，无法触发 hurt→setDarkTicks。必须 Survival。
  const glowSquid = test.spawn(glowSquidType, { x: 3, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 8 后玩家攻击发光鱿鱼：留 8 tick 让实体完成 spawn 注册 + 首 tick 稳定。
  // attackEntity 不受距离限制（基岩语义），紧邻更自然。攻击走 Player::attack → DamageSources::playerAttack
  // → GlowSquidEntity::hurt → SquidEntity::hurt → LivingEntity::hurt 扣血+setLastHurtBy → 返回 true
  // → setDarkTicks(100)。空手攻击伤害 1（Java 玩家空手伤害），发光鱿鱼 10 血不会死。
  test.runAtTickTime(8, () => {
    player.attackEntity(glowSquid);
  });

  // 断言发光鱿鱼受击后暗化：succeedWhen 每 tick 检查 glow_squid_dark_ticks 组件非 undefined。
  // 受击前组件 undefined（未暗化），受击后 setDarkTicks(100) 使组件存在（value=100 递减）。
  // 用 spawn 返回引用读组件（暗化是实体自身状态，引用稳定，不依赖坐标查询）。
  test.succeedWhen(() => {
    const dark = glowSquid.getComponent("minecraft:glow_squid_dark_ticks" as any);
    test.assert(dark !== undefined,
      "glow squid did not darken when hurt (glow_squid_dark_ticks component missing)");
  });
}

export function registerGlowSquidTests(): void {
  GameTest.register("MobBehaviorTests", "glow_squid_darkens_when_hurt", glowSquidDarkensWhenHurt)
    .structureName("gametests:creeper_pit")
    .maxTicks(200);
}
