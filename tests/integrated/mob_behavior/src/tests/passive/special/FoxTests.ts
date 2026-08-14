// 狐狸行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// grass_pen 结构尺寸（9×5×9），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 狐狸免疫甜浆果灌木伤害与减速（wiki tech_狐狸.txt#行为：狐狸不受甜浆果灌木的伤害和减速影响；
// 蜜蜂同理免疫）。成熟（AGE≥2）灌木对其他 LivingEntity 在其中水平移动时每 ~10 tick 造成 1 伤害。
//
// C++ 链路：SweetBerryBushBlock::onEntityCollision（SweetBerryBushBlock.cpp:170-214）对 LivingEntity
//   生效，但 typeId=="minecraft:fox"||"minecraft:bee" 直接 return 免疫伤害+减速；其他 LivingEntity
//   在 AGE>0 灌木中水平移动（|dx|或|dz|≥0.003，对齐 vanilla horizontalDistanceSqr）受 hurt(1.0)
//   （有无敌帧约 10 tick 一次）。 Entity::doBlockCollisions（Entity.cpp:1296-1365）遍历实体 AABB
//   覆盖的所有方块格调 onEntityCollision，故实体 AABB 须覆盖灌木格。
//   碰撞箱对齐：甜浆果灌木不重写 getCollisionShape，继承 BushBlock 返回 empty（对齐 vanilla
//   noCollision()），实体可穿过灌木落入其中持续触发 onEntityCollision。此前 override 返回 fullShape
//   致实体被挡在灌木外，伤害链路变死代码——已修复（删除 getCollisionShape override）。
//
// 环境选择：grass_pen（9×5×9）。结构放置抬高一格（placeOrigin=origin+(0,1,0)），
// helper 相对坐标用 origin（gridStartY=-59），故 helper-y=N → 世界 y=-59+N：
//   helper-y=1 → 世界 y=-58 = 结构内 y=0（满铺 grass_block 草地地板）
//   helper-y=2 → 世界 y=-57 = 结构内 y=1（air，本测试在此满铺 AGE=3 灌木）
//   helper-y=3 → 世界 y=-56 = 结构内 y=2（air，实体 spawn 位）
// 甜浆果灌木需种在草地/泥土上（canSustain 检查 BlockTags::VALID_SWEET_BERRY_BUSH_GROUND），
// helper-y=2 灌木的下方 helper-y=1 是 grass_block 草地，支撑通过。实体 spawn 在 helper-y=3，
// 落到草地顶（世界 y=-57.0=灌木格底），AABB 覆盖灌木格（世界 y=-57）：鸡高 0.7 → AABB y∈[-57,-56.3]
// 覆盖灌木格 y=-57；狐狸用基类默认高 1.8 → AABB y∈[-57,-55.2] 覆盖灌木格 y=-57。两者均触发
// onEntityCollision。满铺 7×7（x,z∈[1,7]）确保实体随机走动始终处于灌木格水平投影内。
//
// 实体移动前提（关键修复）：onEntityCollision 伤害需实体水平移动。RandomWalkingGoal 经
//   RandomPositionGenerator::findRandomTarget→isPositionWalkable 找可行走目标。此前 isPositionWalkable
//   用 !isAir()&&!isLiquid() 判阻挡，把 noCollision 的灌木误判为"被阻挡"，致 RandomWalking 找不到
//   目标、实体静止、水平位移为 0、伤害永不触发。已修复为 !isAir()&&!isLiquid()&&blocksMovement()
//   （对齐 vanilla isPathfindable(LAND)：noCollision 方块不阻挡寻路）。
//
// 狐狸睡眠规避：FoxSleepGoal（白天+hasShelter 时触发，mutex 占用 Move 阻塞 RandomWalking）。
//   hasShelter = !canSeeSky。grass_pen 埋在 gridStartY=-59 地下 worldgen 石头中，canSeeSky 恒 false
//   → hasShelter=true → 狐狸白天睡眠不动。故加 skyAccess(true) 清空结构上方制造露天列，canSeeSky=true
//   → hasShelter=false → 狐狸不睡，RandomWalking 可执行（狐狸免疫，移动也不掉血，验证免疫需它移动）。
//
// 对照设计（严谨性）：单断言"狐狸不掉血"无法排除"灌木根本不造成伤害"的假阴性。故加鸡作对照——
//   鸡在灌木中水平移动受伤（HP<4 或死亡），证明灌木伤害链路对非狐狸生效，反证狐狸"不掉血"是免疫
//   而非灌木失效。双断言：狐狸 HP==10（免疫）+ 鸡 HP<4 或消失（灌木造伤）。
//
// 判定手段：succeedWhen 每 tick 查区域内狐狸与鸡，断言狐狸 HP==10（从未受伤）且鸡 HP<4 或消失。
// 时序：实体落入灌木 + RandomWalking 水平移动触发 onEntityCollision，鸡 4 血每 ~10 tick 1 伤害，
// 4 次约 40+ tick 致死。狐狸始终免疫 HP=10。maxTicks=800 留充裕余量吸收随机移动 + 无敌帧节奏 +
//   GameTest 非确定性。狐狸/鸡查询用区域限定排除并行测试污染；type 用 "minecraft:fox"/"minecraft:chicken"。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狐狸.txt#行为（免疫甜浆果灌木伤害与减速）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_甜浆果.txt#生物（除狐狸蜜蜂外 age≥2 灌木移动受伤）
function foxImmuneToSweetBerryBush(test: Test): void {
  const foxType = "fox";
  const chickenType = "chicken";

  // 在 helper-y=2（世界 y=-57）满铺 7×7 AGE=3 甜浆果灌木（x,z∈[1,7]）。下方 helper-y=1 是 grass_pen
  // 草地，支撑灌木（canSustain 通过）。setBlockWithStates 设 age=3（成熟，AGE>0 造伤）。
  // flags=3 含邻居更新，放灌木时下方草地已就位，updatePostPlacement 支撑检查通过不移除。
  for (let x = 1; x <= 7; x++) {
    for (let z = 1; z <= 7; z++) {
      (test as any).setBlockWithStates("minecraft:sweet_berry_bush", { x, y: 2, z }, "age=3", 3);
    }
  }

  // 狐狸 (2,3,2)、鸡 (5,3,5) spawn 在 helper-y=3，落入 helper-y=2 灌木层（停在草地顶，AABB 覆盖灌木格）。
  // 分散站位避免相互推挤。两者都是被动生物有 RandomWalkingGoal，灌木（noCollision，blocksMovement=false）
  // 修复后不阻挡寻路，实体可在灌木中水平移动触发 onEntityCollision。
  test.spawn(foxType, { x: 2, y: 3, z: 2 });
  test.spawn(chickenType, { x: 5, y: 3, z: 5 });

  // 双断言：狐狸免疫（HP==10）+ 鸡受伤（HP<4 或消失）。
  // succeedWhen 每 tick 持续检查：狐狸必须始终满血（免疫从未受伤），鸡必须掉血或死亡。
  // 鸡 4 血，灌木每 ~10 tick 1 伤害，4 次约 40+ tick 致死；maxTicks=800 留充裕余量吸收随机性。
  test.succeedWhen(() => {
    const foxes = test.getDimension().getEntities({
      type: "minecraft:fox",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(foxes.length > 0, "fox disappeared (should be immune, not die)");
    const foxHealth = foxes[0].getComponent("minecraft:health");
    test.assert(foxHealth !== undefined, "fox has no health component");
    // 狐狸免疫：HP 必须保持满血 10（从未受灌木伤害）。
    test.assert((foxHealth as any).currentValue === 10,
      `fox took sweet berry bush damage (should be immune), hp=${(foxHealth as any).currentValue}`);

    const chickens = test.getDimension().getEntities({
      type: "minecraft:chicken",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    // 鸡受伤死亡消失——灌木伤害链路对非狐狸生效。
    if (chickens.length === 0) {
      return;
    }
    const chickenHealth = chickens[0].getComponent("minecraft:health");
    test.assert(chickenHealth !== undefined, "chicken has no health component");
    // 鸡必须在灌木中受伤（HP<4 满血），证明灌木造伤链路生效（反证狐狸不掉血是免疫而非灌木失效）。
    test.assert((chickenHealth as any).currentValue < 4,
      `chicken not damaged by sweet berry bush (bush damage link may be broken), hp=${(chickenHealth as any).currentValue}`);
  });
}

export function registerFoxTests(): void {
  GameTest.register("MobBehaviorTests", "fox_immune_to_sweet_berry_bush", foxImmuneToSweetBerryBush)
    .structureName("gametests:grass_pen")
    // skyAccess(true)：清空结构上方 worldgen 制造露天列使 canSeeSky=true → hasShelter=false →
    // 狐狸白天不睡眠（FoxSleepGoal 不触发），RandomWalking 可执行移动验证免疫机制。
    .skyAccess(true)
    .maxTicks(800);
}
