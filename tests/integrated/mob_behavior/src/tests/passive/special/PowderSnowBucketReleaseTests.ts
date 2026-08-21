// 粉雪桶放粉雪行为类 GameTest（反向链路，对应空桶舀取粉雪的正向链路）。
//
// 验证玩家手持粉雪桶右键地面放出粉雪方块，并返回空桶（wiki tech_细雪.txt#获取：空桶右键粉雪
// 获得粉雪桶；tech_细雪.txt#放置：粉雪桶右键放置粉雪方块并消耗返回空桶）。
//
// 对齐 Java 1.21.11 SolidBucketItem.use 流程（Cubium 实现为 PowderSnowBucketItem::onItemUse）：
//   1. Survival 玩家主手持 powder_snow_bucket + useItemOnBlock(grass_block, face=Up)
//      → SimulatedPlayer::useItemOnBlock 先 Block.use（grass_block onBlockActivated 基类返 Pass）
//      → fallback Item.useOn → PowderSnowBucketItem::onItemUse。
//   2. onItemUse（PowderSnowBucketItem.cpp:50-88）：水下使用返 Consume（不允许水下放置）；
//      否则 targetPos = blockPos.offset(face=Up)（被点击方块上方 air），emptyContents 放置粉雪方块
//      + 非创造模式 shrink(1) + _returnEmptyBucket 返回空桶。
//   3. emptyContents（PowderSnowBucketItem.cpp:90-122）：目标格须为 air（isAir），setBlockState 粉雪
//      + gameEvent(BLOCK_PLACE) + playSound(ITEM_BUCKET_EMPTY_POWDER_SNOW)。
//
// 此前 PowderSnowBucketItem::onItemUse 与 FishBucketItem 同源对齐缺陷：用 context.getItemStackMut()
// （调用方局部拷贝）做 shrink+_returnEmptyBucket，拷贝赋值不回写权威物品栏——玩家持粉雪桶放粉雪后
// 得不到空桶。已改用 player->getHeldItem(hand) 权威引用修复（同 FishBucketItem 范式）。本测试覆盖
// 反向放粉雪链路（PowderSnowBucketItem.onItemUse + emptyContents），验证修复后主手变空桶。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板 + y=1..4 全 air）。玩家右键 grass_block
// (3,1,3)（结构 y=0 grass_block）顶面（face=Up，useItemOnBlock 默认），placePos=(3,2,3)（结构 y=1 air）
// 放粉雪方块。结构放置 +1 抬升：helper-y=N → 结构内 y=N-1，故 helper (3,1,3)=grass_block，
// (3,2,3)=air。粉雪桶不涉及流体 tick（粉雪是固体方块），放置即生效。
//
// 判定手段（双重断言）：
//   1. placePos (3,2,3) 方块 typeId 为 minecraft:powder_snow（emptyContents 放置）；
//   2. Survival 玩家主手槽（slot 0）typeId 变为 minecraft:bucket（粉雪桶消耗返回空桶）。
// 创造模式不消耗粉雪桶（onItemUse 内 isCreative 守卫跳过 shrink+_returnEmptyBucket），故必须 Survival
// 玩家才能以"主手变空桶"作为放粉雪成功证据。读取主手槽用
// getComponent("minecraft:inventory").container.getItem(0)（slot 0=主手，对齐 setItem(...,0,true)）。
// 方块查询用 test.getBlock(pos).typeId（对齐 EndRodTests/DragonEggTests 范式）。
// 区域限定用 PIT（creeper_pit 7×5×7）仅作注释说明，方块查询是单点查询无需区域限定。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标 x,z∈[0,6], y∈[0,4]。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 放粉雪后主手应变为此物品（空桶）。
const EMPTY_BUCKET_ITEM = "minecraft:bucket";
// 放置目标格应变为此方块（粉雪方块）。
const POWDER_SNOW_BLOCK = "minecraft:powder_snow";
// 玩家初始手持的粉雪桶 typeId。
const POWDER_SNOW_BUCKET_ITEM = "minecraft:powder_snow_bucket";
// 放置目标格（grass_block (3,1,3) 上方 air，placePos=offset(Up)）。
const PLACE_POS = { x: 3, y: 2, z: 3 };

