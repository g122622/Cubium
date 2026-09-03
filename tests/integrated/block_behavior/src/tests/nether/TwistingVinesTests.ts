// 缠怨藤（twisting_vines，向上生长）与垂泪藤（weeping_vines，向下生长）行为 GameTest。
//
// wiki block_缠怨藤.txt#生长（:57-65）：
//   "缠怨藤上方为空气方块时，它会向上生长。
//    ……缠怨藤生长时，新长出的缠怨藤的 age 值增加1，缠怨藤 age 值为25时不再生长。
//    使用骨粉可以加速缠怨藤生长。"
//   关键行为：① randomTick 10% 概率向上生长一格（原位变 plant 身体，上方放新头部 age+1）；
//   ② 骨粉可加速生长（wiki:65 明文）。
//
// wiki block_垂泪藤.txt#生长（:65-73）：
//   "垂泪藤下方为空气方块时，它会向下生长。
//    ……垂泪藤生长时，新长出的垂泪藤的 age 值增加1，垂泪藤 age 值为25时不再生长。
//    使用骨粉可以加速垂泪藤生长。"
//   关键行为：① randomTick 10% 概率向下生长一格；② 骨粉可加速生长。
//
// ============================ Cubium 实现链路 ============================
// TwistingVinesBlock（nether/TwistingVinesBlock.cpp）继承 GrowingPlantHeadBlock（Direction::Up，
//   growPerTickProbability=0.1）。randomTick（GrowingPlantHeadBlock.cpp:70-103）：
//   age>=MAX_AGE(25) 返；random.nextFloat()>=0.1 返；计算 growPos=上方；canGrowInto 检查（须 air）；
//   原位放身体方块（twisting_vines_plant），上方放新头部（getGrowIntoState→age+1）。
// WeepingVinesBlock（nether/WeepingVinesBlock.cpp）继承 GrowingPlantHeadBlock（Direction::Down，
//   growPerTickProbability=0.1）。randomTick 同链路，growPos=下方。
//
// GrowingPlantHeadBlock::randomTick 生长后形态：
//   - 原头部位置：变为身体方块（twisting_vines_plant / weeping_vines_plant）
//   - 生长方向下一格：新头部方块（twisting_vines / weeping_vines，age+1）
//   故判定生长：生长方向下一格出现头部方块 typeId（twisting_vines / weeping_vines），
//   且原位变 plant。但为稳健起见，只判定生长方向下一格出现头部方块即可（生长必定延伸）。
//
// ============================ 骨粉加速生长（已修复）============================
// wiki block_缠怨藤.txt:65 / block_垂泪藤.txt:73 明文："使用骨粉可以加速 X 生长。"
// 修复前缺陷：TwistingVinesBlock/WeepingVinesBlock 不实现 IGrowable，BoneMealItem::onItemUse
//   dynamic_cast<const IGrowable*>(&block) 返 nullptr → 跳过 IGrowable 分支 → 返 Fail → 骨粉无效。
// 修复方案：让 GrowingPlantHeadBlock 继承 IGrowable，提供 canGrow/canUseBonemeal/grow 默认实现。
//   grow 在生长方向下一格放置新头部方块（age+1），原位放身体方块——与 randomTick 生长逻辑一致。
// 测试验证：useItemOnBlock 返 true + 生长目标格出现藤蔓头部方块（骨粉加速生长延伸）。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃坑）============================
// glass_pit：y=0 glass 底座，y=1..3 air 空腔，y=4 glass 顶部。helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// 结构内容从 origin+(0,1,0) 放置（placeOrigin），helper worldBlockPosition(rel)=origin+rel。
// 故相对 y=N 对应结构内 y=N-1。
//
// 缠怨藤向上生长测试布局：
//   (3,1,1) 放 stone（下方支撑，isSolid 满足 isValidPosition）。
//   (3,2,1) 放 twisting_vines（age=0，头部，向上生长）。
//   上方 (3,3,1) 为 air（生长目标格）。
//   调高 randomTickSpeed 后轮询 (3,3,1) 出现 twisting_vines（生长延伸）。
//
// 垂泪藤向下生长测试布局：
//   (3,3,1) 放 weeping_vines（age=0，头部，向下生长）。上方需支撑：GrowingPlantBlock 向下生长
//   isValidPosition 检查 opposite(Down)=Up 方向支撑。故 (3,4,1) 放 stone（上方支撑）。
//   下方 (3,2,1) 为 air（生长目标格）。
//   调高 randomTickSpeed 后轮询 (3,2,1) 出现 weeping_vines（生长延伸）。
//
// ============================ 排除项（不写测试）============================
// - 骨粉散布范围生成藤蔓（诡异菌岩骨粉 1/8 概率）：随机性强，跳过。
// - 剪刀阻止生长（age→25）：藤蔓未实现 onBlockActivated 剪刀逻辑，属未实现，按准则不写。
//   TODO: 待剪刀交互实现后补充。
// - 攀爬行为：依赖玩家移动 AI + 攀爬判定，属实体行为，跳过。
// - 破坏掉落（33% 概率）：依赖破坏物品链路 + 随机，跳过。
// - 堆肥（50% 概率）：依赖堆肥桶 useItem + 随机，跳过。
//
// ============================ 跨服务端对比 ============================
// - twisting_vines/weeping_vines/twisting_vines_plant/weeping_vines_plant typeId 两端一致（1.16 加入）。
// - age 状态属性两端一致（age 0-25）。
// - randomTick 10% 生长行为两端一致（wiki 明文）。
// - 骨粉加速生长两端一致（wiki 明文）——但 Cubium 未实现 IGrowable，骨粉无效，属 Cubium 缺陷。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_缠怨藤.txt#生长（:57-65 向上生长+骨粉加速）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_垂泪藤.txt#生长（:65-73 向下生长+骨粉加速）
// Ref: GrowingPlantHeadBlock.cpp:70-103（randomTick 10% 生长，原位变身体+生长方向放新头部）
// Ref: TwistingVinesBlock.cpp（Direction::Up, growPerTickProbability=0.1, getHeadBlock/getBodyBlock）
// Ref: WeepingVinesBlock.cpp（Direction::Down, growPerTickProbability=0.1, getHeadBlock/getBodyBlock）
// Ref: BoneMealItem.cpp:70（dynamic_cast<IGrowable> 返 nullptr → 骨粉无效，Cubium 缺陷）
// Ref: NetherWartTests.ts（randomTickSpeed 调高 + pollUntilSucceed 范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 内部坐标。缠怨藤向上生长：支撑 (3,1,1)，藤蔓 (3,2,1)，生长目标 (3,3,1)。
const TWISTING_SUPPORT = { x: 3, y: 1, z: 1 }; // 下方 stone 支撑
const TWISTING_VINE = { x: 3, y: 2, z: 1 }; // twisting_vines 头部
const TWISTING_GROW_TARGET = { x: 3, y: 3, z: 1 }; // 上方 air，生长目标格

