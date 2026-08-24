// 滴水石锥行为 GameTest（石笋尖端摔落伤害、钟乳石坠落砸实体伤害）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { BlockPermutation } from "@minecraft/server";
import type { Vector3 } from "@minecraft/server";

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。
// 中心 (3,*,3) 为 1×1 垂直玻璃管落管：y=0 中心格默认为 rail（非固体），y=1..14 中心柱 air
// （下落通道），四周管壁 glass（y=1..15），y=15 顶部 (3,15,3) glass 封顶。用于 getEntities 的
// 区域限定查询。必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type})
// 跨测试污染。对照 HoneyBlockTests/Stone 测试同结构（它们把 y=0 中心格替换为被测完整方块）。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

// 石笋尖端高额摔落伤害 GameTest：猪从高处落到朝上的石笋尖端（thickness=tip）上，承受翻倍 +
// 偏移 2 的高额摔落伤害（远超普通方块），猪被摔死（实体消失）。
//
// C++ 链路：物理引擎 PhysicsEngine 用 BlockState::getCollisionShape() 收集碰撞箱（PhysicsEngine.cpp
// _getBlockCollisionBoxes），实体被石笋锥形碰撞箱阻挡、停在其顶面，触发 Entity::updateFallDistance →
// _handleLandingOnBlock（Entity.cpp，landingPos=floor(pos.y)-1=(3,1,3)）→ PointedDripstoneBlock::
// onFallenUpon（PointedDripstoneBlock.cpp）。着地方块为朝上 Tip 尖端时，以
// causeFallDamage(fallDistance + 2.0, 2.0, stalagmite()) 替代普通摔落伤害（对齐 Java
// PointedDripstoneBlock#fallOn / wiki"摔落高度增加 2，摔落伤害翻倍"）。
// LivingEntity::causeFallDamage 计算 (effectiveDistance - 3) * mult = (fallDistance + 2 - 3) * 2。
//
// PointedDripstoneBlock 默认状态即 VERTICAL_DIRECTION=Up + THICKNESS=Tip（构造 setDefaultState），
// setBlockType 放默认状态就是朝上尖端石笋，无需指定 block state。
//
// 【关键：石笋必须有碰撞箱才能阻挡实体】
// 滴水石锥在原版（Java/基岩）中具有锥形碰撞箱（wiki"滴水石锥尖端的碰撞箱为宽度0.375格、高度
// 0.6875格，但上下连接则时高度为1格"），PointedDripstoneBlock::getShape 已按厚度实现（Tip=0.5宽
// 1.0高中心柱）。CaveBlocks 注册时若误用 noCollision()，则 Block::getCollisionShape 直接返回空
// （Block.cpp: if(!m_hasCollision) return empty），实体穿过石笋落到下方方块，onFallenUpon 永不触发。
// 故注册不能带 noCollision()（notSolid 保留，滴石不作为通用固体支撑面）。
//
// 【关键：石笋放置位置必须有下方固体支撑】
// PointedDripstoneBlock::isValidPointedDripstonePlacement（PointedDripstoneBlock.cpp）：朝上石笋的
// 支撑方向为 Down，需下方方块 isSolidSide(Up)=true 或同向滴石。fall_tower y=0 中心格默认是 rail
// （非固体，isSolidSide=false），若石笋直接放 y=1 则下方 rail 无法支撑，updatePostPlacement 检测
// !isValidPointedDripstonePlacement 后对朝上石笋 scheduleBlockTick 1 tick 破坏（tick 内 setBlockState
// air），石笋消失，猪落空。故测试先在 (3,0,3) 放 cobblestone（固体，isSolidSide=true）提供支撑，
// 石笋再放 (3,1,3)。
//
// 落差设计：(3,0,3) 放 cobblestone 支撑，(3,1,3) 放石笋，猪 spawn (3,11,3)。猪自由落体到石笋顶面
// （石笋 Tip 碰撞箱高 1.0，顶面 y=2.0），落差约 9 格（fall_tower 自由落体实测 fallDistance≈8.x）。
// landingPos=floor(2.0)-1=1 → (3,1,3) 石笋，onFallenUpon 进入石笋分支。
// 石笋伤害 = ceil((8.x + 2 - 3) * 2) ≈ ceil(14+) ≥ 14 >> 猪 MAX_HEALTH 10，猪必死（实体消失）。
// 对照普通 cobble 同落差伤害 = (8.x-3)*1 ≈ 5.x（猪存活），证明石笋伤害远超普通方块（非落差假阳性）。
//
// 囚笼：fall_tower 中心 1×1 玻璃管（结构自带管壁 y=1..15）已围住 (3,*,3) 落管，猪只能垂直下落。
// 石笋 Tip 碰撞箱窄（0.5 宽）但 onFallenUpon 在着地时按方块网格触发（不依赖碰撞箱细节），猪宽 0.9
// AABB 完全覆盖石笋 0.5 碰撞箱水平范围，物理引擎 calculateYOffset 将猪停在石笋顶面，猪落在
// (3,1,3) 石笋方块即触发。
//
// 判定手段：succeedWhen 每 tick 检查猪实体已消失（length === 0，被高额伤害摔死）。区域限定
// fall_tower 7×16×7 排除并行测试污染。摔落是确定性时序（重力 + AABB，零随机），非 flaky。
// maxTicks=200 留足自由落体时间。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_滴水石锥.txt#石笋（thickness=tip 摔落高度+2、
//      伤害翻倍，伤害式 ⌈(h+2-3)*2⌉；碰撞箱宽度/高度）
function stalagmiteTipKillsFallingEntity(test: Test): void {
  const pigType = "pig";

  // (3,0,3) 放 cobblestone 提供固体支撑（fall_tower y=0 中心格默认 rail 非固体，无法支撑石笋）。
  test.setBlockType("minecraft:cobblestone", { x: 3, y: 0, z: 3 });

  // (3,1,3) 放滴水石锥（默认状态=朝上 Tip 尖端石笋）。
  test.setBlockType("minecraft:pointed_dripstone", { x: 3, y: 1, z: 3 });

  // 猪 spawn 于 (3,11,3)（石笋正上方约 10 格），自由落体到石笋尖端顶面。
  test.spawn(pigType, { x: 3, y: 11, z: 3 });

  // 断言猪被石笋摔死：succeedWhen 每 tick 检查猪实体已消失（length === 0）。
  test.succeedWhen(() => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(TOWER_FROM),
      volume: TOWER_VOLUME,
    });
    test.assert(pigs.length === 0,
      `pig survived stalagmite tip fall (should take >=14 damage and die), remaining=${pigs.length}`);
  });
}

