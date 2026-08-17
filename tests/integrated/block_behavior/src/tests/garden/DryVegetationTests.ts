// 矮枯草丛（short_dry_grass）/高枯草丛（tall_dry_grass）集成测试：验证种植支撑存活、支撑失效自毁、
// 支撑替换为不可种植方块自毁行为
// （对齐 wiki 矮枯草丛#用途 :40 / 高枯草丛#用途 :40 支撑面 + BushBlock 通用支撑失效自毁机制）。
//
// wiki other_矮枯草丛.txt#用途（:40）："矮枯草丛只能放置在[[草方块]]、[[菌丝体]]、[[灰化土]]、[[泥土]]、
//   [[耕地]]、[[缠根泥土]]、[[砂土]]、[[泥巴]]、[[沾泥的红树根]]、[[苔藓块]]、[[苍白苔藓块]]、[[沙子]]、
//   [[红沙]]、[[可疑的沙子]]和[[陶瓦]]上。"
// wiki other_高枯草丛.txt#用途（:40）：同样的支撑面清单。
//   ——支撑条件：DIRT 标签面（草方块/泥土/耕地等）+ SAND 标签（沙子/红沙）+ TERRACOTTA 标签（陶瓦），
//   即 #dry_vegetation_may_place_on 标签（= SAND + TERRACOTTA + DIRT + FARMLAND）。
//   比普通 Bush（仅 DIRT 标签）多了沙/陶瓦，以支持沙漠/恶地生成。
//   支撑失效（下方变 air 或非标签方块）时植物自毁（BushBlock 通用机制）。
//
// ============================ Cubium 实现链路 ============================
// DryVegetationBlock（garden/DryVegetationBlock.cpp）继承 BushBlock，不重写 updatePostPlacement → 继承
//   BushBlock::updatePostPlacement（agricultural/BushBlock.cpp:67-93）：facing==Down 时重检下方 canSustain，
//   失败则返回 airState（**同步返 air**，引擎同 tick 替换方块为 air，非 scheduleBlockTick 延迟）。
// DryVegetationBlock::canSustain（:38-44）：查 BlockTags::DRY_VEGETATION_MAY_PLACE_ON().contains(groundState)
//   （= SAND + TERRACOTTA + DIRT + FARMLAND 合并标签，BlockTags.cpp:1264-1282）。
//   注：short_dry_grass 与 tall_dry_grass 共用 DryVegetationBlock 类，canSustain 同标签判定；
//   本测试测 short_dry_grass 代表 SAND/DIRT 标签分支 + 支撑自毁，另测 tall_dry_grass 验证独立类 +
//   TERRACOTTA 标签分支（陶瓦支撑面，干草独有）。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃墙空腔）============================
// glass_pit：7×5×7，block_palette 仅 glass/cobblestone/air 三种。底层 (3,0,1) 是 air（玻璃墙围空腔底），
// 可用 setBlockType 直写覆盖（setBlockType 不经 canPlace，覆盖 air/glass 均可），但 **(3,0,1) 下方
// (3,-1,1) 在结构外为 air**。
//   ——陷阱：sand 是重力方块（FallingBlock）。setBlockType("minecraft:sand", (3,0,1)) 后，sand 检测下方
//   air → 下落消失（实测 timeout 时支撑位变 air，植物支撑失效自毁；下落途中可能落穿到世界底部岩浆层，
//   诊断曾误报 support=minecraft:lava）。grass_block/terracotta/nylium 等非重力方块放 (3,0,1) 稳定（不受此影响）。
//   故 **sand 测试须用三层布局**：cobblestone(3,0,1) 托底（实心非重力）+ sand(3,1,1)（下方 cobblestone 不落）
//   + plant(3,2,1)（air 层）。terracotta 非重力，可直接两层 (3,0,1)+(3,1,1)。
//
// 布局列 (3,*,1)（同 CactusFlowerTests/NetherRootsTests 坐标范式）：
//   - sand 测试：BASE=(3,0,1) cobblestone 托底，SAND=(3,1,1) sand 支撑，PLANT=(3,2,1) 植物。
//   - terracotta 测试：SUPPORT=(3,0,1) terracotta 支撑，PLANT=(3,1,1) 植物。
//   本组测试均用 setBlockType 直写（支撑自毁范式，非 useItemOnBlock），不受 glass_pit 坐标
//   陷阱影响（setBlockType 覆盖 air/glass 不经 canPlace）；唯 sand 须托底防下落。
//
// 测试1 short_dry_grass_survives_on_sand（沙子支撑存活，SAND 标签正向，干草独有支撑面）：
//   cobblestone (3,0,1) + sand (3,1,1) + short_dry_grass (3,2,1)。canSustain(sand)：sand ∈ DRY_VEGETATION_MAY_PLACE_ON
//   （SAND 标签成员）→ true。等待后断言存活（防自毁误触发 + 防 sand 下落）。验证干草比普通 Bush 多的沙支撑面。
//
// 测试2 short_dry_grass_survives_on_grass_block（草方块支撑存活，DIRT 标签正向）：
//   grass_block (3,0,1) + short_dry_grass (3,1,1)。canSustain(grass_block)：grass_block ∈ DIRT 标签
//   → DRY_VEGETATION_MAY_PLACE_ON 包含 → true。等待后断言存活。grass_block 非重力，两层布局稳定。
//
// 测试3 short_dry_grass_breaks_when_sand_support_removed（移除沙子支撑自毁，wiki 支撑失效）：
//   cobblestone (3,0,1) + sand (3,1,1) + short_dry_grass (3,2,1)。t=20 移除 sand→air。short_dry_grass
//   updatePostPlacement(Down) canSustain(air) 失败（air 非标签）→ 返 air 自毁。断言变 air。
//
// 测试4 short_dry_grass_breaks_when_support_replaced_with_stone（支撑换 stone 自毁，验证标签判定）：
//   cobblestone (3,0,1) + sand (3,1,1) + short_dry_grass (3,2,1)。t=20 把 sand 换 stone。short_dry_grass
//   updatePostPlacement(Down) canSustain(stone)：stone 非 DRY_VEGETATION_MAY_PLACE_ON
//   （非 SAND/TERRACOTTA/DIRT/FARMLAND）→ false → 返 air 自毁。验证 canSustain 标签判定
//   （stone 不在 wiki 支撑面清单 :40，自毁）。
//
// 测试5 tall_dry_grass_survives_on_terracotta（陶瓦支撑存活，TERRACOTTA 标签正向，干草独有支撑面）：
//   terracotta (3,0,1) + tall_dry_grass (3,1,1)。canSustain(terracotta)：terracotta ∈ TERRACOTTA 标签
//   → DRY_VEGETATION_MAY_PLACE_ON 包含 → true。等待后断言存活。验证 tall_dry_grass 独立类继承
//   DryVegetationBlock canSustain 标签判定 + 陶瓦支撑面（干草独有，普通 Bush 无）。terracotta 非重力，两层布局稳定。
//
// ============================ 排除项（不写测试）============================
// - 红沙/可疑的沙子支撑（wiki :40）：与沙子同类（SAND 标签成员），测 sand 代表，不重复。
// - 各色陶瓦支撑（TERRACOTTA 标签含 colored terracotta）：测 terracotta 代表，不重复各色。
// - 骨粉矮→高/高→矮传播（wiki :24 :44）：DryVegetationBlock 不继承 IGrowable，Cubium 未实现骨粉
//   传播链路，跳过。TODO: 待干草骨粉传播实现后补。
// - 绵羊啃食（wiki :42）：依赖绵羊 AI + mobGriefing 规则，跳过。
// - 剪子/精准采集掉落（wiki :37）：依赖物品工具判定 + 掉落物实体，跳过。
// - 堆肥 30%（wiki :47）：依赖堆肥桶 useItem 链路 + 随机，跳过。
// - 燃料 0.5 物品（wiki :50）：依赖熔炉烧炼链路，跳过。
// - 环境音效下方两格沙/陶瓦（wiki :53）：依赖音效系统，无脚本 API 断言，跳过。
// - 含雪（wiki :44 BE only）：BE 特性 + 含雪状态，跳过。
//
// ============================ 跨服务端对比 ============================
// - short_dry_grass/tall_dry_grass typeId 两端一致（JE 1.21.5 25w06a / BE 1.21.70 加入，1.21.11 已含，
//   wiki :101 :109）。两端均 1.21.11 已含。
// - 种植支撑面（:40 草方块/...沙子/红沙/可疑的沙子/陶瓦）两端 wiki 明文一致。
// - 支撑失效自毁为 BushBlock 通用机制（Java BushBlock.updateShape + canSurvive），两端一致。
// - 测试用 setBlockType 放 sand/grass_block/terracotta/stone/short_dry_grass/tall_dry_grass/air，均为两端
//   通用 API，非 one-sided。同 tick 同步自毁（updatePostPlacement 返 air），pollUntilSucceed 兼容。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_矮枯草丛.txt#用途（:40 支撑面含沙子/陶瓦）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_高枯草丛.txt#用途（:40 同支撑面清单）
// Ref: DryVegetationBlock.cpp:38-44（canSustain 查 DRY_VEGETATION_MAY_PLACE_ON 标签）
// Ref: BlockTags.cpp:1264-1282（DRY_VEGETATION_MAY_PLACE_ON = SAND + TERRACOTTA + DIRT + FARMLAND 合并）
// Ref: BushBlock.cpp:67-93（updatePostPlacement facing==Down && !canSustain → 返 air 同步自毁）
// Ref: CactusFlowerTests.ts / NetherRootsTests.ts（glass_pit (3,0,1)支撑+(3,1,1)植物 + 支撑自毁范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 内部 air 层坐标。
// sand 测试（三层，防 sand 重力下落）：BASE=(3,0,1) cobblestone 托底，SAND=(3,1,1) sand 支撑，PLANT=(3,2,1) 植物。
// terracotta 测试（两层，terracotta 非重力）：SUPPORT=(3,0,1) terracotta 支撑，PLANT_TERRA=(3,1,1) 植物。
const BASE = { x: 3, y: 0, z: 1 }; // sand 测试托底（cobblestone，实心非重力，托住 sand 不下落）
const SAND = { x: 3, y: 1, z: 1 }; // sand 测试支撑（sand，重力方块，须下方实心）
const PLANT = { x: 3, y: 2, z: 1 }; // sand 测试植物位置（short_dry_grass，air 层）
const SUPPORT = { x: 3, y: 0, z: 1 }; // terracotta 测试支撑（terracotta，非重力，两层布局稳定）
const PLANT_TERRA = { x: 3, y: 1, z: 1 }; // terracotta 测试植物位置（tall_dry_grass）

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 沙子支撑存活（SAND 标签正向，干草独有支撑面，普通 Bush 无沙支撑）。
// wiki :40 矮枯草丛可种沙子。cobblestone (3,0,1) 托底 + sand (3,1,1) + short_dry_grass (3,2,1)。
// sand 是重力方块，须下方 cobblestone 实心托底防下落（见文件头陷阱说明）。不做破坏，等待后断言存活。
function shortDryGrassSurvivesOnSand(test: Test): void {
    test.setBlockType("minecraft:cobblestone", BASE);
    test.setBlockType("minecraft:sand", SAND);
    test.setBlockType("minecraft:short_dry_grass", PLANT);

    pollUntilSucceed(
        test,
        () => getTypeId(test, PLANT) === "minecraft:short_dry_grass",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `short_dry_grass survive on sand: expected short_dry_grass to remain at ${JSON.stringify(PLANT)}, ` +
                        `got ${getTypeId(test, PLANT)} ` +
                        `(base=${getTypeId(test, BASE)} should be cobblestone; ` +
                        `sand=${getTypeId(test, SAND)} should be sand; ` +
                        `if sand=air, sand may have fallen (gravity) or canSustain(sand) SAND-tag falsely failed)`,
                );
            },
        },
    );
}

