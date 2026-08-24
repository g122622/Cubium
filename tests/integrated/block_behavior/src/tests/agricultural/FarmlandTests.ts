// 农田方块行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底（满铺 49 glass），y=1..2 为玻璃墙围出的内部 air 空腔，y=3..4 air+顶部框架。
// 方块测试在内部 air 层操作，需特定支撑时显式 setBlockType 覆盖玻璃底。

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。
// 中心 (3,*,3) 为 1×1 垂直玻璃管落管：y=0 中心格默认 rail（非固体），y=1..14 中心柱 air，
// 四周管壁 glass（y=1..15），y=15 顶部 glass 封顶。用于实体高处自由落体踩踏耕地。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

// 农田上方被不透明固体方块遮挡时退化为泥土（wiki tech_农田.txt#退化：农田上方放置固体方块会使其变回泥土）。
//
// C++ 链路：FarmlandBlock::updatePostPlacement（FarmlandBlock.cpp:115-137）当 facing==Up 且上方方块
// hasOpaqueCollisionShape() 为真时，scheduleBlockTick(currentPos, this, 1) 安排 1 tick 后的刻。
// 随后 tick（:141-149）再次确认上方有固体方块 → turnToDirt（:229-239）将自身方块状态替换为 dirt
// （flags=3）。stone 放置（setBlockType flags=3）向下方 farmland 格派发 neighborChanged +
// updatePostPlacement(Up)，farmland 收到 Up 方向更新即安排 1 tick 后退化。退化需 1 tick 延迟
// （scheduledTick），非同 tick 同步。
//
// 判定手段：先放 farmland，再在其正上方放 stone。stone 放置触发 farmland updatePostPlacement(Up)
// → 1 tick 后 turnToDirt 变 dirt。succeedWhen 持续轮询断言 farmland 格变为 dirt（轮询覆盖 1 tick
// 延迟窗口），maxTicks 留足调度余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_农田.txt#退化（上方固体变泥土）
function farmlandRevertsToDirtWhenSolidAbove(test: Test): void {
  // 放农田 (3,1,1)，下方 (3,0,1) 为 glass_pit 玻璃底（支撑与否不影响退化，退化只看上方固体）。
  // setBlockType 直写 defaultState（moisture=0），不经 getStateForPlacement，放置本身不立即退化
  // （退化靠上方放方块的 updatePostPlacement）。
  test.setBlockType("minecraft:farmland", { x: 3, y: 1, z: 1 });

  // 正上方 (3,2,1) 放 stone（Material::ROCK，hasOpaqueCollisionShape=true）。stone 放置向下方 farmland
  // 派发 updatePostPlacement(Up)，farmland 安排 1 tick 后 turnToDirt。
  test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });

  // 持续轮询断言农田格变为泥土（1 tick 延迟后成立）。
  test.succeedWhenBlockPresent("minecraft:dirt", { x: 3, y: 1, z: 1 }, true);
}