// 垂泪藤向下生长：上方支撑 (3,4,1)，藤蔓 (3,3,1)，生长目标 (3,2,1)。
// 垂泪藤头部需上方支撑（opposite(Down)=Up 方向），生长方向 Down，growPos=下方一格。
// 必须确保 growTarget 落在 air 空腔（结构内 y=1，helper 相对 y=2）而非 glass 底座层。
const WEEPING_SUPPORT = { x: 3, y: 4, z: 1 }; // 上方 stone 支撑
const WEEPING_VINE = { x: 3, y: 3, z: 1 }; // weeping_vines 头部
const WEEPING_GROW_TARGET = { x: 3, y: 2, z: 1 }; // 下方 air，生长目标格

// 调高 randomTickSpeed 使藤蔓格在数 tick 内被随机刻确定性命中。
// 1000 使单格每 tick 命中概率≈24.4%。命中后 10% 概率生长，多次命中后生长概率→1。
const HIGH_RANDOM_TICK_SPEED = "1000";

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 缠怨藤 randomTick 向上生长（wiki: 上方为空气时向上生长，10% 概率）。
// 布局：(3,1,1) stone 支撑，(3,2,1) twisting_vines(age=0)，上方 (3,3,1) 为 air。
// 调高 randomTickSpeed 后轮询 (3,3,1) 出现 twisting_vines（生长延伸到上方）。
function twistingVinesGrowsUpwardViaRandomTick(test: Test): void {
    test.setBlockType("minecraft:stone", TWISTING_SUPPORT);
    test.setBlockType("minecraft:twisting_vines", TWISTING_VINE);
    test.assert(
        getTypeId(test, TWISTING_VINE) === "minecraft:twisting_vines",
        `twisting_vines should be at ${JSON.stringify(TWISTING_VINE)}, got ${getTypeId(test, TWISTING_VINE)}`,
    );
    test.assert(
        getTypeId(test, TWISTING_GROW_TARGET) === "minecraft:air",
        `grow target should be air before growth, got ${getTypeId(test, TWISTING_GROW_TARGET)}`,
    );

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    player.chat(`/gamerule randomTickSpeed ${HIGH_RANDOM_TICK_SPEED}`);

    pollUntilSucceed(
        test,
        () => getTypeId(test, TWISTING_GROW_TARGET) === "minecraft:twisting_vines",
        {
            startTick: 40,
            interval: 20,
            maxTick: 200,
            onTimeout: () => {
                test.assert(
                    false,
                    `twisting_vines random-tick growth: expected twisting_vines at ${JSON.stringify(TWISTING_GROW_TARGET)} (upward), ` +
                        `got ${getTypeId(test, TWISTING_GROW_TARGET)} ` +
                        `(vine=${getTypeId(test, TWISTING_VINE)}; if air, randomTick 10% upward growth may be missing)`,
                );
            },
        },
    );
}

