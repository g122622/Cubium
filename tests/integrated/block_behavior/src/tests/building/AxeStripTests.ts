// 斧头原木剥皮行为 GameTest。
//
// wiki tech_斧.txt#去皮：对木头、原木或竹块使用斧可以使其变成去皮变种（stripped）。每次去皮消耗斧
//   1 点耐久度。去皮保留原木的朝向属性（axis，原木平放/竖放朝向不变）。
//
// C++ 链路：AxeItem::onItemUse（item/items/tool/AxeItem.cpp:57）——物品侧 onItemUse（非 onBlockActivated）。
//   - 交互顺序 1.去皮 → 2.去氧化(刮削铜) → 3.除蜡，每步独立检查，首个匹配执行。
//   - 1.去皮：getStrippedBlock(original) 查 _getStrippingMap（oak_log→stripped_oak_log 等）→
//     strippedBlock.getDefaultState().withPropertiesOf(*state)（保留 axis 等共有属性）+ 剥皮音效 +
//     setBlockState + 消耗耐久 → Success。映射外方块跳过去皮进下一步。
//   - 去皮原木 minecraft:stripped_oak_log（RotatedPillarBlock，有 AXIS state）。
//
// 派发链路：原木无 onBlockActivated override（基类返 Pass），useItemOnBlock ① Block.use 前置分支 Pass →
//   ② fallback Item.useOn（AxeItem.onItemUse）剥皮。手持斧头（非 BlockItem），fallback 不放方块，走剥皮转换。
//
// 测试覆盖（1 个场景，覆盖 wiki 斧去皮核心确定行为）：
//   1. 斧剥橡木原木：放橡木原木 + 钻石斧 useItemOnBlock → 原位 oak_log→stripped_oak_log，返 true。
//
// 关键约束：
// 1. 橡木原木完整方块（Material::WOOD），无 canSurvive 自毁，放 (3,2,1)（minecraft:oak_log）无需支撑。
// 2. 钻石斧用 new ItemStack("minecraft:diamond_axe", 1)（耐久 1561，创造模式 hurtAndBreak 不消耗耐久）。
// 3. useItemOnBlock 传 Direction.Up（点击顶面），AxeItem.onItemUse 不检查 face（任意面可剥皮）。
// 4. 判定原位 block.typeId 转换：oak_log → stripped_oak_log。withPropertiesOf 保留 axis，但默认原木
//   axis=y，去皮后仍 axis=y，本组只断言类型转换不断言 axis（默认即可）。
// 5. 斧头走 fallback 分支，useItemOnBlock 成功后对选中槽 shrink(1)（SimulatedPlayer.cpp:331-337），
//   但创造模式不消耗（isCreative 守卫）。
//
// 不测「去皮保留 axis 朝向」：需用 setBlockWithStates 放 axis=x 原木再剥皮断言 axis 保留，构造稍繁，
//   且 axis 保留逻辑由 withPropertiesOf 保证，本组聚焦类型转换，跳过。TODO: 待需要时补。
// 不测「木头(六面 bark)去皮」：oak_wood→stripped_oak_wood 同逻辑，本组测原木已覆盖去皮行为点，跳过。
// 不测「斧去氧化铜」：需氧化铜方块 + IOxidizableBlock，1.21 铜变化大且非去皮主路径，跳过。
//   TODO: 待铜氧化链路测试完善后补。
// 不测「斧除蜡」：需涂蜡铜方块，跳过。TODO: 待涂蜡铜测试完善后补。
// 不测「斧耐久消耗」：创造模式不消耗，跳过。
//
// 跨服务端：橡木原木 oak_log / 去皮原木 stripped_oak_log 方块名两端一致，斧头剥皮转换行为与 vanilla
//   一致。本组用 setBlockType 放默认原木（无需 setBlockWithStates），两端均可放；斧剥皮行为两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_斧.txt#去皮（对原木/木头使用斧变去皮变种，消耗1耐久）
// Ref: AxeItem.cpp（onItemUse 1.去皮 getStrippedBlock+withPropertiesOf+音效+耐久→Success）
// Ref: SimulatedPlayer.cpp:314-343（useItemOnBlock fallback Item.useOn，原木无 onBlockActivated 故走此分支）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 橡木原木 (3,2,1)，完整方块无需支撑。

// 读取 (x,y,z) 方块 typeId。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 场景 1：斧剥橡木原木——放橡木原木 + 钻石斧 useItemOnBlock → 原位 oak_log→stripped_oak_log，返 true。
//
// 布局：(3,2,1) 橡木原木（minecraft:oak_log，默认 axis=y）。
// useItemOnBlock ① onBlockActivated（oak_log 基类 Pass）→ ② fallback AxeItem.onItemUse：
//   getStrippedBlock(oak_log)=STRIPPED_OAK_LOG + withPropertiesOf 保留 axis → setBlockState
//   stripped_oak_log + 剥皮音效 + 耐久 → Success。
//
// 判定：useItemOnBlock 返 true（Success），原位 (3,2,1) typeId === "minecraft:stripped_oak_log"。
function axeStripsOakLog(test: Test): void {
    test.setBlockType("minecraft:oak_log", { x: 3, y: 2, z: 1 }); // 橡木原木
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:oak_log", `oak_log should be at (3,2,1) before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const axe = new ItemStack("minecraft:diamond_axe", 1);

    // 对橡木原木 useItemOnBlock 钻石斧 → fallback AxeItem.onItemUse oak_log→stripped_oak_log → Success。
    const used = farmer.useItemOnBlock(
        axe as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when stripping oak log with axe");

    // 判定：原位 (3,2,1) 变 stripped_oak_log（橡木原木斧剥皮为去皮橡木原木）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:stripped_oak_log", `stripped_oak_log should be at (3,2,1) after stripping, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerAxeStripTests(): void {
    GameTest.register("BlockBehaviorTests", "axe_strips_oak_log", axeStripsOakLog)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
