// 水与熔岩流体交互行为 GameTest（凝固产物：黑曜石/圆石/石头）。
//
// 覆盖 wiki「熔岩和水的互动」全部四条核心规则 + 一条对照（源下方触水无反应）。
// 黑曜石场景（源水平/上方触水）已在 LiquidTests.lava_source_plus_water_makes_obsidian 覆盖，
// 本文件补充圆石（流水平/上方触水）、石头（流向下流入水）、源下方触水无反应。
//
// wiki 规则（docs\minecraft-wiki-source\minecraft_wiki\tech_熔岩.txt#熔岩和水的互动，第125-140行）：
//   - 熔岩源水平/上方触碰水 → 黑曜石（已覆盖，本文件不重复）
//   - 熔岩源下方触碰水 → 无反应（本文件对照测试）
//   - 熔岩流水平/上方触碰水 → 圆石（本文件）
//   - 熔岩流下方触碰水 → 无反应（同源下方，反应只看"水平/上方"方向，下方跳过）
//   - 熔岩流向下流入水 → 石头（本文件，流体 tick 延迟路径）
//   - 熔岩（源/流）+ 水平/上方蓝冰 + 下方灵魂土 → 玄武岩（三方条件复杂，暂不覆盖，TODO）
//
// C++ 链路：
//   1) 源/流水平上方触水凝固（LiquidBlock::reactWithNeighbors，LiquidBlock.cpp:194-259）：
//      由 onBlockAdded(:123)/neighborChanged(:142)/updatePostPlacement(:171) 同步触发。遍历六向
//      邻居（跳过 Down），若邻居含水流体 → 熔岩方块自身 setBlockState：源→OBSIDIAN、流→COBBLESTONE。
//      同步（同 tick setBlockState flags=3），返回 false 阻止后续流体 tick 调度。
//   2) 流向下流入水凝固石头（LavaFluid::flowInto，LavaFluid.cpp:230-250）：流体 tick 路径
//      （scheduleFluidTick → FlowingFluid::tick → flowAround → flowInto），dir==Down 且目标格含水 →
//      目标水格 setBlockState(STONE)。延迟 = LavaFluid::getTickDelay（主世界 30 tick / 下界 10 tick）。
//
// 流动熔岩放置：Cubium 只注册 minecraft:lava 一个方块 typeId（无独立 flowing_lava 方块），流体流动
// 状态由 LiquidBlock 的 LEVEL_0_15 state 表达——blockLevel=0 源、1-7 流（blockLevelToFluidLevel
// 反向映射）。setBlockType("minecraft:lava") 放 defaultState(level=0 源)；放流动熔岩用 Cubium 专有
// setBlockWithStates("minecraft:lava", pos, "level=1")（level∈[1,7] 均为流，isSource=false）。
// wiki 历史记录印证（tech_熔岩.txt:482）：「熔岩的 level 为6时接触水也会生成圆石」。
//
// 跨服务端：wiki（tech_熔岩.txt:532）称「流动的水和熔岩现在使用与 Java 版相匹配的混合机制」（BE
// 1.20.40 起），故凝固产物映射两端一致，可跨服务端对比。但放流动熔岩的 API（setBlockWithStates）
// 是 Cubium 专有（基岩 BDS 无），基岩端无法直接放指定 level 的流动熔岩——故圆石场景基岩端归类 one-sided。
// 石头场景（源自然流淌向下入水）两端都走流体 tick 自然产生，可跨服务端对比（仅比最终产物）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_熔岩.txt#熔岩和水的互动

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的内部 air 空腔，y=3..4 air+顶部框架。
// 方块测试在内部 air 层操作。

// 放置流动熔岩（level=1，非源）的跨服务端辅助。Cubium 用 setBlockWithStates；基岩 BDS 无此方法，
// 故基岩端无法直接放指定 level 流动熔岩（圆石场景在基岩归类 one-sided，仅 Cubium 跑）。
function placeFlowingLava(test: Test, pos: { x: number; y: number; z: number }): void {
    // Cubium 专有 setBlockWithStates 放 minecraft:lava + level=1（流动熔岩，isSource=false）。
    // level∈[1,7] 均为流，取 1 即可。底层 setBlockState 触发 onBlockAdded → reactWithNeighbors。
    test.setBlockWithStates("minecraft:lava", pos, "level=1");
}

