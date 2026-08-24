// 梯子附着面自毁行为 GameTest（移除背面附着方块时梯子自毁）。
//
// wiki tech_梯子.txt（:52）：「梯子只能被放置在一个方块完整的侧面上。」即梯子须附在固体方块的
// 完整侧面（isSolidSide）。背面附着方块被移除时，梯子失去附着自毁掉落（vanilla 经 neighborChanged
// → updateShape 链）。
//
// C++ 链路：LadderBlock 有 facing state（HORIZONTAL_FACING，梯子朝向=背离附着面）。
//   - isValidPosition（LadderBlock.cpp:101-115）：facing 反方向（背面 attachPos）方块的
//     isSolidSide(world, attachPos, facing)。
//   - updatePostPlacement（:117-145）：facing==opposite(ladderFacing)（背面邻居变化）且
//     isSolidSide(attachState, attachPos, ladderFacing) 失败时返回 air 自毁。同 tick 同步。
// 梯子 facing=East 表示梯子朝东，背面在 West 邻位（附在 West 邻位方块的 East 面）。
// isSolidSide 对固体不透明方块（stone）的完整面返回 true；air/glass（透明）返回 false。
//
// 放置语义：setBlockWithStates 走 _resolveBlock 取 defaultState（facing=North），经 setBlockWithStates
// 显式设 facing=east，不经 isValidPosition，故即使背面非固体也能强放。LadderBlock 无 onBlockAdded
// 重写，放置不向自身派发 updatePostPlacement，强放不立即自毁。需「第二步移除背面附着方块」触发梯子
// opposite(facing) 方向 updatePostPlacement 才自毁。
//
// 测试覆盖（1 个场景，行为与 vanilla 一致，可跨服务端对比）：
//   - 梯子 facing=east 附在 West 邻位 stone 的 East 面，移除 stone → 梯子自毁。
//
// 关键约束（同支撑自毁范式，见 VineTests/LanternTests）：
// 1. 先放背面 stone 再放梯子，保证梯子强放时背面有附着（贴近 vanilla 放置语义）。放置不向自身
//    派发 updatePostPlacement，facing=east 被保留。
// 2. 移除背面 stone 必须是非 no-op 写入——先显式铺 stone 再放梯子，再设 air 移除 stone，保证
//    stone→air 真实状态变化派发更新。air 放置向 East 邻位梯子派发 updatePostPlacement(West) →
//    facing=east→ladderFacing=East→背面 West=(2,1,1)=air → isSolidSide(air, east) false → 返回 air，
//    梯子自毁。
//
// 不测「梯子攀爬速度」：依赖实体碰撞箱 + isLadder，属实体行为，非方块状态行为点，跳过。
// 不测「梯子含水（waterlogged）」：依赖水流动/含水体系，且支撑自毁核心行为点与含水无关，跳过。
// 不测「四个朝向（north/south/west）」：与 east 对称，行为点相同（isSolidSide 判定），按「单一职责」
// 本文件聚焦 east 朝向。TODO: 可补 ladder_other_facings 覆盖其余朝向。
//
// 跨服务端：梯子 facing state 名两端一致（Java 式 north/south/east/west），附着面自毁行为与 vanilla
// 一致（isSolidSide 失败即破坏，同步），可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_梯子.txt（梯子放置在方块完整侧面）
// Ref: LadderBlock.cpp（isValidPosition/updatePostPlacement isSolidSide 附着面自毁）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。
// 中心 (3,*,3) 为 1×1 垂直玻璃管落管：y=0 中心格默认非固体，y=1..14 中心柱 air（下落通道），
// 四周管壁 glass（y=1..15）。用于攀爬摔伤免疫测试的垂直下落场景 + getEntities 区域限定查询。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。
// 测试用 (2,1,1) 作背面附着 stone，(3,1,1) 作梯子，均在 air 空腔内。

