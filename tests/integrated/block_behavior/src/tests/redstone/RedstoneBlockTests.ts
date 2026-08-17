// 红石块（redstone_block）电源行为 GameTest。
//
// wiki mechanism_红石块.txt#用途（:44-55）：红石块是「不能关闭的红石电源」，持续输出信号：
//   - 为毗邻的红石线提供 15 点信号强度（:49）；
//   - 激活附着其上的红石火把使其熄灭（:52）；
//   - 激活毗邻且朝向正确的机械元件（如红石灯）（:53）；
//   - 表现类似强充能方块，但「不会充能毗邻的红石导体」（:55），且本身不是红石导体。
// 红石块被破坏后掉落自身（:37）。挖掘工具为镐（:33）。
//
// C++ 链路：RedstoneBlock（redstone/RedstoneBlock.cpp）无 state，纯电源：
//   - getWeakPower（:41-50）：无条件返回 RedstonePower::MAX_POWER（15），全向弱信号。
//   - getStrongPower（:52-61）：无条件返回 15，全向强信号（强充能相邻方块）。
//   - 无 onBlockAdded/neighborChanged/tick override（继承基类空操作），放置即供电（无 state 切换）。
// 此前各红石测试（红石灯/铜灯/发射器等）均把红石块当作「测试电源」间接使用，但从未单独验证
//   红石块本身的电源行为（全向强弱信号）。本文件补此覆盖。
//
// 派发链路：setBlockType("minecraft:redstone_block", pos) → setBlockState flags=3 → 邻居
//   neighborChanged/updatePostPlacement（机械元件如红石灯走 neighborChanged→isPowered 立即点亮）。
//   红石块注册 BlockItemRegistry.cpp:206 registerSimpleBlock(REDSTONE_BLOCK, "redstone_block")。
//
// 电源选择说明：本测试被测对象即红石块本身，故用红石块作被测 + 红石灯/红石火把作「信号探测器」
//   （红石灯 lit 翻转验证弱信号激活机械元件；红石火把熄灭验证强充能 + 非门）。与 RedstoneLampTests/
//   CopperBulbTests 用红石块作电源的范式同源，但此处红石块是被测方。
//
// 测试覆盖（3 个场景，覆盖 wiki 持续全向供电核心行为，可跨服务端对比）：
//   1. 红石块激活相邻红石灯：红石块水平相邻红石灯 → 灯 lit=true（getWeakPower 全向 15 激活机械元件）。
//   2. 红石块激活相邻红石火把使其熄灭：红石块在火把支撑方块下方强充能支撑方块 → 火把 lit=false
//      （getStrongPower 强充能 + 红石火把非门「附着被充能则熄灭」）。
//   3. 红石块破坏不崩溃：放红石块 → setBlockType air → 位置变 air（基类 onBlockRemoved 空操作）。
//
// 关键约束：
// 1. 场景 1「先灯后块」：先放红石灯（默认 lit=false），再水平相邻放红石块。放红石块 flags=3 →
//    邻居红石灯 neighborChanged → isPowered(红石块 weakPower 15)>0=true → 立即 setBlockState(lit=true)。
//    同 RedstoneLampTests 场景 1 范式，pollUntilSucceed 轮询 lit=true 留余量。
// 2. 场景 2 红石火把布局：支撑方块 A=(3,2,1) stone，火把=(3,3,1)（A 顶面），红石块=(3,1,1)（A 下方）。
//    红石块对 A 输出 strongPower(Down 方向)=15 → A 从 Down 被强充能 → RedstoneTorchBlock::shouldBeOff
//    检查 belowPos=A 的 isSidePowered(A, Down)=true → 火把应熄灭。分阶段：先放火把（A 下方无电源，
//    lit=true 稳定）→ 再放红石块强充能 A → 火把经 REDSTONE_DELAY tick 熄灭 lit=false。
//    RedstoneTorchBlock::shouldBeOff（RedstoneTorchBlock.cpp:70-76）检查 belowPos 从 Down 强充能。
// 3. 场景 3 放红石块后 setBlockType air 破坏：RedstoneBlock 无 onBlockRemoved override（基类空操作），
//    位置变 air。断言变 air（链路不崩溃）。破坏掉落物非确定（项目范式不验证掉落物实体），仅测变 air。
// 4. 读红石灯 lit / 红石火把 lit 用 getState("lit" as any) 绕过 BlockStateSuperset 白名单（同红石灯范式）。
// 5. glass_pit 结构 7×5×7（x,z∈[0,6], y∈[0,4]），y=0 为 glass 底，y=1..4 为 air 空腔。场景 2 用
//    y=1（红石块）/y=2（支撑 stone）/y=3（火把）三层，均在空腔内。
//
// 不测「红石块为毗邻红石线提供 15 信号」：红石线连接形态 + 信号衰减判定链路复杂，RedstoneWireTests
//   已覆盖红石线，本文件聚焦红石块直接供电机械元件 + 强充能非门。TODO: 可补
//   redstone_block_powers_adjacent_wire_15。
// 不测「红石块不会充能毗邻红石导体」：需布置红石导体（如石头）+ 比较器探测其充能等级，链路复杂，跳过。
//   TODO: 可补 redstone_block_does_not_power_adjacent_conductor。
// 不测「红石块被活塞推动」：活塞推动链路依赖 BlockEntity tick 调度，GameTest 不可靠（见 PistonTests）。
// 不测「破坏掉落自身」：破坏掉落物非确定（项目范式不验证掉落物实体）。TODO: 待脚本侧破坏掉落物测试
//   范式完善后补 redstone_block_drops_itself。
//
// 跨服务端：redstone_block 方块名两端一致。红石块全向强弱信号 15 + 激活机械元件/红石火把行为两端与
//   vanilla 一致。无 state，无 one-sided 行为。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mechanism_红石块.txt#用途（持续全向供电，激活火把/机械元件）
// Ref: RedstoneBlock.cpp（getWeakPower/getStrongPower 全向 15，无 state 纯电源）
// Ref: RedstoneLampBlock.cpp（neighborChanged isPowered 立即点亮，作弱信号探测器）
// Ref: RedstoneTorchBlock.cpp（shouldBeOff 检查 belowPos 从 Down 强充能，作强充能+非门探测器）
// Ref: RedstoneLampTests.ts / CopperBulbTests.ts（红石块作电源范式 + pollUntilSucceed 轮询）
// Ref: GrindstoneTests.ts（破坏不崩溃范式：setBlockType air + 断言变 air）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。y=0 为 glass 底，y=1..4 为 air 空腔。
// 场景 1：红石块 (3,2,1) + 红石灯 (4,2,1)（水平相邻，均 y=2 空腔内）。
// 场景 2：红石块 (3,1,1) + 支撑 stone (3,2,1) + 红石火把 (3,3,1)（三层 y=1/2/3，红石块强充能支撑方块）。
// 场景 3：红石块 (3,2,1)（破坏不崩溃）。