// 耕地踩踏退化 GameTest：实体从高处落到耕地上，踩踏使其退化为泥土（wiki tech_农田.txt#退化：
// 实体落到耕地上有概率将其踩回泥土，落下距离越大概率越高）。
//
// C++ 链路：Entity::updateFallDistance → _handleLandingOnBlock（Entity.cpp，landingPos=floor(pos.y)-1）
// → FarmlandBlock::onFallenUpon（FarmlandBlock.cpp:202-225）。踩踏条件（对齐 vanilla FarmBlock.fallOn）：
//   1. 服务端（!isClientSide）；
//   2. 概率：random.nextFloat() < fallDistance - 0.5f（落下距离越大，踩踏概率越高）；
//   3. LivingEntity（物品/箭矢等非生物不踩踏）；
//   4. 玩家 或 mobGriefing 游戏规则 true（猪非玩家，依赖 mobGriefing 默认 true）；
//   5. width*width*height > 0.512（排除蝙蝠等小型实体；猪 0.9³=0.729>0.512 满足）。
// 满足则 turnToDirt（:229-239）将耕地格 setBlockState 为 dirt（flags=3）。之后调父类 Block::onFallenUpon
// 处理摔落伤害（耕地退化先于摔落伤害，即使实体摔死退化已生效）。
//
// vanilla 对照（FarmBlock.java:110-119 fallOn）：
//   if (!level.isClientSide && serverlevel.random.nextFloat() < p_397639_ - 0.5
//       && entity instanceof LivingEntity
//       && (entity instanceof Player || serverlevel.getGameRules().get(MOB_GRIEFING))
//       && entity.getBbWidth()*getBbWidth()*getBbHeight() > 0.512F) { turnToDirt(...); }
//   super.fallOn(...);
// Cubium 完全对齐。
//
// 几何（fall_tower 7×16×7，中心 1×1 玻璃管囚禁实体垂直自由落体）：
//   - (3,0,3) cobblestone：耕地下方固体支撑（FarmlandBlock isValidPosition 需 belowState.isSolid；
//     fall_tower y=0 中心格默认 rail 非固体，需铺 cobblestone 支撑耕地）。
//   - (3,1,3) farmland：耕地方块（setBlockType 直写 defaultState moisture=0，不经 getStateForPlacement，
//     放置本身不退化——退化靠上方固体 updatePostPlacement 或实体踩踏 onFallenUpon）。
//   - 猪 spawn (3,11,3)：沿玻璃管垂直自由落体到耕地顶（脚 y=2.0，耕地格 y=1），fallDistance≈9。
//
// 概率确定性：fallDistance≈9（从 y=11 落到 y=2），概率门槛 nextFloat < 9-0.5=8.5。nextFloat()∈[0,1)
// 恒小于 8.5 → 概率 100% 必触发踩踏。即使 Cubium fallDistance 累积有偏差（任务 #273），fallDistance
// 只要 ≥1.5 则概率门槛 ≥1.0 仍恒真（nextFloat<1.0 恒真）。大落差确保踩踏确定性触发，非 flaky。
//
// 判定手段：succeedWhenBlockPresent 每 tick 检查耕地格 (3,1,3) 变为 dirt（被踩踏退化）。退化在猪落地
// tick 同步生效（onFallenUpon 内 setBlockState dirt），succeedWhen 轮询覆盖落地时序。maxTicks=200
// 留足自由落体时间（fall_tower 高 16，猪从 y=11 落约 20 tick）。
//
// 与 farmland_reverts_to_dirt_when_solid_above 互补：前者覆盖上方固体 updatePostPlacement 退化，
// 本测试覆盖实体踩踏 onFallenUpon 退化，两条独立退化链路。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_农田.txt#退化（实体落到耕地踩回泥土，落差越大
//      概率越高）
// Ref: FarmlandBlock.cpp:202-225（onFallenUpon：概率 + LivingEntity + mobGriefing + 体积门控 → turnToDirt）
// Ref: FarmBlock.java:110-119（vanilla fallOn：nextFloat < fallDistance-0.5 + 同门控）
function farmlandTrampledToDirtByFallingEntity(test: Test): void {
  // (3,0,3) 放 cobblestone 作耕地下方固体支撑（fall_tower y=0 中心格默认 rail 非固体）。
  test.setBlockType("minecraft:cobblestone", { x: 3, y: 0, z: 3 });

  // (3,1,3) 放耕地（defaultState moisture=0）。放置本身不退化（无上方固体，无实体踩踏）。
  test.setBlockType("minecraft:farmland", { x: 3, y: 1, z: 3 });

  // 猪 spawn 于 (3,11,3)，沿 fall_tower 1×1 玻璃管垂直自由落体到耕地顶（脚 y=2.0），fallDistance≈9。
  test.spawn("pig", { x: 3, y: 11, z: 3 });

  // 断言耕地格 (3,1,3) 被猪踩踏退化为泥土（落地 onFallenUpon → turnToDirt 同步生效）。
  test.succeedWhenBlockPresent("minecraft:dirt", { x: 3, y: 1, z: 3 }, true);
}

export function registerFarmlandTests(): void {
  GameTest.register("BlockBehaviorTests", "farmland_reverts_to_dirt_when_solid_above", farmlandRevertsToDirtWhenSolidAbove)
    .structureName("gametests:glass_pit")
    .maxTicks(60);
  GameTest.register("BlockBehaviorTests", "farmland_trampled_to_dirt_by_falling_entity", farmlandTrampledToDirtByFallingEntity)
    .structureName("gametests:fall_tower")
    .maxTicks(200);
}
