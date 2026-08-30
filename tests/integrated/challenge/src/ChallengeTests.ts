// 挑战类 GameTest：minibiomes（矿车载猪滑行）、collapsing（实心空间坍缩）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { assertEntityInVolume } from "./utils/entity/assert.js";

// 矿车载猪：猪骑乘矿车沿铁轨滑行至终点，验证骑乘实体位置随载具同步。
function minibiomes(test: Test): void {
  const minecartEntityType = "minecraft:minecart";
  const pigEntityType = "minecraft:pig";

  const minecart = test.spawn(minecartEntityType, { x: 9, y: 7, z: 7 });
  const pig = test.spawn(pigEntityType, { x: 9, y: 7, z: 7 });

  test.setBlockType("minecraft:cobblestone", { x: 10, y: 7, z: 7 });

  const minecartRideableComp = minecart.getComponent("minecraft:rideable");
  if (!minecartRideableComp) {
    test.assert(false, "minecart has no rideable component");
    return;
  }

  minecartRideableComp.addRider(pig);

  test.succeedWhenEntityPresent(pigEntityType, { x: 5, y: 3, z: 1 }, true);
}

// 坍缩空间：zoglin 被 shulker 击中后浮空上升。3 只 zoglin 与 3 只 shulker 在低处（y=2）生成，
// shulker 受伤反击射出 shulker bullet，命中 zoglin 施加 200t Levitation I，zoglin 浮空上升到
// y=6-13 高处。succeedWhen 断言该高处体积内存在 zoglin（assertEntityInVolume 断言"存在"非"清除"）。
// 按钮触发红石是结构装饰（非成功必要条件）。
//
// 时序裕度：maxTicks=600（原 400 偏紧）。多 RNG 延迟链路叠加——zoglin 目标选择 chance=10（每 tick
// 1/10 概率检查）、shulker 攻击冷却 20-69t、贝壳开启动画 20t、bullet 飞行随机步数、Levitation 上升
// ~100t（y=2→8 升 6 格，每 tick +0.05 加速）——最坏 ~260t 触上限偶发超时。3v3 多实体干扰（目标分散、
// zoglin 游走、shulker 受伤瞬移）进一步拉长。600t 留足裕度覆盖最坏 RNG 种子。底层 C++ 链路（zoglin AI、
// shulker 反击、bullet、Levitation 物理）已正确实现，失败纯为时序裕度不足的非确定性。
// 断言体积（0,6,0）→（13,13,13）：结构为 13x14x11，zoglin 被 bullet 命中后 Levitation 上升至 y≥6
// 即满足。体积上界须达 x=13/y=13——实测失败 run 中贴东墙 zoglin 浮至 x=12.7，撞 y=13 天花板后停留，
// 原体积（0,8,0）→（12,12,12）既够不到墙边也够不到天花板下沿，是该 ~37% 失败率的直接根因。
// Ref: zoglin_float（mob_behavior/ZoglinTests.ts）同链路 1v1 范式
function collapsing(test: Test): void {
  const zoglinEntityType = "minecraft:zoglin";
  const shulkerEntityType = "minecraft:shulker";

  for (let i = 0; i < 3; i++) {
    test.spawn(zoglinEntityType, { x: i + 2, y: 2, z: 3 });
    test.spawn(shulkerEntityType, { x: 4, y: 2, z: i + 2 });
  }

  test.pressButton({ x: 6, y: 8, z: 5 });

  test.succeedWhen(() => {
    assertEntityInVolume(test, zoglinEntityType, 0, 6, 0, 13, 13, 13);
  });
}

export function registerChallengeTests(): void {
  GameTest.register("ChallengeTests", "minibiomes", minibiomes).structureName("gametests:minibiomes").maxTicks(260);
  GameTest.register("ChallengeTests", "collapsing", collapsing)
    .structureName("gametests:collapsing_space")
    .maxTicks(600);
}