// 读取 (x,y,z) 方块 lit state（bool，红石灯/红石火把共用 LIT）。返回 null 表示读取失败或非目标方块。
function getLit(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("lit" as any);
    return typeof value === "boolean" ? value : null;
}

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 场景 1：红石块激活相邻红石灯——红石块水平相邻红石灯 → 灯 lit=true。
//
// 布局：(3,2,1) 放红石块，（4,2,1）放红石灯（默认 lit=false）。放红石块 flags=3 → 邻居红石灯
//   neighborChanged → isPowered(红石块 weakPower 15)>0=true ≠ isLit(false) → 立即 setBlockState(lit=true)。
//
// 判定：pollUntilSucceed 轮询 (4,2,1) lit===true（红石块 getWeakPower 全向 15 激活机械元件红石灯）。
//
// 此场景验证 wiki「红石块激活毗邻机械元件」+ RedstoneBlock::getWeakPower 全向 15：红石块本身是被测方，
//   红石灯作弱信号探测器。与 RedstoneLampTests 场景 1 同构（那里红石块是电源、灯是被测；此处反过来，
//   红石块是被测、灯是探测器），共同验证红石块→红石灯供电链路。
function redstoneBlockPowersAdjacentLamp(test: Test): void {
    // (3,2,1) 放红石块（被测电源，getWeakPower 全向 15）。
    test.setBlockType("minecraft:redstone_block", { x: 3, y: 2, z: 1 });
    // (4,2,1) 放红石灯（弱信号探测器，默认 lit=false）。
    test.setBlockType("minecraft:redstone_lamp", { x: 4, y: 2, z: 1 });

    // 轮询断言红石灯 lit===true（红石块全向弱信号 15 激活红石灯，neighborChanged 同步触发，留余量）。
    pollUntilSucceed(
        test,
        () => getLit(test, 4, 2, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `redstone_lamp lit: should be true when adjacent to redstone_block, got ${getLit(test, 4, 2, 1)}`);
            },
        },
    );
}

