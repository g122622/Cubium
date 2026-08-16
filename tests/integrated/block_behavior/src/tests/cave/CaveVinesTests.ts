// 洞穴藤蔓右键采摘发光浆果行为 GameTest。
//
// wiki other_发光浆果.txt#（采集）：对着长出发光浆果的洞穴藤蔓（berries=true）按下使用键会掉落
//   1 个发光浆果，并使其变为未长出发光浆果的状态（berries→false）。空手/任意物品右键均可采摘
//   （不检查手持物）。未长浆果（berries=false）的藤蔓右键无反应（onBlockActivated 返 Pass）。
//
// C++ 链路：CaveVinesBlock（cave/CaveVinesBlock.cpp）有 AGE_0_25 + BERRIES state（默认 false）。
//   - onBlockActivated（:148）：BERRIES==true → 服务端掉落1发光浆果（ItemDropHelper）+ 采摘音效 +
//     with(BERRIES,false) setBlockState 写回 + gameEvent(BLOCK_CHANGE) → return Success。
//     BERRIES==false → return Pass。不检查手持物（空手/任意物品右键均采摘）。
//   - BERRIES state 名 "berries"（Properties.hpp BERRIES = BooleanProperty("berries")）。
//   - CaveVinesPlantBlock（茎部）同样 onBlockActivated 采摘，本组只测顶部 CaveVinesBlock（cave_vines）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 不 raycast（直接对传入 blockLocation 操作，见
//   SimulatedPlayer.cpp:265-312），noCollision 藤蔓可被指定 pos 点击。先 onBlockActivated（BERRIES=true
//   → Success 短路），不走 fallback。用 stick 触发（onBlockActivated 不检查手持物，stick 不被消耗，
//   仅占位 useItemOnBlock 的 ItemStack 形参）。
//
// 测试覆盖（2 个场景，覆盖 wiki 右键采摘 + 未长浆果无反应核心确定行为）：
//   1. 右键采摘浆果：放 berries=true 洞穴藤蔓 + stick useItemOnBlock → berries=false，返 true。
//   2. 未长浆果右键无反应：放 berries=false 洞穴藤蔓 + stick useItemOnBlock → berries 仍 false，
//      返 false（onBlockActivated Pass，stick onItemUse 无放置返 Fail/Pass）。
//
// 关键约束：
// 1. 洞穴藤蔓 noCollision 下垂植物，无 canSurvive 自毁（基类返 true），放 (3,2,1)
//   （minecraft:cave_vines）。setBlockType 只能放默认 berries=false，需测 berries=true 故用 Cubium
//   专有 setBlockWithStates 放 "berries=true"（弥补 setBlockType 不足）。
// 2. useItemOnBlock 不 raycast，直接对 blockLocation 操作，noCollision 藤蔓可测（与碰撞箱无关）。
// 3. 读 berries state 用 getState("berries" as any) 绕过 BlockStateSuperset 白名单。
// 4. onBlockActivated 不检查手持物，用 stick 触发（stick 不被消耗，可重复使用）。
// 5. 场景 2 berries=false：onBlockActivated 返 Pass → fallback stick.onItemUse（普通 Item 无放置逻辑
//   返 Fail/Pass）→ useItemOnBlock 返 false。判定返 false + berries 仍 false（未长浆果不采摘）。
//
// 不测「掉落发光浆果实体」：掉落物实体生成非确定（位置/时序），本组聚焦 berries state 变化，跳过。
//   TODO: 待掉落物断言 API 完善后补发光浆果数量测试。
// 不测「骨粉使藤蔓长浆果」：涉骨粉 onItemUse + 随机生长（1/9 概率），非确定，跳过。
// 不测「向下生长」：涉 randomTick 随机生长 + age 递增，非确定，跳过。
// 不测「CaveVinesPlantBlock 茎部采摘」：茎部 cave_vines_plant 同逻辑，本组测顶部 cave_vines 已覆盖
//   采摘行为点，茎部跳过。TODO: 待需要时补。
//
// 跨服务端：洞穴藤蔓 cave_vines 方块名两端一致，berries state 名两端一致，右键采摘 berries→false
//   行为与 vanilla 一致。但 setBlockWithStates 是 Cubium 专有（基岩 BDS Test 无此 API），场景 1（需
//   berries=true）为 one-sided（仅 Cubium 可跑）。场景 2（berries=false 默认）两端均可放，可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_发光浆果.txt#采集（右键长浆果藤蔓掉1浆果+berries→false）
// Ref: CaveVinesBlock.cpp（onBlockActivated BERRIES==true 掉浆果+setBlockState→Success；BERRIES state）
// Ref: cubium-gametest-augment.d.ts（setBlockWithStates Cubium 专有，放带 state 方块）
// Ref: SimulatedPlayer.cpp:265-312（useItemOnBlock 不 raycast，直接对 blockLocation 操作）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 洞穴藤蔓 (3,2,1)，noCollision 下垂植物无需支撑。