// 熔岩流水平触碰水 → 圆石（wiki tech_熔岩.txt#熔岩和水的互动 第130行）。
//
// C++ 链路：LiquidBlock::reactWithNeighbors（LiquidBlock.cpp:194-259）跳过 Down 方向遍历邻居，
// 流动熔岩（isSource=false）+ 水平水邻居 → 自身 setBlockState(COBBLESTONE)。流动熔岩放置
// （setBlockWithStates level=1）触发 onBlockAdded → reactWithNeighbors 同步检测到水邻居 → 变圆石。
//
// 布局：先 (4,1,3) 放水（稳定静止源），再 (3,1,3) 放流动熔岩（level=1）。熔岩 onBlockAdded 同步
// 检测 East 邻居 (4,1,3) 水 → 变圆石。反应同 tick 同步完成。
//
// 跨服务端：放流动熔岩需 setBlockWithStates（Cubium 专有），基岩 BDS 无该 API，基岩端 one-sided。
function lavaFlowPlusWaterMakesCobblestone(test: Test): void {
    // 先 (4,1,3) 放水源（静止，作熔岩的 East 邻居）。下方 (4,0,3) glass 底支撑。
    test.setBlockType("minecraft:water", { x: 4, y: 1, z: 3 });

    // (3,1,3) 放流动熔岩（level=1，非源）。onBlockAdded 同步 reactWithNeighbors 检测 East 邻居水 →
    // 流动熔岩变圆石。
    placeFlowingLava(test, { x: 3, y: 1, z: 3 });

    // 断言熔岩流格 (3,1,3) 已凝固为圆石（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:cobblestone", { x: 3, y: 1, z: 3 }, true);
}

// 熔岩源下方触碰水 → 无反应（wiki tech_熔岩.txt#熔岩和水的互动 第129行，对照测试）。
//
// C++ 链路：reactWithNeighbors（LiquidBlock.cpp:214-217）遍历方向时 `if (dir == Down) continue;`
// 显式跳过下方，故熔岩源下方有水时不触发凝固，熔岩源保持。这是 wiki 明确的「源下方触水无反应」。
//
// 布局：(3,2,3) 放熔岩源，(3,1,3) 下方放水。熔岩 onBlockAdded → reactWithNeighbors 跳过 Down 邻居
// (3,1,3) 的水 → 无凝固，熔岩源保持。注意：熔岩源静止不流淌（源不产生流动状态变化），故不会向下
// 流入水（flowInto Down→石头 仅对"流动"熔岩生效；源在水面之上保持静止源）。
//
// 判定：等待若干 tick 后断言 (3,2,3) 仍是熔岩源（未凝固成黑曜石）。用 runAtTickTime 延迟判定，
// 排除"凝固延迟"的假阴性——若凝固会同步发生（reactWithNeighbors 同步），等待 40 tick 仍是熔岩
// 即确证无反应。不用 succeedWhen 检查"熔岩存在"（spawn 第 1 tick 就成立，无法排除"延迟凝固"）。
//
// 跨服务端：两端源下方触水均无反应（reactWithNeighbors 语义两端一致），可跨服务端对比。
function lavaSourceAboveWaterNoReaction(test: Test): void {
    // (3,1,3) 放水（位于熔岩源下方）。
    test.setBlockType("minecraft:water", { x: 3, y: 1, z: 3 });

    // (3,2,3) 放熔岩源（位于水正上方）。onBlockAdded reactWithNeighbors 跳过 Down → 不凝固。
    test.setBlockType("minecraft:lava", { x: 3, y: 2, z: 3 });

    // 40 tick（2 秒，远超任何同步凝固窗口）后断言熔岩源仍存在（未变黑曜石）。
    // 熔岩源静止不流淌，不会向下流入水（源不流动）。若凝固会同步发生，40 tick 后仍是熔岩即确证无反应。
    test.runAtTickTime(40, () => {
        const block = test.getBlock({ x: 3, y: 2, z: 3 });
        test.assert(
            block !== undefined && block.typeId === "minecraft:lava",
            `lava source above water should not solidify (got ${block?.typeId}), expected minecraft:lava`,
        );
        test.succeed();
    });
}