// 移除梯子背面附着方块（石头）时梯子自毁变 air。
//
// 布局：(2,1,1) 铺 stone 作梯子背面附着（isSolidSide(east) true），(3,1,1) 用 setBlockWithStates
// 放 facing=east 梯子（背面 West=(2,1,1) stone），再 (2,1,1) 设 air 移除附着。
// air 放置向 East 邻位梯子派发 updatePostPlacement(West=opposite(east)) → 背面 (2,1,1)=air →
// isSolidSide(air, east) false → 返回 air，梯子自毁。
//
// 判定：succeedWhenBlockPresent 断言梯子格 (3,1,1) 梯子消失（同 tick 同步）。
function ladderBreaksWhenAttachedBlockRemoved(test: Test): void {
    // (2,1,1) 铺 stone 作梯子背面附着（isSolidSide(east) true，梯子 facing=east 附在其 East 面）。
    test.setBlockType("minecraft:stone", { x: 2, y: 1, z: 1 });

    // (3,1,1) 用 setBlockWithStates 放 facing=east 梯子（背面 West=(2,1,1) stone）。setBlockType 取
    // defaultState（facing=North），需 setBlockWithStates 显式设 facing=east 使背面为 West 邻位。
    test.setBlockWithStates("minecraft:ladder", { x: 3, y: 1, z: 1 }, "facing=east");

    // (2,1,1) 设 air 移除背面附着（stone→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向
    // East 邻位梯子派发 updatePostPlacement(West) → ladderFacing=East→opposite=West→背面 (2,1,1)=air →
    // isSolidSide(air, east) false → 返回 air，梯子自毁。
    test.setBlockType("minecraft:air", { x: 2, y: 1, z: 1 });

    // 断言梯子格 (3,1,1) 梯子已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:ladder", { x: 3, y: 1, z: 1 }, false);
}

