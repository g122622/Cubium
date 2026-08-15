// 骨粉催熟作物行为类 GameTest。
//
// 本测试验证 SimulatedPlayer::useItemOnBlock 真实派发链路（任务 #186）：SimulatedPlayer 持骨粉对
// 小麦方块使用 → 经 ItemUseContext + BoneMealItem::onItemUse → dynamic_cast<IGrowable> → canGrow
// （未成熟 true）→ 确定性随机 canUseBonemeal（作物恒 true）→ CropBlock::grow（getAge +
// getBonemealAgeIncrease 返回 2-5）→ setBlockState 写回新 age → shrink(1) 消耗骨粉 → 返回 Success。
//
// 判定链路：test.getBlock(wheatPos).permutation.getState("age")。getBlock 经 wrapBlock 包装 Block
// JS 对象（ScriptBlockRef 快照），permutation 属性包装 BlockPermutation，getState("age") 经
// StateHolder::values() + IProperty::valueToString + typeName 调度取 IntegerProperty 数值。
//
// wiki 行为（docs\minecraft-wiki-source\minecraft_wiki\tech_骨粉.txt#催熟）：
//   小麦使用骨粉"使之生长 2-5 个生长阶段"。小麦共 8 个生长阶段（age 0-7，AGE_0_7）。
//   初始 age=0，骨粉后 age ∈ [2,5]（2-5 增量，未达 maxAge=7 不钳制）。
//   基岩创造模式对作物直接成熟（age=7），但 SimulatedPlayer 默认生存模式走标准 2-5 增量。
//
// C++ 链路（CropBlock.cpp:182-211）：grow 用 getAge(state) + getBonemealAgeIncrease(world, pos)
//   （= 2 + random.nextInt(4)，确定性 seed=world.seed()^hash(pos)），min(newAge, maxAge=7) 钳制，
//   setBlockState(pos, &withAge(newAge), 2) 同步写回。canUseBonemeal 恒 true（作物骨粉 100% 即时催熟，
//   非随机概率跳过）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { BlockPermutation, ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 骨粉催熟小麦：初始 age=0 的小麦经骨粉 useItemOnBlock 后 age 增长 2-5（≥2）。
//
// 布局：(3,1,1) 放耕地（farmland）支撑，(3,2,1) 放 age=0 小麦。SimulatedPlayer 持骨粉对 (3,2,1)
// 用 useItemOnBlock（direction=Up，faceLocation=方块中心）。骨粉经 BoneMealItem::onItemUse →
// CropBlock::grow 同步写回新 age。
//
// 判定：getBlock((3,2,1)).permutation.getState("age") 应 ≥2（初始 0 + 2-5 增量）。
// getState 经 Cubium BlockPermutation.getState 绑定（MinecraftModuleFactory.cpp）取 IntegerProperty
// 数值。骨粉 grow 是同步 setBlockState（flags=2），useItemOnBlock 返回后即可读，无需轮询。
//
// 跨服务端：本测试核心验证 Cubium 端 useItemOnBlock 派发 + getBlock/getState 判定链路。基岩 BDS 端
// getBlock/permutation.getState 行为可能差异（基岩 BlockPermutation API），故本测试以 Cubium 验证
// 为主。getState("age") 在两端小麦 state 名均为 age（不像甜浆果 age/growth 分歧），值域一致。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_骨粉.txt#催熟（小麦骨粉生长 2-5 阶段）
function bonemealGrowsWheat(test: Test): void {
  // 耕地 (3,1,1) 支撑小麦（小麦需在耕地上方，canSustain 检查）。setBlockType 直写 defaultState
  // （moisture=0），放置本身不触发退化（退化靠上方固体，耕地无上方固体）。
  test.setBlockType("minecraft:farmland", { x: 3, y: 1, z: 1 });

  // 小麦 (3,2,1) age=0。用 BlockPermutation.resolve + setBlockPermutation 放带 state 方块
  // （setBlockType 只放 defaultState，小麦 defaultState 已是 age=0，但显式 resolve 确保初值明确）。
  // any 绕过 @minecraft/server 两版本 BlockPermutation 类型冲突（见 sweetBerryBush.ts 注释）。
  const wheatPerm = BlockPermutation.resolve("minecraft:wheat", { age: 0 }) as any;
  (test as unknown as {
    setBlockPermutation: (blockData: unknown, blockLocation: { x: number; y: number; z: number }) => void;
  }).setBlockPermutation(wheatPerm, { x: 3, y: 2, z: 1 });

  // SimulatedPlayer 持骨粉。gameMode 省略走 Cubium 默认创造模式——创造模式下 useItemOnBlock 内部
  // onItemUse 仍正常催熟（grow 与模式无关），仅消耗语义跳过（创造不 shrink）。催熟判定不依赖消耗。
  const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");

  // 骨粉物品（Cubium new ItemStack(typeId, amount) 已实现）。
  const boneMeal = new ItemStack("minecraft:bone_meal", 1);

  // 对小麦方块 (3,2,1) 使用骨粉。direction=Up（从上方使用），faceLocation=方块中心（默认）。
  // useItemOnBlock 内部构造 ItemUseContext → BoneMealItem::onItemUse → CropBlock::grow 同步写回。
  // blockLocation 为结构相对坐标，SimulatedPlayer 内部经 worldBlockPosition 转世界绝对坐标。
  // 返回 true 表示物品被使用（onItemUse 返回 Success/Consume）。
  const used = farmer.useItemOnBlock(
    boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
    { x: 3, y: 2, z: 1 },
    Direction.Up,
  );

  // useItemOnBlock 应返回 true（骨粉成功催熟）。
  test.assert(used, "useItemOnBlock should return true when bone meal successfully grows wheat");

  // 判定：小麦 age 应 ≥2（初始 0 + 2-5 增量）。grow 是同步 setBlockState，useItemOnBlock 返回后
  // 立即可读。getBlock 经 wrapBlock 包装 Block JS 对象，permutation.getState("age") 取 IntegerProperty 数值。
  const block = test.getBlock({ x: 3, y: 2, z: 1 });
  test.assert(block !== undefined, "getBlock should return a Block object for the wheat position");
  const age = block?.permutation?.getState("age");
  test.assert(
    typeof age === "number" && age >= 2 && age <= 5,
    `wheat age should be in [2,5] after bone meal (got ${age})`,
  );

  test.succeed();
}

export function registerBoneMealTests(): void {
  GameTest.register("BlockBehaviorTests", "bonemeal_grows_wheat", bonemealGrowsWheat)
    .structureName("gametests:glass_pit")
    .maxTicks(60);
}
