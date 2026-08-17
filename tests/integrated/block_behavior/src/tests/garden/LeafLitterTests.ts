// 枯叶堆（leaf_litter）集成测试：验证顶面支撑存活（含 stone 等非泥土顶面完整方块）、
// 同格堆叠段数 1→4、堆叠保持朝向、支撑失效自毁行为
// （对齐 wiki 枯叶堆#用途 :48 + LeafLitterBlock 顶面完整支撑判定）。
//
// wiki other_枯叶堆.txt：
//   #用途（:48）："枯叶堆可以放置在所有[[判定箱#方块支撑形状|顶面支撑形状完整]]的方块的上表面，
//     每个方块最多可以放置4个。放置时按逆时针的顺序依次放置。"——支撑面=顶面支撑形状完整的方块
//     （isFaceSturdy(Up, Full)），含草方块/泥土/石头/沙子/陶瓦/玻璃等顶面完整的方块；
//     同格最多 4 段（SEGMENT_AMOUNT 1-4），堆叠按逆时针顺序。
//   #破坏（:45）："枯叶堆被破坏后会掉落自身，掉落个数等于方块内枯叶堆数。"——掉落数=段数，
//     依赖战利品表 + 掉落物实体，跳过。
//   #历史（:343-344）：1.21.5 25w09a 起枯叶只能放顶面支撑形状完整的方块上（之前可放泥土类+草方块），
//     1.21.11 已含此行为。:352 BE 1.21.70.23 起移除骨粉催生（JE 从未支持骨粉），故枯叶不支持骨粉，
//     不测骨粉。
//
// ============================ Cubium 实现链路 ============================
// LeafLitterBlock（garden/LeafLitterBlock.cpp）继承 BushBlock：
//   - 状态属性：FACING（HORIZONTAL_FACING，north/east/south/west）+ SEGMENT_AMOUNT
//     （segment_amount 1-4，默认1）。state 字符串如 "facing=north,segment_amount=1"。
//   - getStateForPlacement（:75-96）：目标位置已有同类型枯叶且 SEGMENT_AMOUNT<4 → SEGMENT_AMOUNT+1
//     （保持 FACING）；否则新放 facing=玩家朝向反方向 + SEGMENT_AMOUNT=1。
//   - isReplaceable（:99-124）：玩家未潜行 + 手持同类型物品 + SEGMENT_AMOUNT<4 → true（同格替换堆叠）。
//   - canSustain（:126-131，本提交新增重写）：用下方方块 isFaceSturdy(Up, Full) 判定，
//     替代 BushBlock 默认的 DIRT 标签判定。这样枯叶可放任意顶面完整方块（草方块/泥土/石头/沙子/
//     陶瓦/玻璃等），对齐 wiki :48。**修复前**继承 BushBlock canSustain（DIRT 标签）只能放泥土类，
//     无法放 stone——本测试2 leaf_litter_survives_on_stone 即验证此修复。
//   - 不重写 updatePostPlacement → 继承 BushBlock::updatePostPlacement（agricultural/BushBlock.cpp:67-93）：
//     facing==Down 时重检下方 canSustain，失败则返回 airState（同步返 air 自毁）。
//
// 堆叠派发链路（同 WildflowersTests/SeaPickleTests）：useItemOnBlock 手持 leaf_litter 物品点击已有枯叶 →
//   onBlockActivated 基类 Pass → fallback Item.useOn → BlockItem::tryPlace → _canReplace（replaceable=true）
//   → placementPos=原枯叶格（同格替换）→ getStateForPlacement 堆叠分支 SEGMENT_AMOUNT+1 → setBlockState 同步。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃墙空腔）============================
// glass_pit：7×5×7，block_palette 仅 glass/cobblestone/air。列 (3,*,1)：y=0 glass 底，y=1 glass 墙，
// y=2..4 air（内部空腔）。setBlockType 直写覆盖 glass，useItemOnBlock 真实放置须命中 air 层（canPlace
// 检查目标非 glass 不可替换），故枯叶放 y=2 air 层（同 WildflowersTests 范式）。
//
// 布局列 (3,*,1)：SUPPORT=(3,1,1)（setBlockType 覆盖 glass 墙成 grass_block/stone），LITTER=(3,2,1)
// （air 层，useItemOnBlock placementPos=支撑.up() 可命中）。玩家站 (5,2,1) 点击 SUPPORT 顶面 Up。
//
// 测试1 leaf_litter_survives_on_grass_block（草方块支撑存活，正向防误判）：
//   grass_block (3,1,1) + useItemOnBlock 放首段 (3,2,1)。canSustain(grass_block)：grass_block 顶面
//   isFaceSturdy(Up,Full)=true → 存活。断言 leaf_litter 存在 + segment_amount=1。
//
// 测试2 leaf_litter_survives_on_stone（石头支撑存活，验证 canSustain 顶面完整判定修复）：
//   stone (3,1,1) + useItemOnBlock 放首段 (3,2,1)。canSustain(stone)：stone 顶面 isFaceSturdy(Up,Full)=true
//   → 存活。**修复前**继承 BushBlock canSustain（DIRT 标签），stone 非 DIRT → canSustain false →
//   isValidPosition false → tryPlace 失败 → useItemOnBlock 返 false，枯叶放不上。本测试验证修复后
//   stone 顶面可放枯叶（wiki :48 顶面完整方块均可放）。
//
// 测试3 leaf_litter_stacks_up_to_4（同格堆叠段数 1→4，wiki :48 每方块最多4个）：
//   grass_block (3,1,1) 支撑。手持 leaf_litter useItemOnBlock 点击 SUPPORT 顶面 Up → 首段落 (3,2,1)
//   segment_amount=1。再点击 LITTER 顶面 Up 堆叠 +1=2，再 +1=3，再 +1=4。每步断言 segment_amount 递增。
//   （同 WildflowersTests 测试4 范式）
//
// 测试4 leaf_litter_keeps_facing_when_stacking（堆叠保持朝向，wiki :48 逆时针放置不重置朝向）：
//   grass_block (3,1,1) 支撑。放首段记录 facing，再堆叠 +1，断言 facing 不变
//   （getStateForPlacement 堆叠分支 with(SEGMENT_AMOUNT) 不动 FACING）。
//   注：不预设 facing 具体值（取决于 SimulatedPlayer 默认朝向），只断言「堆叠前后 facing 相同」。
//
// 测试5 leaf_litter_breaks_when_support_removed（移除支撑自毁，wiki 支撑失效）：
//   grass_block (3,1,1) + leaf_litter (3,2,1)。t=20 移除 grass_block→air。leaf_litter
//   updatePostPlacement(Down) canSustain(air)：air 顶面非 isFaceSturdy(Up,Full) → false → 返 air 自毁。
//
// ============================ 排除项（不写测试）============================
// - 破坏掉落数=段数（wiki :45）：依赖战利品表 segment_amount block_state_property + 掉落物实体，跳过。
// - 燃料 0.5 物品（wiki :51）：依赖熔炉烧炼链路，跳过。
// - 堆肥 30%（wiki :54）：依赖堆肥桶 useItem 链路 + 随机，跳过。
// - 生物群系着色（wiki :56-137）：渲染层颜色，无脚本 API 断言，跳过。
// - 自然生成（wiki :25）：依赖地物 + 生物群系 + 随机，跳过。
// - 骨粉催生（wiki :352）：BE 1.21.70.23 移除、JE 从未支持，两端均不支持，无行为可测，跳过。
// - 满4堆叠不上溢（wiki :48 上限4）：getStateForPlacement amount>=4 return *existingState（保持4），
//   测试3 已覆盖堆叠到 4，此边界不重复测。
// - 逆时针放置顺序（wiki :48）：依赖形状渲染/碰撞盒，无脚本 API 断言段位置，跳过。
//
// ============================ 跨服务端对比 ============================
// - leaf_litter typeId 两端一致（JE 1.21.5 25w02a / BE 1.21.70 加入，1.21.11 已含，wiki :339 :349）。
// - segment_amount state 名两端一致（JE 1.21.5 引入 segment_amount 1-4）。
// - 顶面完整支撑面（:48）、同格堆叠4段（:48）、朝向保持（:48 逆时针不重置）均为 wiki 明文记录的
//   1.21.11 一致行为。
// - 测试用 setBlockType 放 grass_block/stone/air + useItemOnBlock leaf_litter 物品，均为两端通用 API，
//   非 one-sided。同 tick 同步自毁（updatePostPlacement 返 air）+ 堆叠同步 setBlockState，
//   pollUntilSucceed 兼容同步与可能延迟。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_枯叶堆.txt#用途（:48 顶面完整支撑 + 最多4段 + 逆时针）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_枯叶堆.txt#历史（:343-344 25w09a 顶面完整判定 / :352 移除骨粉）
// Ref: LeafLitterBlock.cpp:126-131（canSustain 重写 isFaceSturdy(Up,Full)，本提交修复顶面完整判定）
// Ref: LeafLitterBlock.cpp:75-96（getStateForPlacement 堆叠分支 SEGMENT_AMOUNT+1 保持 facing）
// Ref: LeafLitterBlock.cpp:99-124（isReplaceable 同格替换堆叠，玩家未潜行+同类型物品+SEGMENT_AMOUNT<4）
// Ref: BushBlock.cpp:67-93（updatePostPlacement facing==Down && !canSustain → 返 air 同步自毁）
// Ref: BushBlock.cpp:121-132（canSustain 基类委托下方 canSustainPlant，LeafLitterBlock 重写覆盖之）
// Ref: WildflowersTests.ts（useItemOnBlock 堆叠 +1 范式 + getState 读数量/facing + glass_pit air 层布局）
// Ref: CactusFlowerTests.ts（canSustain 重写用 isFaceSturdy 的先例）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 内部 air 层坐标。支撑 (3,1,1)（覆盖 glass 墙），枯叶 (3,2,1)（air，useItemOnBlock 可放置）。
const SUPPORT = { x: 3, y: 1, z: 1 }; // 下方支撑（grass_block/stone，setBlockType 覆盖 glass 墙）
const LITTER = { x: 3, y: 2, z: 1 }; // 枯叶位置（air 层，useItemOnBlock 可放置）

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 读取枯叶 segment_amount state（number：1-4）。返回 null 表示读取失败或非枯叶。
// Cubium SEGMENT_AMOUNT state C++ 属性名为 "segment_amount"（IntegerProperty::create("segment_amount", 1, 4)），
// getState 对 IntegerProperty 返 number（i32）。用 as any 绕过 BlockStateSuperset 白名单（同 WildflowersTests）。
function getSegmentAmount(test: Test, pos: { x: number; y: number; z: number }): number | null {
    const block = test.getBlock(pos);
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("segment_amount" as any);
    return typeof value === "number" ? value : null;
}