// 熔岩流向下流入水 → 石头（wiki tech_熔岩.txt#熔岩和水的互动 第132行）。
//
// C++ 链路：LavaFluid::flowInto（LavaFluid.cpp:230-250）流体 tick 路径。熔岩源 scheduleFluidTick
// （onBlockAdded:127，getTickDelay 主世界 30 tick）→ FlowingFluid::tick → flowAround → 向下 flowInto，
// dir==Down 且目标格含水 → 目标水格 setBlockState(STONE)。注意：生成的是「目标水格变石头」，
// 即石头出现在原水所在格，而非熔岩源格。
//
// 布局：(3,3,3) 放熔岩源，(3,2,3) 是 air，(3,1,3) 放水。熔岩源放后 scheduleFluidTick（30 tick），
// 到期流淌：向下 flowInto (3,2,3) air（无水，正常流动产生 flowing_lava），继续向下 flowInto (3,1,3)
// 水 → (3,1,3) 变石头。需等待流体 tick 链路完成（约 30 tick 起算，留余量到 100 tick）。
//
// 时序注意：熔岩源先要流淌到 (3,2,3)（产生 flowing_lava），再向下流入 (3,1,3) 水。源向下流淌是
// fluid tick 触发，(3,2,3) 流入是同一次 flowAround 的 Down 分支；(3,1,3) 的 flowInto Down→石头
// 紧接其后。整个链路在一次 fluid tick 内完成（30 tick 后）。但 flowInto 正常流动（非 Down 入水）
// 会 setBlockState flowing_lava 到 (3,2,3)，可能先与 (3,1,3) 水水平触——(3,2,3) flowing_lava 的
// South/East/West 邻居是 air/glass，无水，故不触发圆石；Down 邻居 (3,1,3) 是水，flowInto Down→石头。
// 最终 (3,1,3) 变石头。
//
// 判定：pollUntilSucceed 轮询 (3,1,3) 出现 stone。maxTick=120 留足 30 tick 延迟 + 流淌链路余量。
//
// 跨服务端：源自然流淌向下入水两端都走流体 tick 自然产生，可跨服务端对比（仅比最终产物 stone）。
function lavaFlowsDownIntoWaterMakesStone(test: Test): void {
    // (3,1,3) 放水（熔岩将向下流入此格）。
    test.setBlockType("minecraft:water", { x: 3, y: 1, z: 3 });

    // (3,3,3) 放熔岩源（位于水上方 2 格，中间 (3,2,3) air）。源 scheduleFluidTick（30 tick）后流淌。
    test.setBlockType("minecraft:lava", { x: 3, y: 3, z: 3 });

    // 轮询断言 (3,1,3) 变石头（熔岩流向下流入水）。流体 tick 延迟约 30 tick，留余量到 120 tick。
    pollUntilSucceed(
        test,
        () => {
            const block = test.getBlock({ x: 3, y: 1, z: 3 });
            return block !== undefined && block.typeId === "minecraft:stone";
        },
        {
            startTick: 20,
            interval: 10,
            maxTick: 120,
            onTimeout: () => {
                const b1 = test.getBlock({ x: 3, y: 1, z: 3 });
                const b2 = test.getBlock({ x: 3, y: 2, z: 3 });
                const b3 = test.getBlock({ x: 3, y: 3, z: 3 });
                test.assert(
                    false,
                    `lava did not flow down into water to make stone: (3,1,3)=${b1?.typeId} (3,2,3)=${b2?.typeId} (3,3,3)=${b3?.typeId}`,
                );
            },
        },
    );
}

export function registerWaterLavaInteractionTests(): void {
    GameTest.register("BlockBehaviorTests", "lava_flow_plus_water_makes_cobblestone", lavaFlowPlusWaterMakesCobblestone)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "lava_source_above_water_no_reaction", lavaSourceAboveWaterNoReaction)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "lava_flows_down_into_water_makes_stone", lavaFlowsDownIntoWaterMakesStone)
        .structureName("gametests:glass_pit")
        .maxTicks(200);
}