// 梯子攀爬摔伤免疫 GameTest：实体从高处落入梯子柱，沿梯子缓慢下滑期间 fallDistance 被
// handleOnClimbable resetFallDistance 反复清零，落到下方实方块时不承受摔落伤害（hp 保持满血 10）。
//
// C++ 链路（对齐 vanilla LivingEntity#handleOnClimbable，LivingEntity.java:2570-2572）：
//   LivingEntity::travel（LivingEntity.cpp:1876）攀爬分支 onLadder=true（isOnLadder 检测碰撞箱内
//   方块 isLadder，LadderBlock::isLadder 对梯子返回 true）时，分支开头 resetFallDistance——对齐
//   vanilla handleOnClimbable 第一行 this.resetFallDistance()。实体沿梯子缓慢下滑（velocity.y 限为
//   -LADDER_SPEED_MAX 小负值）时 fallDistance 不累积，离开梯子落地不摔伤。
//
//   此前 Cubium 缺这行 resetFallDistance，攀爬时 updateFallDistance（Entity.cpp:1074，不考虑攀爬
//   状态，y<0 即累积 fallDistance -= y）持续累积下滑量，落地触发摔伤——与 vanilla 偏差。修复后在
//   攀爬分支开头清零 fallDistance 对齐 vanilla。
//
//   注：FALL_DAMAGE_RESETTING 射线（_checkFallDamageResettingBlocks）是补充机制，仅在本帧位移
//   >=1.0 时触发；梯子缓慢下滑位移 <1.0 故射线不生效，摔伤免疫完全依赖此 handleOnClimbable 主机制。
//   梯子与 cobweb/sweet_berry_bush 不同：cobweb/sweet_berry_bush 无 isLadder，靠 setMotionMultiplier
//   首行 resetFallDistance + 射线双机制；梯子靠 isLadder 攀爬物理 + handleOnClimbable resetFallDistance。
//
// vanilla 对照（LivingEntity.java:2570-2572 handleOnClimbable）：
//   if (this.onClimbable()) {
//       this.resetFallDistance();   // ← 攀爬时清零 fallDistance（主机制）
//       ...
//       double d2 = Math.max(p_21298_.y, -0.15F);  // 限制下滑速度
//   }
//   onClimbable() 检查碰撞箱内方块 isLadder 或 BlockTags.CLIMBABLE。Cubium isOnLadder 只查 isLadder
//   虚函数（LadderBlock/VineBlock/ScaffoldingBlock 重写），weeping/twisting/cave vines 未重写 isLadder
//   故实体在其上不触发攀爬物理（与 vanilla 偏差，TODO，见 BlockTags.hpp CLIMBABLE 注释）。
//
// 几何（fall_tower 中心 1×1 玻璃管囚禁实体垂直下落，结构尺寸 7×16×7）：
//   - (3,0,3) cobblestone：承接猪的实方块（fall_tower y=0 中心格默认非固体）。
//   - (3,1..10,3) ladder 梯子柱：猪从 (3,11,3) 落入梯子柱，isOnLadder 检测碰撞箱内梯子 isLadder=true
//     → onLadder=true → 攀爬物理（缓慢下滑 + resetFallDistance）。梯子 getCollisionShape=empty
//     （LadderBlock.cpp:174-180 不阻挡下落），猪穿过梯子柱下滑。梯子 facing=east 强放（setBlockWithStates
//     不经 isValidPosition，背面 glass 不变不触发自毁）。
//   - 猪 spawn (3,11,3)：落入梯子柱顶部，沿梯子缓慢下滑到 (3,0,3) cobblestone 顶面。
//
// 判定手段：runAtTickTime(100) 在 100 tick 后检查猪 hp===10（满血，未摔伤）。落差约 10 格（y=11→y=1），
// 若无 resetFallDistance，沿梯子下滑累积 fallDistance≈10，落地应承受 (10-3)*1=7 摔落伤害（hp=3）。
// hp===10 精确证明 handleOnClimbable resetFallDistance 生效。tick 100 留足余量（梯子下滑极慢，
// LADDER_SPEED_MAX 限速，10 格下滑需约 60-80 tick + 落地余量）。
// 区域限定 fall_tower 7×16×7 排除并行测试污染。
//
// 选猪：猪 MAX_HEALTH=10，落差 10 格无重置则 7 伤害（hp=3）与 hp===10 区分明显。猪体积适中落
// fall_tower 1×1 玻璃管居中。
//
// 与 cobweb_resets_fall_distance_no_damage / sweet_berry_bush_resets_fall_distance_no_damage 互证：
// 三者验证不同摔伤免疫机制（cobweb/sweet_berry_bush 走 setMotionMultiplier+射线双机制，梯子走
// isLadder+handleOnClimbable 攀爬主机制）对各自方块生效。若梯子测试失败而 cobweb/sweet_berry_bush
// 通过，说明 handleOnClimbable resetFallDistance 缺失（攀爬主机制缺陷）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_梯子.txt（梯子可攀爬，攀爬时不承受摔落伤害）
// Ref: LivingEntity.cpp:1876（travel 攀爬分支 onLadder resetFallDistance，对齐 handleOnClimbable）
// Ref: LadderBlock.cpp isLadder（梯子 isLadder 返回 true）+ getCollisionShape=empty（不阻挡下落）
// Ref: LivingEntity.java:2570-2572（vanilla handleOnClimbable 第一行 resetFallDistance）
function ladderClimbResetsFallDistanceNoDamage(test: Test): void {
  const pigType = "pig";

  // (3,0,3) 放 cobblestone 作猪落地实方块（fall_tower y=0 中心格默认非固体）。
  test.setBlockType("minecraft:cobblestone", { x: 3, y: 0, z: 3 });

  // (3,1..10,3) 放梯子柱（facing=east 强放）。setBlockWithStates 不经 isValidPosition，背面 glass
  // 不变不触发自毁。梯子 getCollisionShape=empty 不阻挡猪下落，isLadder=true 触发攀爬物理。
  for (let y = 1; y <= 10; y++) {
    test.setBlockWithStates("minecraft:ladder", { x: 3, y, z: 3 }, "facing=east");
  }

  // 猪 spawn 于 (3,11,3)，落入梯子柱顶部，沿梯子缓慢下滑（onLadder=true 攀爬物理 + resetFallDistance）
  // 到 (3,0,3) cobblestone 顶面。
  test.spawn(pigType, { x: 3, y: 11, z: 3 });

  // 200 tick 后检查：猪存在且 hp===10（满血，沿梯子下滑 resetFallDistance 致未摔伤）。
  // 时序：梯子下滑极慢（LADDER_SPEED_MAX 限速，实测约 8 格/100tick），10 格下滑需约 125 tick +
  // 落地余量，tick 200 留足余量确保猪已落到 (3,0,3) cobblestone 顶面（实测 t=200 猪已落地 hp=10）。
  // 此前用 t=100 检查致假阳性——猪 t=100 仅下滑到 helper-y≈3 未落地，hp=10 是"没落地"非"免疫"。
  // 若未重置 fallDistance，落差 10 格应承受 (10-3)*1=7 伤害（hp=3），hp===10 精确证明免疫。
  test.runAtTickTime(200, () => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(TOWER_FROM),
      volume: TOWER_VOLUME,
    });
    test.assert(pigs.length > 0, "pig disappeared before fall damage check");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue === 10,
      `ladder climb did not reset fall distance (expected hp=10 no fall damage, got hp=${(health as any).currentValue};`
        + ` hp<10 means pig took fall damage sliding down ladder — fallDistance was not reset by handleOnClimbable in travel climb branch)`);
    test.succeed();
  });
}

