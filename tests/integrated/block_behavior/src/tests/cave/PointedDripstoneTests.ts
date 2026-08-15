// 滴水石锥行为 GameTest（石笋尖端摔落伤害）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

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

export function registerPointedDripstoneTests(): void {
  GameTest.register("BlockBehaviorTests", "stalagmite_tip_kills_falling_entity", stalagmiteTipKillsFallingEntity)
    .structureName("gametests:fall_tower")
    .maxTicks(200);
}