// 草方块支撑存活（DIRT 标签正向，grass_block ∈ DIRT ⊂ DRY_VEGETATION_MAY_PLACE_ON）。
// wiki :40 矮枯草丛可种草方块。grass_block (3,0,1) + short_dry_grass (3,1,1)，等待后断言存活。
function shortDryGrassSurvivesOnGrassBlock(test: Test): void {
    test.setBlockType("minecraft:grass_block", SUPPORT);
    test.setBlockType("minecraft:short_dry_grass", PLANT);

    pollUntilSucceed(
        test,
        () => getTypeId(test, PLANT) === "minecraft:short_dry_grass",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `short_dry_grass survive on grass_block: expected short_dry_grass to remain at ${JSON.stringify(PLANT)}, ` +
                        `got ${getTypeId(test, PLANT)} ` +
                        `(support=${getTypeId(test, SUPPORT)}; ` +
                        `if air, canSustain(grass_block) DIRT-tag may falsely fail)`,
                );
            },
        },
    );
}

// 移除沙子支撑 → 自毁（wiki 支撑失效，BushBlock 通用机制）。
// cobblestone (3,0,1) + sand (3,1,1) + short_dry_grass (3,2,1)。t=20 移除 sand→air。short_dry_grass
// updatePostPlacement(Down) canSustain(air) 失败（air 非标签）→ 返 air 自毁。
function shortDryGrassBreaksWhenSandSupportRemoved(test: Test): void {
    test.setBlockType("minecraft:cobblestone", BASE);
    test.setBlockType("minecraft:sand", SAND);
    test.setBlockType("minecraft:short_dry_grass", PLANT);

    test.runAtTickTime(20, () => {
        if (getTypeId(test, PLANT) === "minecraft:short_dry_grass") {
            test.setBlockType("minecraft:air", SAND); // 移除 sand，派发 Up 更新触发 short_dry_grass updatePostPlacement(Down)
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, PLANT) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `short_dry_grass break on sand removed: expected air at ${JSON.stringify(PLANT)} after removing sand, ` +
                        `got ${getTypeId(test, PLANT)} ` +
                        `(sand=${getTypeId(test, SAND)} should be air; ` +
                        `if still short_dry_grass, BushBlock updatePostPlacement Down-fail->air may be missing)`,
                );
            },
        },
    );
}