// 垂泪藤 randomTick 向下生长（wiki: 下方为空气时向下生长，10% 概率）。
// 布局：(3,4,1) stone 上方支撑，(3,3,1) weeping_vines(age=0)，下方 (3,2,1) 为 air。
// 调高 randomTickSpeed 后轮询 (3,2,1) 出现 weeping_vines（生长延伸到下方）。
function weepingVinesGrowsDownwardViaRandomTick(test: Test): void {
    test.setBlockType("minecraft:stone", WEEPING_SUPPORT);
    test.setBlockType("minecraft:weeping_vines", WEEPING_VINE);
    test.assert(
        getTypeId(test, WEEPING_VINE) === "minecraft:weeping_vines",
        `weeping_vines should be at ${JSON.stringify(WEEPING_VINE)}, got ${getTypeId(test, WEEPING_VINE)}`,
    );
    test.assert(
        getTypeId(test, WEEPING_GROW_TARGET) === "minecraft:air",
        `grow target should be air before growth, got ${getTypeId(test, WEEPING_GROW_TARGET)}`,
    );

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    player.chat(`/gamerule randomTickSpeed ${HIGH_RANDOM_TICK_SPEED}`);

    pollUntilSucceed(
        test,
        () => getTypeId(test, WEEPING_GROW_TARGET) === "minecraft:weeping_vines",
        {
            startTick: 40,
            interval: 20,
            maxTick: 200,
            onTimeout: () => {
                test.assert(
                    false,
                    `weeping_vines random-tick growth: expected weeping_vines at ${JSON.stringify(WEEPING_GROW_TARGET)} (downward), ` +
                        `got ${getTypeId(test, WEEPING_GROW_TARGET)} ` +
                        `(vine=${getTypeId(test, WEEPING_VINE)}; if air, randomTick 10% downward growth may be missing)`,
                );
            },
        },
    );
}