// 通用放粉雪测试：Survival 玩家持粉雪桶右键 grass_block 顶面，断言出现粉雪方块 + 主手变空桶。
function powderSnowBucketReleaseTest(test: Test): void {
  // Survival 玩家 (1,2,3) 持粉雪桶（slot 0 主手）。距放置点 2 格（useItemOnBlock 远程触发无距离门控）。
  // Survival 模式放粉雪会消耗粉雪桶返回空桶（创造模式保留粉雪桶），消耗/替换是放粉雪成功的判定证据。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "powderSnowReleaser", 0 as any);
  const powderSnowBucket = new ItemStack(POWDER_SNOW_BUCKET_ITEM, 1);
  player.setItem(powderSnowBucket as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持粉雪桶右键 grass_block (3,1,3) 顶面（face=Up 默认）
  // → PowderSnowBucketItem::onItemUse → placePos=(3,2,3) 放粉雪 + 主手变空桶。
  // tick 5 留足 spawn 注册 + 首 tick 稳定。
  test.runAtTickTime(5, () => {
    player.useItemOnBlock(
      powderSnowBucket as unknown as Parameters<typeof player.useItemOnBlock>[0],
      { x: 3, y: 1, z: 3 },
    );
  });

  // 轮询双重断言：placePos 变粉雪方块（emptyContents 放置）+ 主手变空桶。
  // onItemUse 在 tick 5 后下一 tick 即可查（setBlockState + heldItem 赋值同步生效）。
  pollUntilSucceed(test, () => {
    // 断言 1：placePos (3,2,3) 方块 typeId 为粉雪方块（emptyContents 放置）。
    const block = test.getBlock(PLACE_POS) as unknown as { typeId?: string } | undefined;
    if (block?.typeId !== POWDER_SNOW_BLOCK) return false;

    // 断言 2：主手槽（slot 0）typeId 为空桶（粉雪桶消耗返回空桶）。
    const inv = player.getComponent("minecraft:inventory") as any;
    if (inv === undefined || inv.container === undefined) return false;
    const mainHand = inv.container.getItem(0);
    if (mainHand === undefined) return false;
    return mainHand.typeId === EMPTY_BUCKET_ITEM;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const block = test.getBlock(PLACE_POS) as unknown as { typeId?: string } | undefined;
      const inv = player.getComponent("minecraft:inventory") as any;
      const mainHand = (inv?.container?.getItem?.(0)) as any;
      const typeId = mainHand?.typeId;
      const amount = mainHand?.amount;
      // 区域内粉雪方块计数（辅助诊断，确认是否放置在意外位置）。
      let powderSnowCount = 0;
      for (let x = PIT_FROM.x; x < PIT_FROM.x + PIT_VOLUME.x; x++) {
        for (let y = PIT_FROM.y; y < PIT_FROM.y + PIT_VOLUME.y; y++) {
          for (let z = PIT_FROM.z; z < PIT_FROM.z + PIT_VOLUME.z; z++) {
            const b = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
            if (b?.typeId === POWDER_SNOW_BLOCK) powderSnowCount++;
          }
        }
      }
      test.assert(false,
        `powder_snow_bucket_releases_powder_snow: release failed: `
        + `placePos(${PLACE_POS.x},${PLACE_POS.y},${PLACE_POS.z})={typeId:${block?.typeId}} `
        + `mainHand={typeId:${typeId}, amount:${amount}} `
        + `powderSnowCountInPit=${powderSnowCount} `
        + `(expected placePos=${POWDER_SNOW_BLOCK} and mainHand=${EMPTY_BUCKET_ITEM})`);
    },
  });
}

export function registerPowderSnowBucketReleaseTests(): void {
  GameTest.register("MobBehaviorTests", "powder_snow_bucket_releases_powder_snow", powderSnowBucketReleaseTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
}
