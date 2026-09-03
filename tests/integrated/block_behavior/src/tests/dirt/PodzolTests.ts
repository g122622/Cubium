// 灰化土（podzol）SNOWY 属性同步行为 GameTest。
//
// vanilla SnowyDirtBlock（:28-77）持有 SNOWY 布尔属性，表示顶部是否覆盖雪：
//   - updateShape（:37-44）：仅当 facing==Up 时，按上方新方块是否雪（isSnowySetting：方块在 SNOW 标签内）
//     同步 SNOWY。其余方向走 super.updateShape（不变）。
//   - getStateForPlacement（:48-52）：读 placementPos.above()，按是否雪设 SNOWY。
//   - isSnowySetting（:56-58）：方块 is(BlockTags.SNOW)。SNOW 标签含 snow、snow_block、grass（此处无关）。
//
// Cubium SnowyDirtBlock（SnowyDirtBlock.cpp）：
//   - getStateForPlacement（:61-74）：aboveState 是 SNOW_BLOCK 或 SNOW（任意层数）→ SNOWY=true。
//   - updatePostPlacement（:76-91）：仅 facing==Up 时，facingState 是 SNOW_BLOCK 或 SNOW → SNOWY=true。
//   podzol 注册为 blocks::SnowyDirtBlock（BaseBlocks.cpp:317-318），最纯粹的不蔓延实例。
//
// 测试覆盖（2 个场景，覆盖放置时 + 邻居更新时两条 SNOWY 同步链路）：
//   1. podzol_snowy_when_snow_block_above_placement：先在 podzol 上方放 snow_block，再用 useItemOnBlock
//      放 podzol 物品到 snow_block 下方，验证 getStateForPlacement 读上方 snow_block 设 SNOWY=true。
//   2. podzol_snowy_syncs_when_snow_block_placed_above：先用 setBlockType 直写 podzol（defaultState
//      SNOWY=false），再在上方 setBlockType snow_block，触发 updatePostPlacement(Up) 同步 SNOWY=true。
//
// 不测「上方放雪层（LAYERS）使 SNOWY=true」：雪层方块需精确控制 LAYERS state，且 Cubium SnowBlock
//   雪层 state 体系与基岩存在差异，价值有限，跳过。TODO: 待雪层 state 体系稳定后补 snow_layer 测试。
// 不测「移除上方雪使 SNOWY=false」：需精确控制雪块移除后上方变 air 触发 updatePostPlacement(Up)，
//   布局复杂且核心「放置上方雪块 SNOWY=true」已由场景 2 覆盖，跳过。
//   TODO: 待需要时补 snow_removed_above 测试。
//
// 跨服务端：podzol/snow_block typeId 两端一致；snowy state 名两端一致（BooleanProperty "snowy"）。
//   放置时 + 邻居更新时同步 SNOWY=true 行为两端一致，可跨服务端对比。
//
// Ref: SnowyDirtBlock.cpp:61-74（getStateForPlacement 读上方雪设 SNOWY）
// Ref: SnowyDirtBlock.cpp:76-91（updatePostPlacement 仅 Up 方向同步 SNOWY）
// Ref: BaseBlocks.cpp:317-318（podzol 注册为 SnowyDirtBlock）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
//
// 坐标偏移约定（见 MinecraftStructurePlacer.cpp:122 与 GameTestHelper.cpp:334）：
//   结构内容从 origin+(0,1,0) 放置（placeOrigin），helper 的 worldBlockPosition(rel)=origin+rel。
//   故相对 y=N 对应结构内 y=N-1。即相对 y=1 是结构首层（结构内 y=0）。
//
// glass_pit 结构内各层（parse-structures.mjs 解析）：
//   y=0（相对 y=1）：满铺 glass 底
//   y=1（相对 y=2）：air 层（玻璃墙围出的内部空腔）
//   y=2（相对 y=3）：air 层
//   y=3（相对 y=4）：air 层
//   y=4（相对 y=5）：cobblestone 顶
//
// 列 (3,*,5) 结构内逐层：
//   (3,0,5)=glass  (3,1,5)=air  (3,2,5)=air  (3,3,5)=air  (3,4,5)=cobblestone
//
// 测试布局（相对坐标）：
//   - GLASS_POS=(3,1,5)：glass 支撑面（结构内 y=0），玩家点击其顶面 Up 放 podzol
//   - PODZOL_POS=(3,2,5)：podzol 放置位（结构内 y=1 = air）
//   - SNOW_BLOCK_POS=(3,3,5)：snow_block 放置位（结构内 y=2 = air），podzol 正上方
const GLASS_POS = { x: 3, y: 1, z: 5 };
const PODZOL_POS = { x: 3, y: 2, z: 5 };
const SNOW_BLOCK_POS = { x: 3, y: 3, z: 5 };

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 podzol 的 SNOWY state（boolean）。返回 null 表示失败或非 podzol。
function getSnowy(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("snowy" as any);
    return typeof value === "boolean" ? value : null;
}

