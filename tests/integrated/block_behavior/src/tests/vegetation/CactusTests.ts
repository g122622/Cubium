// 植被类方块行为 GameTest（仙人掌等）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底（满铺 49 glass），y=1..2 为玻璃墙围出的内部 air 空腔，y=3..4 air+顶部框架。
// 方块测试在内部 air 层操作，需特定支撑时显式 setBlockType 覆盖玻璃底。

// 仙人掌水平相邻出现固体方块时自毁变 air（wiki tech_仙人掌.txt#种植：仙人掌水平方向不能有
// 固体方块，旁边放方块时仙人掌被破坏）。
//
// C++ 链路：CactusBlock::updatePostPlacement（CactusBlock.cpp:130-163）当 facing 为水平方向
// （非 Up/Down）且 facingState.getMaterial().isSolid() 为真时，返回 airState（自毁）。放仙人掌
// 自身不立即自毁（CactusBlock 无 onBlockAdded 重写，放置不检查邻居）。需"第二步在相邻水平格放
// 固体方块"触发仙人掌 updatePostPlacement → 返回 air → ServerWorld 将仙人掌格替换为 air。
// ServerWorld 邻居更新（ServerWorld.cpp:884-889）对仙人掌格调 updatePostPlacement(opposite(方向),
// facingState=stone)，facing 为水平方向命中自毁分支。反应同 tick 同步（updatePostPlacement 直接
// 返回 air，ServerWorld 立即 setBlockState）。
//
// 关键约束：仙人掌 isValidPosition 要求下方为 sand/red_sand/仙人掌（CactusBlock.cpp:112-113）。
// GameTestHelper.setBlock 直写不经 isValidPosition，故即使下方非沙也能强放；但为贴近 vanilla 语义
// 且排除下方支撑缺失导致的 Down 分支自毁干扰，先在 (3,0,1) 显式铺 sand 作支撑。(3,1,1) 的 North
// 邻居 (3,1,0) 是 glass_pit 玻璃墙（结构固有，不变化，不触发仙人掌 North 方向 updatePostPlacement），
// 故不影响；East/South/West 邻居为 air，放 stone 在 East 触发自毁。
//
// 判定手段：先 (3,0,1) 铺 sand，(3,1,1) 放 cactus，再 (4,1,1) 放 stone（East 相邻固体）。stone
// 放置触发仙人掌 updatePostPlacement → 返回 air。succeedWhenBlockPresent 断言仙人掌格 (3,1,1)
// 仙人掌消失（同 tick 同步成立）。
// 注意：仙人掌自毁掉落物品依赖 onBlockRemoved/掉落系统，本测试仅断言方块变 air（核心行为），
// 掉落物未断言。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_仙人掌.txt#种植（水平相邻固体方块破坏仙人掌）
function cactusBreaksWhenSolidBlockAdjacent(test: Test): void {
  // (3,0,1) 铺 sand 作仙人掌下方支撑（isValidPosition 要求下方 sand/red_sand/仙人掌）。
  test.setBlockType("minecraft:sand", { x: 3, y: 0, z: 1 });

  // (3,1,1) 放仙人掌（在 sand 上）。放置自身不立即自毁（无 onBlockAdded 重写）。
  test.setBlockType("minecraft:cactus", { x: 3, y: 1, z: 1 });

  // (4,1,1) 放 stone（仙人掌 East 相邻固体方块，Material::ROCK isSolid=true）。stone 放置向
  // 仙人掌格派发 updatePostPlacement(水平方向) → facingState=stone solid → 返回 air，仙人掌自毁。
  test.setBlockType("minecraft:stone", { x: 4, y: 1, z: 1 });

  // 断言仙人掌格 (3,1,1) 仙人掌已自毁消失（同 tick 同步）。
  test.succeedWhenBlockPresent("minecraft:cactus", { x: 3, y: 1, z: 1 }, false);
}

export function registerCactusTests(): void {
  GameTest.register("BlockBehaviorTests", "cactus_breaks_when_solid_block_adjacent", cactusBreaksWhenSolidBlockAdjacent)
    .structureName("gametests:glass_pit")
    .maxTicks(60);
}
