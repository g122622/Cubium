// 命名牌命名生物消耗对齐测试（验证 itemInteractionForEntity 权威手持数量消耗，对应同款拷贝缺陷修复）。
//
// 验证 NameTagItem::itemInteractionForEntity 的命名牌消耗链路（wiki tech_命名牌.txt：命名牌须在铁砧命名后，
// 右键生物消耗1个命名牌并给生物命名）。
//
// 此前 NameTagItem::itemInteractionForEntity 用 stack 参数（Player::interactOn 传入的 getHeldItem 值拷贝
// Player.cpp:2858）做 shrink，shrink 作用于拷贝不回写权威物品栏——Survival 玩家持多个命名牌命名生物时
// 不消耗（仅持1个时靠 Player 路径拷贝 isEmpty 回写空槽侥幸消耗）。已改用 player.getHeldItem(hand) 权威
// 手持 shrink 修复（同 GoldenAppleItem/SaddleItem 修复范式）。本测试持已命名命名牌数量2，断言命名后主手
// 数量变1（消耗1个），覆盖"持多个不消耗"bug。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。牛 (3,2,3)，Survival 玩家 (1,2,3) 持命名牌数量2。
// 牛是被动生物不攻击玩家，环境干净。
//
// 命名牌须有自定义名（NameTagItem.cpp:44 hasCustomName 检查）：用 ItemStack 构造选项 nameTag 设名"TestCow"。
// 脚本侧无法直接读生物 nameTag 验证命名成功（Entity.nameTag 属性绑定情况未确认），故以"主手命名牌数量变1"
// 作为命名成功的直接证据（itemInteractionForEntity 返 true 才走消耗，返 false 不消耗）。
//
// 判定手段：主手槽（slot 0）命名牌数量变1（消耗1个）。Survival 模式（创造跳过消耗无证据）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const NAME_TAG = "minecraft:name_tag";

function getMainHandAmount(player: any): number {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.amount ?? 0;
}

// 命名牌命名牛消耗1个：Survival 玩家持已命名命名牌数量2，断言命名后主手数量1。
function nameTagCowConsumptionTest(test: Test): void {
  // 牛 (3,2,3)（creeper_pit y=0 grass_block 地板）。
  const cow = test.spawn("minecraft:cow", { x: 3, y: 2, z: 3 });

  // Survival 玩家 (1,2,3) 持已命名命名牌数量2（slot 0 主手，nameTag="TestCow" 使 hasCustomName=true）。
  // 持2个验证"持多个也消耗1"。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "namer", 0 as any);
  const nameTags = new ItemStack(NAME_TAG, 2);
  (nameTags as any).nameTag = "TestCow";
  player.setItem(nameTags as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持命名牌 interactWithEntity(cow) → NameTagItem::itemInteractionForEntity
  // → setCustomName + enablePersistence + 权威手持 shrink(1)（数量2→1）。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(cow);
  });

  // 轮询断言：主手命名牌数量变1（消耗1个）。
  pollUntilSucceed(test, () => {
    return getMainHandAmount(player) === 1;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const amount = getMainHandAmount(player);
      test.assert(false,
        `name_tag_cow_consumption: failed: mainHand amount=${amount} (expected 1, started with 2)`);
    },
  });
}

export function registerNameTagConsumptionTests(): void {
  GameTest.register("MobBehaviorTests", "name_tag_consumed_when_naming_cow", nameTagCowConsumptionTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
}
