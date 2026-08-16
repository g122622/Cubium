// 绊线钩（tripwire_hook）绊线链 ATTACHED 连接行为 GameTest。
//
// wiki mechanism_绊线.txt#绊线钩：绊线钩成对使用，两个朝向相对的绊线钩之间用绊线（tripwire）连接
//   构成绊线链。当链完整（两端钩朝向相对 + 中间绊线连通）时，两端钩的 ATTACHED state 翻 true
//   （视觉上钩"拉紧"连接绊线）。ATTACHED 是纯几何连接判定（不依赖实体触发），实体进入绊线区域才
//   触发 POWERED（信号输出，本测试不测，涉实体非确定）。
//   - 绊线钩于 1.3.1 加入，1.21.11 已包含，属 vanilla 正式特性。
//
// C++ 链路：TripWireHookBlock（redstone/TripWireHookBlock.cpp）继承 Block，有 HORIZONTAL_FACING +
//   POWERED + ATTACHED 三个 state。默认 state（:81-84）：facing=North, powered=false, attached=false。
//   - onBlockAdded（:112-137）：检查 attachPos=pos.offset(opposite(facing)) 是否 isSolid，无支撑则
//     spawnItemEntity 掉落绊线钩物品 + setBlockState(nullptr) 自毁。故钩须贴 solid 方块侧面放置。
//   - neighborChanged（:139-178）：再次检查支撑，有支撑则 _calculateState(world, pos, facing, state, true)。
//   - _calculateState（:262-345）：_checkForTripwire 沿 facing 找对面钩 → foundChain。若 foundChain，
//     沿 facing 遍历中间格子确认全是 tripwire 且末端是对面钩（朝向 opposite）。shouldPower=foundChain
//     && isTripwirePowered && !shouldBreak（实体触发才 POWERED，本测试无实体 → POWERED 保持 false）。
//     若 foundChain != wasConnected → withConnected(foundChain) 设 ATTACHED=foundChain + setBlockState。
//   - _checkForTripwire（:347-382）：沿 facing 最多 42 格，遇绊线钩且朝向 opposite → foundChain=true
//     （outOtherHookPos）；遇 tripwire 继续；否则 return false。
//   - ATTACHED state 翻转在 neighborChanged 同步触发（_calculateState 直接 setBlockState），无延迟。
//
// 测试覆盖（2 个场景，覆盖 wiki 绊线链 ATTACHED 连接核心确定行为，双向闭合）：
//   1. 链连通 ATTACHED：两朝向相对绊线钩 + 中间绊线 → 两端钩 ATTACHED 翻 true（foundChain）。
//   2. 链断开 ATTACHED 复位：链连通后移除中间绊线 → 两端钩 _calculateState foundChain=false →
//      ATTACHED 翻回 false（链断开复位）。
//
// 关键约束：
// 1. 绊线钩须贴 solid 方块侧面放置——attachPos=pos.offset(opposite(facing)) 须 isSolid，否则
//    onBlockAdded 掉落物品 + 自毁。两端钩各需一个 stone 支撑方块在 attachPos 方向。
//    - 钩 A (3,2,2) facing=east → attachPos=(2,2,2) 须 stone。
//    - 钩 B (5,2,2) facing=west → attachPos=(6,2,2) 须 stone。
// 2. 钩 facing 须用 setBlockWithStates 设（setBlockType 默认 facing=North，attachPos=South 不匹配布局）。
//    setBlockWithStates("minecraft:tripwire_hook", pos, "facing=east")（C++ 属性名 "facing"，值 east/west）。
// 3. 中间绊线 (4,2,2) tripwire 连通两钩。绊线放置触发两端钩 neighborChanged → _calculateState →
//    _checkForTripwire 沿 facing 找对面钩（中间绊线连通）→ foundChain=true → ATTACHED=true。
//    绊线自身须下方 solid 支撑（TripWireBlock::neighborChanged 检查 belowPos isSolid，无支撑掉落+自毁），
//    故 (4,1,2) 放 stone 支撑绊线。
// 4. ATTACHED 翻转在 neighborChanged 同步触发，pollUntilSucceed 留余量防多级传播时序。
// 5. 读 ATTACHED state 用 getState("attached" as any)（C++ 属性名 "attached"，BooleanProperty）。
// 6. 钩 A/B 朝向须相对（east vs west，opposite），_checkForTripwire 才认定 foundChain。
//
// 不测「实体进入绊线区域触发 POWERED 信号」：TripWireBlock entityInteract 涉实体进入区域时序，非确定，
//   跳过。本测试聚焦 ATTACHED 连接判定（纯几何，确定）。TODO: 可补 tripwire_hook_powers_on_entity。
// 不测「绊线钩背面红石信号输出」：POWERED state 需实体触发绊线，同上非确定，跳过。
// 不测「绊线钩无支撑自毁」：onBlockAdded 无支撑掉落物品+自毁，与 SnowTests/SoulFire 支撑自毁同构但
//   掉落物实体污染测试区域，本文件聚焦 ATTACHED 连接。TODO: 可补 tripwire_hook_drops_without_support。
//
// 跨服务端：tripwire_hook/tripwire 方块名两端一致，attached state 名两端一致，绊线链 ATTACHED 连接
//   判定与 vanilla 一致（两端钩朝向相对 + 中间绊线连通 → ATTACHED=true）。setBlockWithStates 设 facing
//   是 Cubium 专有写入（基岩侧用物品放置朝向），但 ATTACHED 行为本身两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mechanism_绊线.txt#绊线钩（绊线链连接，ATTACHED 拉紧）
// Ref: TripWireHookBlock.cpp（_calculateState: _checkForTripwire foundChain → withConnected 设 ATTACHED）
// Ref: TripWireHookBlock.cpp（_checkForTripwire 沿 facing 找朝向 opposite 的对面钩 + 中间绊线连通）
// Ref: TripWireHookBlock.cpp（onBlockAdded 检查 attachPos isSolid，无支撑掉落+自毁）
// Ref: BedTests.ts（setBlockWithStates 设 facing 范式："facing=east" via cast）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 布局：钩 A (3,2,2) facing=east（attachPos=(2,2,2) stone），钩 B (5,2,2) facing=west
//   （attachPos=(6,2,2) stone），中间绊线 (4,2,2)。两钩朝向相对（east vs west），中间绊线连通。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BedTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取绊线钩 ATTACHED state（bool）。返回 null 表示读取失败或非绊线钩。
function getHookAttached(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("attached" as any);
    return typeof value === "boolean" ? value : null;
}