// 支撑换 stone → 自毁（验证 canSustain 标签判定，stone 非 wiki 支撑面 :40）。
// cobblestone (3,0,1) + sand (3,1,1) + short_dry_grass (3,2,1)。t=20 把 sand 换 stone。short_dry_grass
// updatePostPlacement(Down) canSustain(stone)：stone 非 DRY_VEGETATION_MAY_PLACE_ON
// （非 SAND/TERRACOTTA/DIRT/FARMLAND）→ false → 返 air 自毁。
function shortDryGrassBreaksWhenSupportReplacedWithStone(test: Test): void {
    test.setBlockType("minecraft:cobblestone", BASE);
    test.setBlockType("minecraft:sand", SAND);
    test.setBlockType("minecraft:short_dry_grass", PLANT);

    test.runAtTickTime(20, () => {
        if (getTypeId(test, PLANT) === "minecraft:short_dry_grass") {
            // 把 sand 换成 stone（真实变化，派发 Up 更新）。stone 非标签，canSustain 失败。
            test.setBlockType("minecraft:stone", SAND);
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, PLANT) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `short_dry_grass break on stone support: expected air at ${JSON.stringify(PLANT)} after replacing sand with stone, ` +
                        `got ${getTypeId(test, PLANT)} ` +
                        `(sand-support=${getTypeId(test, SAND)} should be stone; ` +
                        `if still short_dry_grass, canSustain(stone) may falsely return true (stone not in DRY_VEGETATION_MAY_PLACE_ON))`,
                );
            },
        },
    );
}