// 钟乳石坠落砸实体伤害 GameTest：朝下尖端的钟乳石失去上方支撑后坠落，砸中下方站立的实体造成
// 高额伤害（远超实体血量），实体被砸死消失。
//
// C++ 链路（独立于石笋 onFallenUpon 的另一条伤害链路，零测试覆盖）：
//   1. 触发：朝下 tip 钟乳石上方支撑被移除 → PointedDripstoneBlock::updatePostPlacement
//      （PointedDripstoneBlock.cpp:190-229）facing==Up（==opposite(Down tipDirection)）→
//      isValidPointedDripstonePlacement（:164-184，朝下钟乳石支撑方向 Up，需上方 isSolidSide(Up)
//      或同向滴石）返回 false（上方变 air）→ tipDirection==Down → scheduleBlockTick(DELAY_BEFORE_FALLING=2)。
//   2. tick（:290-308）：isStalactite → _spawnFallingStalactite（:951-1014）逐格向下生成
//      FallingBlockEntity，仅尖端 isTip 设伤害参数：
//        fallHeight = max(pos.y - currentPos.y + 1, 6) = max(10-10+1, 6) = 6
//        damagePerDist = FALLING_STALACTITE_FALL_DAMAGE_PER_DISTANCE(1.0F) * fallHeight = 6.0
//        maxDmg = FALLING_STALACTITE_MAX_DAMAGE = 40
//        damageType = FallingStalactite
//   3. FallingBlockEntity::tick（MiscEntities.cpp:152-206）：重力 -0.04/tick 下落，onGround 时
//      _handleLanding（:208）→ m_hurtEntities 时 _hurtEntities（:399-459）：
//        fallDistance = m_fallStartY(10) - y()(落地≈1.0) ≈ 9.0
//        effectiveDistance = ceil(fallDistance - 1.0) = ceil(8.0) = 8
//        damage = min(effectiveDistance * damagePerDist, maxDmg) = min(8*6, 40) = min(48, 40) = 40
//        FallingStalactite 分支：EntityDamageSource(DamageSources::fallingStalactite(this))
//        hurtBox = boundingBox()（FallingBlockEntity size 0.98×0.98，落地 y∈[1.0,1.98]）
//        遍历 hurtBox 内 LivingEntity 调 hurt（仅生物）。
//
// vanilla 对照（PointedDripstoneBlock.java:291-303 spawnFallingStalactite）：
//   int i = Math.max(1 + startY - currentY, 6); float f = 1.0F * i;
//   fallingblockentity.setHurtsEntities(f, 40); break;
// Cubium 完全对齐（fallHeight、damagePerDist=1.0F*i、maxDmg=40、damageType=FallingStalactite）。
//
// 几何（fall_tower 中心 1×1 玻璃管囚笼，结构自带管壁 y=1..15 围住 (3,*,3)）：
//   - (3,0,3) cobblestone：猪落脚支撑面（fall_tower y=0 中心格默认 rail 非固体，需铺 cobblestone
//     作猪落脚实体方块）。猪 spawn (3,2,3) 下落落 cobblestone 顶（脚 y=1.0），AABB y∈[1.0,1.9]，
//     水平 0.9 宽中心 (3.5,*,3.5)。
//   - (3,10,3) 朝下 tip 钟乳石：坠落源。需 vertical_direction=down + dripstone_thickness=tip
//     （默认状态是朝上 tip 石笋，必须显式设朝下）。用 setBlockPermutation 放置。
//   - (3,11,3) cobblestone：钟乳石上方支撑（朝下钟乳石支撑方向 Up，需上方 isSolidSide(Up)）。
//     待 runAtTickTime(2) 移除设 air 触发钟乳石 updatePostPlacement(Up,air) → 坠落链路。
//   - 中间 (3,2..9,3) air：FallingBlockEntity 下落通道（fall_tower 管内中心柱 air，畅通无阻）。
//
// 时序（确定性，零随机）：
//   tick 2：移除 (3,11,3) 支撑 → 钟乳石 updatePostPlacement 同步 scheduleBlockTick(2)。
//   tick 4（2+DELAY_BEFORE_FALLING）：tick 触发 _spawnFallingStalactite 生成 FallingBlockEntity
//     于 (3,10,3)，钟乳石格变 air。
//   tick 4 起：FallingBlockEntity 重力下落，约 30-40 tick 从 y=10 落到 y=1（重力 -0.04/tick 累积）。
//   落地 onGround：_hurtEntities，hurtBox(y∈[1.0,1.98]) 与猪 AABB(y∈[1.0,1.9]) 相交，命中猪
//     造成 40 伤害 >> 猪 MAX_HEALTH 10，猪死。
//
// 判定手段：succeedWhen 每 tick 检查猪实体已消失（length===0，被 40 伤害砸死）。区域限定
// fall_tower 7×16×7 排除并行测试污染。下落是确定性时序（重力 + AABB，零随机），非 flaky。
// maxTicks=300 留足下落时间（重力 -0.04/tick，10 格下落约 30-40 tick + 落地余量）。
//
// 与 stalagmite_tip_kills_falling_entity 互补：石笋测试覆盖 onFallenUpon（实体摔到石笋尖端），
// 本测试覆盖 _spawnFallingStalactite + _hurtEntities（钟乳石坠落砸实体），两条独立伤害链路。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_滴水石锥.txt#钟乳石（钟乳石从天花板坠落
//      砸中下方实体造成伤害，伤害 = 下落格数 × 系数，上限 40）
// Ref: PointedDripstoneBlock.cpp:951-1014（_spawnFallingStalactite：尖端设伤害参数）
// Ref: MiscEntities.cpp:399-459（_hurtEntities：fallDistance/damage 计算 + FallingStalactite 分支）
// Ref: PointedDripstoneBlock.java:291-303（vanilla spawnFallingStalactite：setHurtsEntities(f,40)）
function stalactiteFallKillsEntityBelow(test: Test): void {
  const pigType = "pig";

  // (3,0,3) 放 cobblestone 作猪落脚支撑面（fall_tower y=0 中心格默认 rail 非固体）。
  test.setBlockType("minecraft:cobblestone", { x: 3, y: 0, z: 3 });

  // (3,10,3) 放朝下 tip 钟乳石（坠落源）。默认状态是朝上 tip 石笋，必须显式设 vertical_direction=down。
  // 用 BlockPermutation.resolve + setBlockPermutation（同 sweetBerryBush.ts 跨服务端范式）。
  // any 绕过 @minecraft/server 两版本 BlockPermutation 类型冲突（见 sweetBerryBush.ts 文件头注释）。
  const stalactitePermutation = BlockPermutation.resolve("minecraft:pointed_dripstone", {
    vertical_direction: "down",
    dripstone_thickness: "tip",
  }) as any;
  (test as unknown as {
    setBlockPermutation: (blockData: unknown, blockLocation: Vector3) => void;
  }).setBlockPermutation(stalactitePermutation, { x: 3, y: 10, z: 3 });

  // (3,11,3) 放 cobblestone 作钟乳石上方支撑（朝下钟乳石支撑方向 Up，需上方 isSolidSide(Up)=true）。
  // 待移除触发坠落。
  test.setBlockType("minecraft:cobblestone", { x: 3, y: 11, z: 3 });

  // 猪 spawn 于 (3,2,3)，下落落 cobblestone(3,0,3) 顶面（脚 y=1.0），等待钟乳石坠落砸中。
  test.spawn(pigType, { x: 3, y: 2, z: 3 });

  // tick 2 移除 (3,11,3) 上方支撑（设 air）→ 钟乳石 updatePostPlacement(Up, air) →
  // !isValidPointedDripstonePlacement → scheduleBlockTick(DELAY_BEFORE_FALLING=2) → tick →
  // _spawnFallingStalactite 生成 FallingBlockEntity 坠落砸猪。
  test.runAtTickTime(2, () => {
    test.setBlockType("minecraft:air", { x: 3, y: 11, z: 3 });
  });

  // 断言猪被钟乳石坠落砸死：succeedWhen 每 tick 检查猪实体已消失（length===0，40 伤害 >> 10 血）。
  test.succeedWhen(() => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(TOWER_FROM),
      volume: TOWER_VOLUME,
    });
    test.assert(pigs.length === 0,
      `pig survived falling stalactite (should take 40 damage and die), remaining=${pigs.length}`);
  });
}

export function registerPointedDripstoneTests(): void {
  GameTest.register("BlockBehaviorTests", "stalagmite_tip_kills_falling_entity", stalagmiteTipKillsFallingEntity)
    .structureName("gametests:fall_tower")
    .maxTicks(200);
  GameTest.register("BlockBehaviorTests", "stalactite_fall_kills_entity_below", stalactiteFallKillsEntityBelow)
    .structureName("gametests:fall_tower")
    .maxTicks(300);
}