// 读取枯叶 facing state（string：north/east/south/west）。返回 null 表示读取失败或非枯叶。
// Cubium FACING state（HORIZONTAL_FACING）C++ 属性名为 "facing"，getState 对 DirectionProperty 返 string。
function getFacing(test: Test, pos: { x: number; y: number; z: number }): string | null {
    const block = test.getBlock(pos);
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 草方块支撑存活（正向防误判，验证 canSustain(grass_block) 顶面完整通过时不触发自毁）。
// wiki :48 枯叶可放顶面完整的方块。grass_block (3,1,1) 支撑，useItemOnBlock 放首段 (3,2,1)。
function leafLitterSurvivesOnGrassBlock(test: Test): void {
    test.setBlockType("minecraft:grass_block", SUPPORT);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");
    const used = farmer.useItemOnBlock(
        new ItemStack("minecraft:leaf_litter", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        SUPPORT,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing first leaf_litter on grass_block");

    pollUntilSucceed(
        test,
        () => getTypeId(test, LITTER) === "minecraft:leaf_litter" && getSegmentAmount(test, LITTER) === 1,
        {
            startTick: 2,
            interval: 2,
            maxTick: 30,
            onTimeout: () => {
                test.assert(
                    false,
                    `leaf_litter survive on grass_block: expected leaf_litter segment_amount=1 at ${JSON.stringify(LITTER)}, ` +
                        `got typeId=${getTypeId(test, LITTER)} segment_amount=${getSegmentAmount(test, LITTER)} ` +
                        `(support=${getTypeId(test, SUPPORT)}; ` +
                        `if air, canSustain(grass_block) isFaceSturdy(Up,Full) may falsely fail)`,
                );
            },
        },
    );
}

// 石头支撑存活（验证 canSustain 顶面完整判定修复，wiki :48 顶面完整方块均可放）。
// stone (3,1,1) 支撑，useItemOnBlock 放首段 (3,2,1)。canSustain(stone)：stone 顶面 isFaceSturdy(Up,Full)=true。
// **修复前**继承 BushBlock canSustain（DIRT 标签），stone 非 DIRT → 放不上。本测试验证修复后可放 stone。
function leafLitterSurvivesOnStone(test: Test): void {
    test.setBlockType("minecraft:stone", SUPPORT);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");
    const used = farmer.useItemOnBlock(
        new ItemStack("minecraft:leaf_litter", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        SUPPORT,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing first leaf_litter on stone");

    pollUntilSucceed(
        test,
        () => getTypeId(test, LITTER) === "minecraft:leaf_litter" && getSegmentAmount(test, LITTER) === 1,
        {
            startTick: 2,
            interval: 2,
            maxTick: 30,
            onTimeout: () => {
                test.assert(
                    false,
                    `leaf_litter survive on stone: expected leaf_litter segment_amount=1 at ${JSON.stringify(LITTER)}, ` +
                        `got typeId=${getTypeId(test, LITTER)} segment_amount=${getSegmentAmount(test, LITTER)} ` +
                        `(support=${getTypeId(test, SUPPORT)} should be stone; ` +
                        `if used=false or air, canSustain(stone) may still use DIRT-tag (regression of isFaceSturdy fix))`,
                );
            },
        },
    );
}

// 同格堆叠段数 1→4（wiki :48 每方块最多4个，同 WildflowersTests 测试4 范式）。
// grass_block (3,1,1) 支撑。手持 leaf_litter 点击 SUPPORT 顶面 Up → 首段 (3,2,1) segment_amount=1。
// 再点击 LITTER 顶面 Up 堆叠 +1=2，再 +1=3，再 +1=4。每步断言递增。
function leafLitterStacksUpTo4(test: Test): void {
    test.setBlockType("minecraft:grass_block", SUPPORT);
    test.assert(
        getTypeId(test, SUPPORT) === "minecraft:grass_block",
        `grass_block should be at ${JSON.stringify(SUPPORT)}, got ${getTypeId(test, SUPPORT)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");

    // 放置首段：点击 (3,1,1) grass_block 顶面 Up → 枯叶落 (3,2,1) segment_amount=1。
    let used = farmer.useItemOnBlock(
        new ItemStack("minecraft:leaf_litter", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        SUPPORT,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing first leaf_litter");
    test.assert(getTypeId(test, LITTER) === "minecraft:leaf_litter", `leaf_litter should be at ${JSON.stringify(LITTER)}, got ${getTypeId(test, LITTER)}`);
    test.assert(getSegmentAmount(test, LITTER) === 1, `first leaf_litter segment_amount should be 1, got ${getSegmentAmount(test, LITTER)}`);

    // 堆叠 +1 到 2：点击 (3,2,1) 顶面 Up → 同格替换堆叠，segment_amount=2。
    used = farmer.useItemOnBlock(
        new ItemStack("minecraft:leaf_litter", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        LITTER,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when stacking leaf_litter to 2");
    test.assert(getSegmentAmount(test, LITTER) === 2, `leaf_litter segment_amount should be 2 after stack, got ${getSegmentAmount(test, LITTER)}`);

    // 堆叠 +1 到 3：点击 (3,2,1) 顶面 Up → segment_amount=3。
    used = farmer.useItemOnBlock(
        new ItemStack("minecraft:leaf_litter", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        LITTER,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when stacking leaf_litter to 3");
    test.assert(getSegmentAmount(test, LITTER) === 3, `leaf_litter segment_amount should be 3 after stack, got ${getSegmentAmount(test, LITTER)}`);

    // 堆叠 +1 到 4：点击 (3,2,1) 顶面 Up → segment_amount=4（达上限）。
    used = farmer.useItemOnBlock(
        new ItemStack("minecraft:leaf_litter", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        LITTER,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when stacking leaf_litter to 4");
    test.assert(getSegmentAmount(test, LITTER) === 4, `leaf_litter segment_amount should be 4 after stack, got ${getSegmentAmount(test, LITTER)}`);

    test.succeed();
}

// 堆叠保持朝向（wiki :48 逆时针放置不重置朝向，同 WildflowersTests 测试5 范式）。
// grass_block (3,1,1) 支撑。放首段记录 facing，再堆叠 +1，断言 facing 不变
// （getStateForPlacement 堆叠分支 with(SEGMENT_AMOUNT) 不动 FACING）。
function leafLitterKeepsFacingWhenStacking(test: Test): void {
    test.setBlockType("minecraft:grass_block", SUPPORT);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");

    // 放置首段：点击 (3,1,1) 顶面 Up → 枯叶落 (3,2,1)，facing=玩家朝向反方向，segment_amount=1。
    const usedFirst = farmer.useItemOnBlock(
        new ItemStack("minecraft:leaf_litter", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        SUPPORT,
        Direction.Up,
    );
    test.assert(usedFirst, "useItemOnBlock should return true when placing first leaf_litter for facing test");
    test.assert(getTypeId(test, LITTER) === "minecraft:leaf_litter", `leaf_litter should be at ${JSON.stringify(LITTER)}, got ${getTypeId(test, LITTER)}`);

    const facingBefore = getFacing(test, LITTER);
    test.assert(
        facingBefore !== null && ["north", "east", "south", "west"].includes(facingBefore as string),
        `leaf_litter facing should be a valid horizontal direction before stack, got ${facingBefore}`,
    );

    // 堆叠 +1 到 2：点击 (3,2,1) 顶面 Up。getStateForPlacement 堆叠分支 with(SEGMENT_AMOUNT,2) 不动 FACING。
    const usedStack = farmer.useItemOnBlock(
        new ItemStack("minecraft:leaf_litter", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        LITTER,
        Direction.Up,
    );
    test.assert(usedStack, "useItemOnBlock should return true when stacking leaf_litter for facing test");
    test.assert(getSegmentAmount(test, LITTER) === 2, `leaf_litter segment_amount should be 2 after stack, got ${getSegmentAmount(test, LITTER)}`);

    // 断言 facing 不变（堆叠保持原朝向，wiki :48 逆时针不重置朝向）。
    const facingAfter = getFacing(test, LITTER);
    test.assert(
        facingAfter === facingBefore,
        `leaf_litter facing should not change when stacking (wiki :48), before=${facingBefore} after=${facingAfter}`,
    );

    test.succeed();
}

// 移除支撑 → 自毁（wiki 支撑失效，BushBlock 通用机制）。
// grass_block (3,1,1) + leaf_litter (3,2,1)。t=20 移除 grass_block→air。leaf_litter updatePostPlacement(Down)
// canSustain(air)：air 顶面非 isFaceSturdy(Up,Full) → false → 返 air 自毁。
function leafLitterBreaksWhenSupportRemoved(test: Test): void {
    test.setBlockType("minecraft:grass_block", SUPPORT);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");
    const used = farmer.useItemOnBlock(
        new ItemStack("minecraft:leaf_litter", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        SUPPORT,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing first leaf_litter for self-destruct test");
    test.assert(getTypeId(test, LITTER) === "minecraft:leaf_litter", `leaf_litter should be at ${JSON.stringify(LITTER)}, got ${getTypeId(test, LITTER)}`);

    test.runAtTickTime(20, () => {
        if (getTypeId(test, LITTER) === "minecraft:leaf_litter") {
            test.setBlockType("minecraft:air", SUPPORT); // 移除 grass_block，派发 Up 更新触发 leaf_litter updatePostPlacement(Down)
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, LITTER) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `leaf_litter break on support removed: expected air at ${JSON.stringify(LITTER)} after removing grass_block, ` +
                        `got ${getTypeId(test, LITTER)} ` +
                        `(support=${getTypeId(test, SUPPORT)} should be air; ` +
                        `if still leaf_litter, BushBlock updatePostPlacement Down-fail->air may be missing)`,
                );
            },
        },
    );
}

export function registerLeafLitterTests(): void {
    GameTest.register("BlockBehaviorTests", "leaf_litter_survives_on_grass_block", leafLitterSurvivesOnGrassBlock)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "leaf_litter_survives_on_stone", leafLitterSurvivesOnStone)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "leaf_litter_stacks_up_to_4", leafLitterStacksUpTo4)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "leaf_litter_keeps_facing_when_stacking", leafLitterKeepsFacingWhenStacking)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "leaf_litter_breaks_when_support_removed", leafLitterBreaksWhenSupportRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
}