// 陶瓦支撑高枯草丛存活（TERRACOTTA 标签正向，干草独有支撑面；验证 tall_dry_grass 独立类）。
// wiki :40 高枯草丛可种陶瓦。terracotta (3,0,1) + tall_dry_grass (3,1,1)。canSustain(terracotta)：
// terracotta ∈ TERRACOTTA 标签 → DRY_VEGETATION_MAY_PLACE_ON 包含 → true。等待后断言存活。
// terracotta 非重力方块，两层布局稳定（无需托底）。
function tallDryGrassSurvivesOnTerracotta(test: Test): void {
    test.setBlockType("minecraft:terracotta", SUPPORT);
    test.setBlockType("minecraft:tall_dry_grass", PLANT_TERRA);

    pollUntilSucceed(
        test,
        () => getTypeId(test, PLANT_TERRA) === "minecraft:tall_dry_grass",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `tall_dry_grass survive on terracotta: expected tall_dry_grass to remain at ${JSON.stringify(PLANT_TERRA)}, ` +
                        `got ${getTypeId(test, PLANT_TERRA)} ` +
                        `(support=${getTypeId(test, SUPPORT)}; ` +
                        `if air, canSustain(terracotta) TERRACOTTA-tag may falsely fail or DRY_VEGETATION_MAY_PLACE_ON missing terracotta)`,
                );
            },
        },
    );
}

export function registerDryVegetationTests(): void {
    GameTest.register("BlockBehaviorTests", "short_dry_grass_survives_on_sand", shortDryGrassSurvivesOnSand)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "short_dry_grass_survives_on_grass_block", shortDryGrassSurvivesOnGrassBlock)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "short_dry_grass_breaks_when_sand_support_removed", shortDryGrassBreaksWhenSandSupportRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "short_dry_grass_breaks_when_support_replaced_with_stone", shortDryGrassBreaksWhenSupportReplacedWithStone)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "tall_dry_grass_survives_on_terracotta", tallDryGrassSurvivesOnTerracotta)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
