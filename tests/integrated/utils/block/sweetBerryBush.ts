// 甜浆果灌木跨服务端放置工具：在 Cubium 与官方基岩 BDS 两端统一放置指定生长阶段的甜浆果灌木。
//
// 背景：甜浆果灌木的"生长阶段"block state 在两端命名不同——
//   - Cubium（对齐 Java 1.21.11）：state 名 `age`，值域 0-3（BlockStateProperties::AGE_0_3）。
//   - 官方基岩 BDS：state 名 `growth`，值域 0-3（wiki Bedrock Edition data values：growth 0=幼苗/
//     1=成熟无果/2=有果/3=满果，与 Java age 0-3 一一对应，Metadata Bits 0x1）。
// 两端值域与语义完全一致（age/growth≥1 即"生长阶段至少为2"，移动实体受伤），仅 state 名不同。
// 此外，放置带 state 方块的 API 两端也不同：
//   - Cubium：Test.setBlockWithStates(blockType, pos, "age=N" statesStr, updateFlags)（Cubium C++ 专有扩展，
//     GameTestHelper.cpp:346，基岩 BDS 无此方法）。
//   - 基岩：Test.setBlockPermutation(BlockPermutation.resolve(blockType, { growth: N }), pos)（官方通用 API）。
//
// 故本工具运行时检测平台（Cubium 有 setBlockWithStates，基岩无），分别用各自 API + 各自 state 名放置，
// 使同一份 TS 测试代码在两端都能正确放置指定阶段的甜浆果灌木，支撑基岩对比（run_diff.ts）。
//
// 平台检测依据：setBlockWithStates 是 Cubium 专有方法，基岩 BDS 的 Test 无此方法（typeof === "undefined"）。
// 这与 gametest-shim.ts 用 RegistrationBuilder.skyAccess 检测平台的思路一致。
//
// 类型注记：BlockPermutation 从 @minecraft/server 命名导入。注意 node_modules 中 @minecraft/server
// (1.13.0-beta) 与 @minecraft/server-gametest 内嵌的 @minecraft/server (1.19.0) 是两份类型声明，
// Test.setBlockPermutation 期望后者版本的 BlockPermutation，直接传会触发 TS2345 版本冲突。故基岩分支
// 用 any 绕过编译期类型边界（运行时两端都是同一 BlockPermutation 运行时对象，无版本差异）。Cubium 分支
// 不访问 BlockPermutation.resolve（Cubium 未实现该静态方法），仅基岩分支调用（基岩原生支持）。

import type { Test } from "@minecraft/server-gametest";
import { BlockPermutation } from "@minecraft/server";
import type { Vector3 } from "@minecraft/server";

/**
 * 在指定 helper 相对坐标放置指定生长阶段的甜浆果灌木。
 *
 * @param test GameTest Test 对象
 * @param pos helper 相对坐标
 * @param age 生长阶段（0-3，两端值域一致；age/growth≥1 的灌木对移动实体造成伤害）
 */
export function setSweetBerryBush(test: Test, pos: Vector3, age: number): void {
  const blockType = "minecraft:sweet_berry_bush";
  // 平台检测：Cubium 的 Test 有 setBlockWithStates 专有方法，基岩 BDS 无。
  const hasSetBlockWithStates = typeof (test as unknown as {
    setBlockWithStates?: unknown;
  }).setBlockWithStates === "function";
  if (hasSetBlockWithStates) {
    // Cubium：用 age（对齐 Java），setBlockWithStates 接受 "age=N" statesStr + updateFlags=3（含邻居更新）。
    (test as unknown as {
      setBlockWithStates: (blockType: string, pos: Vector3, statesStr: string, updateFlags: number) => void;
    }).setBlockWithStates(blockType, pos, `age=${age}`, 3);
  } else {
    // 基岩 BDS：用 growth（基岩 state 名，值域 0-3 与 Java age 一致），官方 setBlockPermutation + resolve。
    // any 绕过 @minecraft/server 两版本 BlockPermutation 类型冲突（见文件头注释）。
    const permutation = BlockPermutation.resolve(blockType, { growth: age }) as unknown;
    (test as unknown as {
      setBlockPermutation: (blockData: unknown, blockLocation: Vector3) => void;
    }).setBlockPermutation(permutation, pos);
  }
}