// 读取洞穴藤蔓 berries state（boolean）。返回 null 表示读取失败或非洞穴藤蔓。
// berries state 名 "berries"（Java 命名，见 Properties.hpp BERRIES = BooleanProperty("berries")）。
function getCaveVinesBerries(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("berries" as any);
    return typeof value === "boolean" ? value : null;
}

// 放长浆果洞穴藤蔓：(3,2,1) cave_vines berries=true（用 Cubium 专有 setBlockWithStates 放带 state 方块）。
function placeBerriedCaveVines(test: Test): void {
    (test as unknown as {
        setBlockWithStates(blockType: string, loc: { x: number; y: number; z: number }, statesStr: string): void;
    }).setBlockWithStates("minecraft:cave_vines", { x: 3, y: 2, z: 1 }, "berries=true");
}

// 放未长浆果洞穴藤蔓：(3,2,1) cave_vines 默认 berries=false（setBlockType 即可）。
function placeBareCaveVines(test: Test): void {
    test.setBlockType("minecraft:cave_vines", { x: 3, y: 2, z: 1 }); // 默认 berries=false
}

// 场景 1：右键采摘浆果——放 berries=true 洞穴藤蔓 + stick useItemOnBlock → berries=false，返 true。
//
// 布局：(3,2,1) cave_vines berries=true。
// onBlockActivated：BERRIES==true → 服务端掉落1发光浆果 + 音效 + with(BERRIES,false) setBlockState +
//   gameEvent → Success（短路，不走 fallback）。
//
// 判定：useItemOnBlock 返 true（Success），berries === false（采摘后变未长浆果）。
function caveVinesPickBerriesOnUse(test: Test): void {
    placeBerriedCaveVines(test);
    test.assert(getCaveVinesBerries(test, 3, 2, 1) === true, `cave vines berries should be true before, got ${getCaveVinesBerries(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对洞穴藤蔓 useItemOnBlock stick → onBlockActivated BERRIES==true 掉浆果+berries→false → Success。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when picking berries from cave vines");

    // 判定：berries === false（采摘后变未长浆果状态）。
    test.assert(getCaveVinesBerries(test, 3, 2, 1) === false, `cave vines berries should be false after picking, got ${getCaveVinesBerries(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：未长浆果右键无反应——放 berries=false 洞穴藤蔓 + stick useItemOnBlock → berries 仍 false，返 false。
//
// 布局：(3,2,1) cave_vines berries=false（默认）。
// onBlockActivated：BERRIES==false → return Pass。fallback stick.onItemUse（普通 Item 无放置逻辑）
//   → Fail/Pass → useItemOnBlock 返 false。
//
// 判定：useItemOnBlock 返 false（无反应），berries 仍 === false（未长浆果不采摘）。
function caveVinesBareDoesNotReactOnUse(test: Test): void {
    placeBareCaveVines(test);
    test.assert(getCaveVinesBerries(test, 3, 2, 1) === false, `cave vines berries should be false before, got ${getCaveVinesBerries(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对未长浆果洞穴藤蔓 useItemOnBlock stick → onBlockActivated Pass → fallback stick 无放置 → false。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(!used, "useItemOnBlock should return false when use on bare cave vines (no berries)");

    // 判定：berries 仍 === false（未长浆果藤蔓右键无反应，不采摘）。
    test.assert(getCaveVinesBerries(test, 3, 2, 1) === false, `cave vines berries should remain false after use on bare vines, got ${getCaveVinesBerries(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerCaveVinesTests(): void {
    GameTest.register("BlockBehaviorTests", "cave_vines_pick_berries_on_use", caveVinesPickBerriesOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "cave_vines_bare_does_not_react_on_use", caveVinesBareDoesNotReactOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