// 诊断：读取方块 typeId（Cubium Block 暴露 typeId 属性）。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 诊断：读取 facing state（字符串）。
function getFacing(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z });
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : String(value);
}

// 放置绊线链：两端 stone 支撑 + 两朝向相对绊线钩 + 中间绊线（绊线下方须 solid 支撑，否则自毁）。
// 钩 A (3,2,2) facing=east（attachPos=(2,2,2) stone），钩 B (5,2,2) facing=west（attachPos=(6,2,2) stone），
// 中间绊线 (4,2,2)（下方 (4,1,2) stone 支撑，TripWireBlock::neighborChanged 检查下方 isSolid 无支撑自毁）。
// 放绊线触发两端钩 neighborChanged → _calculateState → foundChain → ATTACHED=true。
function placeTripwireChain(test: Test): void {
    // 两端 stone 支撑（attachPos 须 isSolid，否则钩 onBlockAdded 掉落+自毁）。
    test.setBlockType("minecraft:stone", { x: 2, y: 2, z: 2 }); // 钩 A 支撑（attachPos of A facing=east）
    test.setBlockType("minecraft:stone", { x: 6, y: 2, z: 2 }); // 钩 B 支撑（attachPos of B facing=west）

    // 绊线下方支撑（TripWireBlock::neighborChanged 检查 belowPos isSolid，无支撑掉落+自毁）。
    test.setBlockType("minecraft:stone", { x: 4, y: 1, z: 2 }); // 绊线 (4,2,2) 下方支撑

    // 钩 A (3,2,2) facing=east（setBlockWithStates 设 facing，attachPos=(2,2,2) stone 已放，存活）。
    (test as TestWithStates).setBlockWithStates("minecraft:tripwire_hook", { x: 3, y: 2, z: 2 }, "facing=east");

    // 钩 B (5,2,2) facing=west（attachPos=(6,2,2) stone 已放，存活）。
    (test as TestWithStates).setBlockWithStates("minecraft:tripwire_hook", { x: 5, y: 2, z: 2 }, "facing=west");

    // 中间绊线 (4,2,2)（下方 stone 已放，存活）。放绊线通知两端钩 neighborChanged → _calculateState →
    // _checkForTripwire 沿 facing 找对面钩（中间绊线连通）→ foundChain=true → 两端 ATTACHED=true。
    test.setBlockType("minecraft:tripwire", { x: 4, y: 2, z: 2 });
}

