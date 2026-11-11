// /clone 命令 GameTest：masked 复制、方块实体复制与 filtered 属性过滤。
//
// 旧实现依赖 Bedrock 结构内预置的按钮/红石线/命令方块（`^` 局部坐标 + 基岩 clone 语法）触发，
// 其供电链路与本项目 Java 版红石语义不一致，且 filtered 子句是基岩写法。命令类测试本就只在
// Cubium 侧运行（基岩 chat 不发命令），因此改为用 SimulatedPlayer.chat 直接执行 Java 1.21.11
// 语法命令，并复用 gametests:clone_command 结构作为 10×5×10 的空间容器。
//
// 覆盖点（与 tests/world/CloneCommandTests.ts 互补，不重复 replace/masked 的基础语义）：
//   1. 带方块实体的方块（chest）被完整复制。
//   2. masked 模式不复制 air，目标区域其余格保持原样。
//   3. filtered 按 Java 方块状态谓词精确过滤（andesite_stairs[facing=east,half=bottom]），
//      仅匹配的楼梯被复制。
//
// 坐标均为结构相对坐标（helper 相对坐标，placeOrigin 已抬高一格），经 worldLocation 转世界绝对
// 坐标后交给 /clone。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露，用 cast 访问，同 BellTests/EndRodTests）。
type TestWithStates = Test & {
  setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

const PLAYER_POS = { x: 1, y: 1, z: 1 };

function worldCoords(test: Test, rel: { x: number; y: number; z: number }): string {
  const w = test.worldLocation(rel);
  return `${Math.floor(w.x)} ${Math.floor(w.y)} ${Math.floor(w.z)}`;
}

function cloneCommand(
  test: Test,
  begin: { x: number; y: number; z: number },
  end: { x: number; y: number; z: number },
  destination: { x: number; y: number; z: number },
  suffix: string,
): string {
  return `/clone ${worldCoords(test, begin)} ${worldCoords(test, end)} ${worldCoords(test, destination)}${suffix === "" ? "" : " " + suffix}`;
}

// 场景：三组 clone 各验证一个语义点。
function cloneBlocksCommand(test: Test): void {
  const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

  // ---- 1. 方块实体复制：两个箱子复制到目标区域 ----
  test.setBlockType("minecraft:chest", { x: 2, y: 2, z: 4 });
  test.setBlockType("minecraft:chest", { x: 3, y: 2, z: 4 });
  player.chat(cloneCommand(test, { x: 2, y: 2, z: 4 }, { x: 3, y: 2, z: 4 }, { x: 5, y: 2, z: 4 }, ""));

  // ---- 2. masked 复制：源区域只有 3 格非空气，目标其余格保持空气 ----
  test.setBlockType("minecraft:purple_glazed_terracotta", { x: 2, y: 2, z: 2 });
  test.setBlockType("minecraft:oak_log", { x: 3, y: 2, z: 2 });
  test.setBlockType("minecraft:pink_glazed_terracotta", { x: 4, y: 3, z: 3 });
  // 断言用的空气目标格选源区空气格 (2,3,2)/(3,3,2) 的映射结果 (5,3,6)/(6,3,6)：
  // 结构模板在 helper y=2 层还残留旧基岩供电链的命令方块（如 (5,2,7)/(7,2,7)），
  // 那些格子本身不是空气，不能用来断言“masked 未覆盖”。
  player.chat(cloneCommand(test, { x: 2, y: 2, z: 2 }, { x: 4, y: 3, z: 3 }, { x: 5, y: 2, z: 6 }, "masked"));

  // ---- 3. filtered 属性过滤：4 个楼梯只有 facing=east 的那个被复制 ----
  // 源区域 (2,2,8)..(3,3,9)：两个朝东、两个朝西（half 均为 bottom）。
  (test as TestWithStates).setBlockWithStates("minecraft:andesite_stairs", { x: 2, y: 2, z: 8 }, "facing=east,half=bottom");
  (test as TestWithStates).setBlockWithStates("minecraft:andesite_stairs", { x: 3, y: 2, z: 8 }, "facing=west,half=bottom");
  (test as TestWithStates).setBlockWithStates("minecraft:andesite_stairs", { x: 2, y: 2, z: 9 }, "facing=west,half=bottom");
  (test as TestWithStates).setBlockWithStates("minecraft:andesite_stairs", { x: 3, y: 2, z: 9 }, "facing=east,half=bottom");
  player.chat(
    cloneCommand(
      test,
      { x: 2, y: 2, z: 8 },
      { x: 3, y: 3, z: 9 },
      { x: 6, y: 2, z: 8 },
      "filtered andesite_stairs[facing=east,half=bottom] normal",
    ),
  );

  // 命令同步执行，留 5 tick 余量。
  test.runAtTickTime(5, () => {
    // 1. 箱子复制到位（含方块实体，由 /clone 的方块实体复制路径覆盖）。
    test.assertBlockPresent("minecraft:chest", { x: 5, y: 2, z: 4 }, true);
    test.assertBlockPresent("minecraft:chest", { x: 6, y: 2, z: 4 }, true);

    // 2. masked：3 个非空气方块复制到对应偏移（dest 起点 (5,2,6)）：
    //    (2,2,2)→(5,2,6)、(3,2,2)→(6,2,6)、(4,3,3)→(7,3,7)。
    test.assertBlockPresent("minecraft:purple_glazed_terracotta", { x: 5, y: 2, z: 6 }, true);
    test.assertBlockPresent("minecraft:oak_log", { x: 6, y: 2, z: 6 }, true);
    test.assertBlockPresent("minecraft:pink_glazed_terracotta", { x: 7, y: 3, z: 7 }, true);
    // 源 air 格不覆盖目标：对应偏移格保持空气。
    test.assertBlockPresent("minecraft:air", { x: 5, y: 3, z: 6 }, true);
    test.assertBlockPresent("minecraft:air", { x: 6, y: 3, z: 6 }, true);

    // 3. filtered：只有 facing=east 的两个楼梯被复制到 (6,2,8) 与 (7,2,9)；
    //    facing=west 的对应偏移格 (7,2,8)/(6,2,9) 保持空气。
    test.assertBlockPresent("minecraft:andesite_stairs", { x: 6, y: 2, z: 8 }, true);
    test.assertBlockPresent("minecraft:andesite_stairs", { x: 7, y: 2, z: 9 }, true);
    test.assertBlockPresent("minecraft:air", { x: 7, y: 2, z: 8 }, true);
    test.assertBlockPresent("minecraft:air", { x: 6, y: 2, z: 9 }, true);

    test.succeed();
  });
}

export function registerCommandTests(): void {
  GameTest.register("CommandTests", "cloneBlocksCommand", cloneBlocksCommand)
    .structureName("gametests:clone_command")
    .maxTicks(50);
}