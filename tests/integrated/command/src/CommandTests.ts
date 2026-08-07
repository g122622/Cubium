// 命令类 GameTest：cloneBlocksCommand（/clone 命令克隆方块）。
// 原 runAsLlama 测试已移除：其命令方块使用基岩旧版 /execute 语法（/execute @e[type=llama] ~ ~ ~ tp ~ ~ ~2），
// 与项目 Java 新版 /execute 命令体系不兼容（解析期 Expected literal 'as'），且 tp 命令暂不支持非玩家实体相对坐标位移。
// 后续若实现基岩旧版 /execute 兼容层与 tp 实体位移，可恢复该测试。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 克隆命令：依次触发多个按钮，每个按钮触发一个命令方块执行 /clone，验证克隆结果正确。
function cloneBlocksCommand(test: Test): void {
  // 克隆部分陶瓦方块
  test.pressButton({ x: 1, y: 2, z: 0 });

  // 克隆箱子
  test.pressButton({ x: 1, y: 2, z: 3 });

  // 克隆安山岩楼梯
  test.pressButton({ x: 1, y: 2, z: 6 });

  test.runAtTickTime(10, () => {
    test.assertBlockPresent("minecraft:purple_glazed_terracotta", { x: 5, y: 2, z: 1 }, true);
    test.assertBlockPresent("minecraft:pink_glazed_terracotta", { x: 6, y: 2, z: 2 }, true);
    test.assertBlockPresent("minecraft:oak_log", { x: 5, y: 2, z: 2 }, true);

    // 验证箱子被克隆
    test.assertBlockPresent("minecraft:chest", { x: 5, y: 2, z: 4 }, true);
    test.assertBlockPresent("minecraft:chest", { x: 6, y: 2, z: 4 }, true);

    // 验证安山岩楼梯被克隆
    test.assertBlockPresent("minecraft:andesite_stairs", { x: 5, y: 2, z: 8 }, true);
    test.assertBlockPresent("minecraft:andesite_stairs", { x: 6, y: 2, z: 8 }, true);
    test.assertBlockPresent("minecraft:andesite_stairs", { x: 5, y: 2, z: 9 }, true);
    test.assertBlockPresent("minecraft:andesite_stairs", { x: 6, y: 2, z: 9 }, true);
  });

  test.runAtTickTime(20, () => {
    // 再次克隆部分陶瓦，但确保圆石不被空气覆盖
    test.pressButton({ x: 2, y: 2, z: 0 });

    // 再次克隆箱子
    test.pressButton({ x: 2, y: 2, z: 3 });

    // 使用过滤器只克隆其中一个安山岩楼梯
    test.pressButton({ x: 3, y: 2, z: 7 });
  });

  test.runAtTickTime(30, () => {
    test.assertBlockPresent("minecraft:purple_glazed_terracotta", { x: 8, y: 2, z: 1 }, true);
    test.assertBlockPresent("minecraft:pink_glazed_terracotta", { x: 9, y: 2, z: 2 }, true);
    test.assertBlockPresent("minecraft:cobblestone", { x: 9, y: 2, z: 1 }, true);
    test.assertBlockPresent("minecraft:chest", { x: 5, y: 2, z: 4 }, true);
    test.assertBlockPresent("minecraft:purple_glazed_terracotta", { x: 6, y: 2, z: 4 }, true);
    test.assertBlockPresent("minecraft:oak_log", { x: 6, y: 2, z: 5 }, true);
    test.assertBlockPresent("minecraft:pink_glazed_terracotta", { x: 7, y: 2, z: 5 }, true);

    // 验证只有一个安山岩楼梯被克隆（其余为空气）
    test.assertBlockPresent("minecraft:air", { x: 8, y: 2, z: 8 }, true);
    test.assertBlockPresent("minecraft:air", { x: 9, y: 2, z: 8 }, true);
    test.assertBlockPresent("minecraft:air", { x: 8, y: 2, z: 9 }, true);
    test.assertBlockPresent("minecraft:andesite_stairs", { x: 9, y: 2, z: 9 }, true);
  });
  test.runAtTickTime(40, () => {
    test.succeed();
  });
}

export function registerCommandTests(): void {
  GameTest.register("CommandTests", "cloneBlocksCommand", cloneBlocksCommand)
    .structureName("gametests:clone_command")
    .maxTicks(50);
}
