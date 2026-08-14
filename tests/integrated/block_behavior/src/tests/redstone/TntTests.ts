// 红石类方块行为 GameTest（TNT 等）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底（满铺 49 glass），y=1..2 为玻璃墙围出的内部 air 空腔，y=3..4 air+顶部框架。
// 方块测试在内部 air 层操作。

// TNT 相邻熔岩时被点燃激活，原方块消失并生成被激活的 TNT 实体（wiki tech_TNT.txt#用途：TNT 被火烧毁
// 时会被激活）。
//
// C++ 链路：TNTBlock::onBlockAdded（TNTBlock.cpp:91-103）放置后检查 hasPower（红石信号）或
// hasFire（_hasFlammableNeighbor，:394-415 检查 6 向邻居是否为 fire/soul_fire/lava）。满足任一则
// prime(world, pos, nullptr)（:249-318）：受 tntExplodes 游戏规则（默认 true）守卫，通过 EntityType
// 工厂创建 TNTEntity，设置位置/随机速度/引信(80 tick)后 spawnEntity，并 playSound + 发 PRIME_FUSE
// 游戏事件。prime 成功后 onBlockAdded 调 world.setBlockState(pos, nullptr, 11) 将 TNT 方块移除。
// 反应同 tick 同步（onBlockAdded 在 setBlockState(tnt) 内部触发，prime + 移除方块均在同一调用栈完成）。
//
// 关键约束：
// 1. 放 TNT 自身即触发 onBlockAdded（air→tnt 是真实状态变化，非 no-op），故无需"第二步"触发——
//    只需 TNT 放置时已存在相邻熔岩/火即可。先放熔岩源，再放 TNT。
// 2. 触发源选熔岩源（minecraft:lava）而非火（minecraft:fire）：熔岩源是稳定液体方块，放置后不会立即
//    消失（火走 randomTick 消亡，放置稳定性差）；熔岩流动是 scheduledTick 下一 tick 才发生，同 tick
//    内熔岩源存在于原格，TNT 放置时 _hasFlammableNeighbor 能稳定检测到。
// 3. tntExplodes 游戏规则默认 true（GameRules.cpp:130 registerBoolean("tntExplodes", ..., true)），
//    prime 会真正执行。Cubium --gametest 默认不改该规则。
//
// 判定手段：先 (3,1,1) 放熔岩源，再 (3,1,2) 放 TNT（South 邻居为熔岩）。TNT onBlockAdded 同步检测
// 熔岩 → prime 生成 TNT 实体 + setBlockState(null) 移除方块。succeedWhenBlockPresent 断言 TNT 格
// (3,1,2) TNT 消失（prime 成功的同步结果）。maxTicks=30 远小于引信 80 tick，确保爆炸在测试断言后
// 才发生，不干扰本测试判定。
// 注意：TNT 实体生成后约 80 tick 爆炸，爆炸可能波及 glass_pit 局部（爆炸半径约 4），但不影响本测试
// 已完成的方块消失断言；掉落物/爆炸破坏未断言（依赖爆炸系统，非本测试核心）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_TNT.txt#用途（被火烧毁激活）
function tntPrimesWhenLavaAdjacent(test: Test): void {
  // (3,1,1) 放熔岩源（稳定液体方块，同 tick 内存在于原格，作 TNT 点燃源）。
  test.setBlockType("minecraft:lava", { x: 3, y: 1, z: 1 });

  // (3,1,2) 放 TNT（South 邻居 (3,1,1) 为熔岩）。air→tnt 真实状态变化触发 onBlockAdded →
  // _hasFlammableNeighbor 检测到熔岩 → prime 生成 TNT 实体 + setBlockState(null) 移除 TNT 方块。
  test.setBlockType("minecraft:tnt", { x: 3, y: 1, z: 2 });

  // 断言 TNT 格 (3,1,2) TNT 已被点燃消失（同 tick 同步）。maxTicks=30 < 引信 80，爆炸在断言后发生。
  test.succeedWhenBlockPresent("minecraft:tnt", { x: 3, y: 1, z: 2 }, false);
}

export function registerTntTests(): void {
  GameTest.register("BlockBehaviorTests", "tnt_primes_when_lava_adjacent", tntPrimesWhenLavaAdjacent)
    .structureName("gametests:glass_pit")
    .maxTicks(30);
}