// 缠怨藤骨粉加速生长（wiki:65 明文骨粉可加速生长）。
// 布局：(3,1,1) stone 支撑，(3,2,1) twisting_vines(age=0)，上方 (3,3,1) 为 air。
// 骨粉后断言：useItemOnBlock 返 true + (3,3,1) 出现 twisting_vines（生长延伸到上方）。
function twistingVinesBonemealGrows(test: Test): void {
    test.setBlockType("minecraft:stone", TWISTING_SUPPORT);
    test.setBlockType("minecraft:twisting_vines", TWISTING_VINE);
    test.assert(
        getTypeId(test, TWISTING_VINE) === "minecraft:twisting_vines",
        `twisting_vines should be at ${JSON.stringify(TWISTING_VINE)}, got ${getTypeId(test, TWISTING_VINE)}`,
    );
    test.assert(
        getTypeId(test, TWISTING_GROW_TARGET) === "minecraft:air",
        `grow target should be air before growth, got ${getTypeId(test, TWISTING_GROW_TARGET)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对缠怨藤 useItemOnBlock 骨粉 → IGrowable::grow 在生长方向下一格放新头部。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        TWISTING_VINE,
        Direction.Up,
    );

    // 骨粉应成功（返 true），且生长方向下一格应出现 twisting_vines 头部方块。
    test.assert(
        used,
        `bonemeal on twisting_vines should succeed (used=${used}). ` +
            `wiki:65 bonemeal accelerates growth. Check IGrowable impl on GrowingPlantHeadBlock.`,
    );
    test.assert(
        getTypeId(test, TWISTING_GROW_TARGET) === "minecraft:twisting_vines",
        `bonemeal growth: expected twisting_vines at ${JSON.stringify(TWISTING_GROW_TARGET)} (upward), ` +
            `got ${getTypeId(test, TWISTING_GROW_TARGET)}`,
    );

    test.succeed();
}

// 垂泪藤骨粉加速生长（wiki:73 明文骨粉可加速生长）。
// 布局：(3,4,1) stone 上方支撑，(3,3,1) weeping_vines(age=0)，下方 (3,2,1) 为 air。
// 骨粉后断言：useItemOnBlock 返 true + (3,2,1) 出现 weeping_vines（生长延伸到下方）。
function weepingVinesBonemealGrows(test: Test): void {
    test.setBlockType("minecraft:stone", WEEPING_SUPPORT);
    test.setBlockType("minecraft:weeping_vines", WEEPING_VINE);
    test.assert(
        getTypeId(test, WEEPING_VINE) === "minecraft:weeping_vines",
        `weeping_vines should be at ${JSON.stringify(WEEPING_VINE)}, got ${getTypeId(test, WEEPING_VINE)}`,
    );
    test.assert(
        getTypeId(test, WEEPING_GROW_TARGET) === "minecraft:air",
        `grow target should be air before growth, got ${getTypeId(test, WEEPING_GROW_TARGET)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对垂泪藤 useItemOnBlock 骨粉 → IGrowable::grow 在生长方向下一格放新头部。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        WEEPING_VINE,
        Direction.Up,
    );

    // 骨粉应成功（返 true），且生长方向下一格应出现 weeping_vines 头部方块。
    test.assert(
        used,
        `bonemeal on weeping_vines should succeed (used=${used}). ` +
            `wiki:73 bonemeal accelerates growth. Check IGrowable impl on GrowingPlantHeadBlock.`,
    );
    test.assert(
        getTypeId(test, WEEPING_GROW_TARGET) === "minecraft:weeping_vines",
        `bonemeal growth: expected weeping_vines at ${JSON.stringify(WEEPING_GROW_TARGET)} (downward), ` +
            `got ${getTypeId(test, WEEPING_GROW_TARGET)}`,
    );

    test.succeed();
}

export function registerTwistingVinesTests(): void {
    GameTest.register("BlockBehaviorTests", "twisting_vines_grows_upward_via_random_tick", twistingVinesGrowsUpwardViaRandomTick)
        .structureName("gametests:glass_pit")
        .maxTicks(320);
    GameTest.register("BlockBehaviorTests", "weeping_vines_grows_downward_via_random_tick", weepingVinesGrowsDownwardViaRandomTick)
        .structureName("gametests:glass_pit")
        .maxTicks(320);
    GameTest.register("BlockBehaviorTests", "twisting_vines_bonemeal_grows", twistingVinesBonemealGrows)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "weeping_vines_bonemeal_grows", weepingVinesBonemealGrows)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