// 场景 1：上方已有 snow_block 时放置 podzol → SNOWY=true（验证 getStateForPlacement）。
//
// 布局：先 (3,3,5) 放 snow_block（setBlockType 直写，下方 (3,2,5) 暂为 air 不触发下方更新）。
//   玩家在 (1,2,5) 点击 (3,1,5) glass 顶面 Up → BlockItem::place 在 (3,2,5) air 位放 podzol。
//   getStateForPlacement 读 (3,2,5).above()=(3,3,5)=snow_block → SNOWY=true。
//
// 判定：podzol SNOWY === true（放置时上方有 snow_block，getStateForPlacement 设 SNOWY=true）。
function podzolSnowyWhenSnowBlockAbovePlacement(test: Test): void {
    // 先在上方放 snow_block（直写，下方暂为 air，不触发下方更新）。
    test.setBlockType("minecraft:snow_block", SNOW_BLOCK_POS);
    test.assert(
        getBlockTypeId(test, SNOW_BLOCK_POS.x, SNOW_BLOCK_POS.y, SNOW_BLOCK_POS.z) === "minecraft:snow_block",
        `snow_block should be at ${JSON.stringify(SNOW_BLOCK_POS)}, got ${getBlockTypeId(test, SNOW_BLOCK_POS.x, SNOW_BLOCK_POS.y, SNOW_BLOCK_POS.z)}`,
    );

    // 玩家在 (1,2,5) 点击 (3,1,5) glass 顶面 Up → podzol 落 (3,2,5)。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 5 }, "farmer");
    const podzolItem = new ItemStack("minecraft:podzol", 1);

    const used = player.useItemOnBlock(
        podzolItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        GLASS_POS,
        Direction.Up,
    );
    test.assert(used, `useItemOnBlock should return true when placing podzol, got used=${used}`);

    pollUntilSucceed(
        test,
        () => {
            return getBlockTypeId(test, PODZOL_POS.x, PODZOL_POS.y, PODZOL_POS.z) === "minecraft:podzol"
                && getSnowy(test, PODZOL_POS.x, PODZOL_POS.y, PODZOL_POS.z) === true;
        },
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(
                    false,
                    `podzol snowy (placement) wrong: typeId=${getBlockTypeId(test, PODZOL_POS.x, PODZOL_POS.y, PODZOL_POS.z)} `
                        + `snowy=${getSnowy(test, PODZOL_POS.x, PODZOL_POS.y, PODZOL_POS.z)} `
                        + `(expected: podzol / snowy=true [snow_block above at placement time])`,
                );
            },
        },
    );
}

// 场景 2：放 podzol 后在上方放 snow_block → SNOWY 同步为 true（验证 updatePostPlacement）。
//
// 布局：先用 setBlockType 直写 podzol 到 (3,2,5)（defaultState SNOWY=false，不经 getStateForPlacement）。
//   再用 setBlockType 放 snow_block 到 (3,3,5)。snow_block 放置向下方 podzol 派发
//   updatePostPlacement(Up, snow_block) → facingState.is(SNOW_BLOCK) → SNOWY=true。
//
// 判定：podzol SNOWY === true（上方放 snow_block 后，updatePostPlacement 同步 SNOWY=true）。
function podzolSnowySyncsWhenSnowBlockPlacedAbove(test: Test): void {
    // 先放 podzol（直写 defaultState，SNOWY=false）。
    test.setBlockType("minecraft:podzol", PODZOL_POS);
    test.assert(
        getBlockTypeId(test, PODZOL_POS.x, PODZOL_POS.y, PODZOL_POS.z) === "minecraft:podzol",
        `podzol should be at ${JSON.stringify(PODZOL_POS)}, got ${getBlockTypeId(test, PODZOL_POS.x, PODZOL_POS.y, PODZOL_POS.z)}`,
    );
    // 放置后立即检查 SNOWY=false（defaultState），确认初始状态正确。
    test.assert(
        getSnowy(test, PODZOL_POS.x, PODZOL_POS.y, PODZOL_POS.z) === false,
        `podzol snowy should be false initially (defaultState), got ${getSnowy(test, PODZOL_POS.x, PODZOL_POS.y, PODZOL_POS.z)}`,
    );

    // 在上方放 snow_block → 触发下方 podzol updatePostPlacement(Up) → SNOWY=true。
    test.setBlockType("minecraft:snow_block", SNOW_BLOCK_POS);

    pollUntilSucceed(
        test,
        () => {
            return getSnowy(test, PODZOL_POS.x, PODZOL_POS.y, PODZOL_POS.z) === true;
        },
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(
                    false,
                    `podzol snowy (neighbor update) wrong: snowy=${getSnowy(test, PODZOL_POS.x, PODZOL_POS.y, PODZOL_POS.z)} `
                        + `(expected: snowy=true [snow_block placed above triggers updatePostPlacement sync])`,
                );
            },
        },
    );
}

export function registerPodzolTests(): void {
    GameTest.register("BlockBehaviorTests", "podzol_snowy_when_snow_block_above_placement", podzolSnowyWhenSnowBlockAbovePlacement)
        .structureName("gametests:glass_pit")
        .maxTicks(60);

    GameTest.register("BlockBehaviorTests", "podzol_snowy_syncs_when_snow_block_placed_above", podzolSnowySyncsWhenSnowBlockPlacedAbove)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