// 场景 1：链连通 ATTACHED——两朝向相对绊线钩 + 中间绊线 → 两端钩 ATTACHED 翻 true。
//
// 布局：placeTripwireChain 放绊线链。放中间绊线触发两端钩 neighborChanged → _calculateState：
//   钩 A facing=east，_checkForTripwire 沿 east：(4,2,2)绊线 → (5,2,2) 钩 B facing=west=opposite(east)
//   → foundChain=true → withConnected(true) → ATTACHED=true。钩 B 同理。
//
// 判定：pollUntilSucceed 轮询两端钩 (3,2,2) 和 (5,2,2) ATTACHED===true。
function tripwireHookAttachesWhenChainConnected(test: Test): void {
    placeTripwireChain(test);

    // 轮询断言两端钩 ATTACHED===true（绊线链连通，foundChain）。neighborChanged 同步触发，留余量防时序。
    pollUntilSucceed(
        test,
        () => getHookAttached(test, 3, 2, 2) === true && getHookAttached(test, 5, 2, 2) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 50,
            onTimeout: () => {
                test.assert(false, `tripwire_hook ATTACHED: should be true when chain connected, got A=${getHookAttached(test, 3, 2, 2)}, B=${getHookAttached(test, 5, 2, 2)} | typeA=${getBlockTypeId(test, 3, 2, 2)} typeB=${getBlockTypeId(test, 5, 2, 2)} facingA=${getFacing(test, 3, 2, 2)} facingB=${getFacing(test, 5, 2, 2)} midType=${getBlockTypeId(test, 4, 2, 2)}`);
            },
        },
    );
}

// 场景 2：链断开 ATTACHED 复位——链连通后移除中间绊线 → 两端钩 ATTACHED 翻回 false。
//
// 布局：承接场景 1——两端钩 ATTACHED=true（链连通）。runAtTickTime 等连通稳定后 (4,2,2) 设 air
//   移除中间绊线。air 放置通知两端钩 neighborChanged → _calculateState：
//   钩 A facing=east，_checkForTripwire 沿 east：(4,2,2)=air（非绊线非钩）→ return false →
//   foundChain=false → withConnected(false) → ATTACHED=false（链断开复位）。钩 B 同理。
//
// 判定：pollUntilSucceed 轮询两端钩 ATTACHED===false（中间绊线移除，链断开复位）。
function tripwireHookDetachesWhenChainBroken(test: Test): void {
    placeTripwireChain(test);

    // 等链连通稳定（两端 ATTACHED=true）后移除中间绊线。
    test.runAtTickTime(6, () => {
        if (getHookAttached(test, 3, 2, 2) !== true || getHookAttached(test, 5, 2, 2) !== true) {
            test.assert(false, `tripwire_hook should be attached before breaking chain, got A=${getHookAttached(test, 3, 2, 2)}, B=${getHookAttached(test, 5, 2, 2)}`);
            return;
        }
        // (4,2,2) 设 air 移除中间绊线 → 两端钩 neighborChanged → _calculateState foundChain=false → ATTACHED=false。
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 2 });
    });

    // 轮询断言两端钩 ATTACHED===false（中间绊线移除，链断开复位）。
    pollUntilSucceed(
        test,
        () => getHookAttached(test, 3, 2, 2) === false && getHookAttached(test, 5, 2, 2) === false,
        {
            startTick: 12,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `tripwire_hook ATTACHED: should be false when chain broken, got A=${getHookAttached(test, 3, 2, 2)}, B=${getHookAttached(test, 5, 2, 2)}`);
            },
        },
    );
}

export function registerTripWireHookTests(): void {
    GameTest.register("BlockBehaviorTests", "tripwire_hook_attaches_when_chain_connected", tripwireHookAttachesWhenChainConnected)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "tripwire_hook_detaches_when_chain_broken", tripwireHookDetachesWhenChainBroken)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
}