// 场景 2：红石块激活相邻红石火把使其熄灭——红石块强充能火把支撑方块 → 火把 lit=false。
//
// 布局：支撑 stone A=(3,2,1)，红石火把=(3,3,1)（A 顶面）。先放火把（A 下方 (3,1,1) 无电源，火把 lit=true
//   稳定）。再 (3,1,1) 放红石块 → 红石块对 A 输出 strongPower(Down)=15 → A 从 Down 被强充能 →
//   RedstoneTorchBlock::shouldBeOff（belowPos=A，isSidePowered(A, Down)=true）→ 火把应熄灭。
//   neighborChanged 触发 updateState → shouldBeLit=false ≠ isLit(true) → scheduleBlockTick(REDSTONE_DELAY)
//   → tick 执行 setBlockState(lit=false)。
//
// 判定：分阶段——先 pollUntilSucceed 确认火把初始 lit=true（无电源点亮），再 runAtTickTime 放红石块
//   强充能，再 pollUntilSucceed 轮询 lit===false（火把非门「附着被充能则熄灭」）。
//
// 此场景验证 wiki「红石块激活附着其上的红石火把使其熄灭」+ RedstoneBlock::getStrongPower 全向 15 强充能：
//   红石块强充能支撑方块 A，红石火把（附在 A 上）检测到 A 被充能而熄灭（非门逻辑）。红石火把作强充能
//   + 非门探测器。注意 wiki :57「附着在红石块上的红石火把刚放置亮一段再熄灭」是 BE 特殊时序，本场景
//   红石块在火把支撑方块下方（非火把直接附着红石块），避开该特殊时序，测稳定的非门逻辑。
function redstoneBlockExtinguishesAdjacentTorch(test: Test): void {
    // 支撑 stone A=(3,2,1)（火把下方支撑，顶面 sturdy）。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });
    // 红石火把=(3,3,1)（A 顶面，setBlockType 绕过 isValidPosition 直接放置，onBlockAdded 检查初始状态：
    // A 下方 (3,1,1) 此时为 air 无电源 → shouldBeOff=false → shouldBeLit=true=isLit → 无需翻转，保持 lit=true）。
    test.setBlockType("minecraft:redstone_torch", { x: 3, y: 3, z: 1 });

    // 阶段 1：等火把初始 lit=true 稳定（无电源点亮，非门默认输出）。REDSTONE_DELAY 后 onBlockAdded
    //   调度的 tick 若有也应在此时已执行且保持 lit=true。
    pollUntilSucceed(
        test,
        () => getLit(test, 3, 3, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(false, `redstone_torch lit: should be true initially (no power), got ${getLit(test, 3, 3, 1)}`);
            },
        },
    );

    // 阶段 2：放红石块强充能支撑方块 A。runAtTickTime 在火把 lit=true 稳定后（tick 8）放红石块。
    //   (3,1,1) 放红石块 → 对 A=(3,2,1) 输出 strongPower(Down)=15 → A 从 Down 强充能 → 火把
    //   neighborChanged（红石块放置 flags=3 派发邻居更新到 A，A 再通知其上火把？实际红石块在 A 下方，
    //   红石块放置派发邻居更新给 A，A 的 neighborChanged；火把在 A 上方，需 A 变化或红石系统更新通知火把）。
    test.runAtTickTime(8, () => {
        if (getLit(test, 3, 3, 1) !== true) {
            test.assert(false, `redstone_torch should be lit=true before powering support, got ${getLit(test, 3, 3, 1)}`);
            return;
        }
        // (3,1,1) 放红石块强充能 A。红石块放置 flags=3 → 邻居 A neighborChanged + 红石系统更新传播到火把。
        test.setBlockType("minecraft:redstone_block", { x: 3, y: 1, z: 1 });
    });

    // 阶段 3：轮询火把 lit===false（A 被强充能 → 火把非门熄灭，经 REDSTONE_DELAY tick 延迟）。
    //   startTick=12 留红石块放置 + REDSTONE_DELAY 延迟余量。
    pollUntilSucceed(
        test,
        () => getLit(test, 3, 3, 1) === false,
        {
            startTick: 12,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `redstone_torch lit: should be false when support block powered by redstone_block, got ${getLit(test, 3, 3, 1)}`);
            },
        },
    );
}

// 场景 3：红石块破坏不崩溃——放红石块 → setBlockType air 破坏 → 位置变 air。
//
// 布局：(3,2,1) 放红石块。setBlockType("minecraft:air", (3,2,1)) 破坏红石块 → RedstoneBlock 无
//   onBlockRemoved override（基类 Block::onBlockRemoved 空操作），位置变 air。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（红石块已破坏，链路不崩溃）。
//
// 此场景验证红石块破坏链路安全性：放红石块后破坏，基类 onBlockRemoved 不崩溃，位置正确变 air。
//   破坏掉落物（wiki :37 破坏掉落自身）非确定（项目范式不验证掉落物实体），故仅测变 air。
function redstoneBlockBreaksWhenRemoved(test: Test): void {
    test.setBlockType("minecraft:redstone_block", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:redstone_block", `redstone_block should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏红石块 → 基类 onBlockRemoved 空操作 → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言红石块 (3,2,1) 已破坏变 air（链路不崩溃）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `redstone_block pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerRedstoneBlockTests(): void {
    GameTest.register("BlockBehaviorTests", "redstone_block_powers_adjacent_lamp", redstoneBlockPowersAdjacentLamp)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "redstone_block_extinguishes_adjacent_torch", redstoneBlockExtinguishesAdjacentTorch)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "redstone_block_breaks_when_removed", redstoneBlockBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