// 对照测试：猪从同样高度直接落到 cobblestone（无梯子）承受完整摔落伤害（血量大降）。
// 验证落差 10 格确会造成重伤，使梯子免疫判定有意义（非"落差不足本就不受伤"的假阳性）。
function ladderAbsentDealsFallDamage(test: Test): void {
  const pigType = "pig";

  // (3,0,3) 放 cobblestone（普通方块，走 Block::onFallenUpon 默认 multiplier=1.0 完整摔落伤害）。
  test.setBlockType("minecraft:cobblestone", { x: 3, y: 0, z: 3 });

  // (3,1,3) 保持 air（无梯子），猪直接落到 cobblestone 承受完整摔落伤害。
  test.setBlockType("minecraft:air", { x: 3, y: 1, z: 3 });

  // 猪 spawn 于 (3,11,3)，自由落体到 cobblestone 顶面，落差 10 格，承受 (10-3)*1=7 摔落伤害。
  test.spawn(pigType, { x: 3, y: 11, z: 3 });

  // 断言猪承受重伤：runAtTickTime(60) 检查 hp<8。cobblestone 伤害 7（hp=3 < 8）。
  // 梯子免疫 hp=10 > 8，故 hp<8 证明落差足以重伤，梯子免疫有意义（非落差不足假阳性）。
  test.runAtTickTime(60, () => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(TOWER_FROM),
      volume: TOWER_VOLUME,
    });
    test.assert(pigs.length > 0, "pig disappeared before fall damage check");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue < 8,
      `pig should take heavy fall damage without ladder (expected hp<8, got hp=${(health as any).currentValue};`
        + ` hp>=8 means fall distance was insufficient — ladder immunity test above would be a false positive)`);
    test.succeed();
  });
}

export function registerLadderTests(): void {
  GameTest.register("BlockBehaviorTests", "ladder_breaks_when_attached_block_removed", ladderBreaksWhenAttachedBlockRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
  // 梯子攀爬摔伤免疫测试用 fall_tower（垂直下落场景）。
  GameTest.register("BlockBehaviorTests", "ladder_climb_resets_fall_distance_no_damage", ladderClimbResetsFallDistanceNoDamage)
        .structureName("gametests:fall_tower")
        .maxTicks(300);
  GameTest.register("BlockBehaviorTests", "ladder_absent_deals_fall_damage", ladderAbsentDealsFallDamage)
        .structureName("gametests:fall_tower")
        .maxTicks(200);
}
