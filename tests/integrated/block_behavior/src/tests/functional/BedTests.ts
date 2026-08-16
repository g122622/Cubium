// 床睡眠占用行为 GameTest。
//
// wiki block_床.txt#睡觉：玩家在主世界夜晚右键床可睡觉，睡觉时床标记为「已占用」（OCCUPIED=true）。
//   - 主世界（bedWorks=true）右键床 → 睡眠 → 床头 OCCUPIED 翻 true；交互脚部则脚部也翻 true。
//   - 下界/末地（bedWorks=false）右键床 → 爆炸（不测，涉方块破坏+火焰非确定）。
//   - 床已占用时再右键 → 显示「这张床已被占用」消息，返 Success（不改变 state）。
//   - 睡眠需满足前置：玩家靠近床（水平≤3 垂直≤2）、床未被阻挡（床头/床尾上方及起床位有空间）、
//     时间允许睡眠（夜晚 dayTime≥12542 或雷暴）、非创造模式还需周围无怪物。
//
// C++ 链路：BedBlock（functional/BedBlock.cpp）有 HORIZONTAL_FACING + BED_PART（Foot/Head）+
//   OCCUPIED 三个 state。默认 state（:83-86）：facing=North, part=Foot, occupied=false。
//   - onBlockActivated（:334-427）：取维度 bedWorks，false→爆炸分支（移除床+createExplosion）；
//     true→算床头位置（脚部 offset(facing)）→ 取 headState，无 OCCUPIED 属性则 Pass → 已占用则
//     sendStatusMessage + Success → 否则 player.tryStartSleeping(bedHeadPos)：
//       OK → headState.with(OCCUPIED,true) setBlockState；交互脚部则 foot 也 with(OCCUPIED,true) → Success；
//       非 OK → sendStatusMessage(错误) + Success（不改 state）。
//   - onBlockPlacedBy（:429-439）：脚部放置时在 offset(facing) 放 head 半（setBlockType 不走此流程）。
//   - updatePostPlacement（:138-165）：配对半床缺失（facingState.isAir()）→ 当前床自毁变 air；
//     facing==otherDir 且 facingState 有 OCCUPIED → 同步 occupied。
//   - ServerPlayer::trySleep（ServerPlayer.cpp:375-441）：前置检查——isPlayerNearBed（水平3垂直2）、
//     isBedObstructed（床头/床尾上方+起床位有站立空间）、canSleepAtTime（晴天 dayTime∈[12542,23459]
//     或雷暴）、非创造 isBedSurroundedByMonsters。全通过→startSleeping→返 OK。
//   - canSleepAtTime（SleepManager.cpp:45-63）：晴天 dayTime∈[12542,23459]；降雨∈[12010,23991]；雷暴任意。
//
// 派发链路：SimulatedPlayer 继承 ServerPlayer，tryStartSleeping 委托 trySleep（前置检查）。
//   GameTest batch("night") 设 dayTime=18000（Cubium GameTestServer.cpp:329 TimeOfDayEnvironment），
//   满足 canSleepAtTime 夜间范围。interactWithBlock（空手右键）→ onBlockActivated 睡眠分支。
//
// 测试覆盖（1 个场景，覆盖 wiki 主世界夜晚睡眠占用 OCCUPIED 翻转核心确定行为）：
//   1. 夜晚睡眠占用：双半红床（foot facing=North + head）+ batch("night") + interactWithBlock（空手右键
//      脚部）→ 床头+脚部 OCCUPIED 翻 true，返 true。
//
// 关键约束：
// 1. 双半床手动放置（setBlockType 不走 onBlockPlacedBy 自动配对 head）：foot 用 setBlockType 放默认
//    state（part=Foot facing=North），head 用 setBlockWithStates 放 part=head facing=north。facing=North
//    时床头在脚部 offset(North)=(0,0,-1)（z-1 方向）。
// 2. 床头/床尾上方及起床位须有站立空间（isBedObstructed）：glass_pit y=3 默认空气满足。
// 3. 玩家须靠近床（isPlayerNearBed 水平≤3 垂直≤2）：SimulatedPlayer 站 foot 南侧相邻格。
// 4. batch("night") 设 dayTime=18000 满足 canSleepAtTime（否则返 NOT_POSSIBLE_NOW 不翻 OCCUPIED）。
// 5. 创造模式 SimulatedPlayer 跳过 isBedSurroundedByMonsters（:423 守卫）。
// 6. updatePostPlacement 自毁风险：放第一半时另一 half 是 air 可能触发自毁。需验证双半放置顺序。
//    若自毁，调整放置策略（先 head 后 foot / 用 flags 跳过邻居更新）。
//
// 不测「下界/末地爆炸」：涉 createExplosion 方块破坏+火焰，非确定，跳过。
// 不测「已占用床再右键」：需先成功睡眠占用一张床（本场景），再用第二个玩家右键——但 OCCUPIED 已 true
//   时 onBlockActivated 走 sendStatusMessage+Success（不改 state），判定仅返回值 true，单薄。跳过。
//   TODO: 待需扩展「占用态再交互」覆盖时补。
// 不测「设置重生点」：setSpawnPoint 副作用无脚本断言 API，跳过。
//
// 跨服务端：床 red_bed 方块名两端一致，occupied state 名两端一致，睡眠占用行为与 vanilla 一致。
//   注意：基岩 BDS 无 interactWithBlock（Cubium 补全）+ batch("night") 语义需基岩侧另行验证，
//   场景在基岩侧为 one-sided。但 OCCUPIED 翻转行为本身两端语义一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_床.txt#睡觉（夜晚右键床睡眠，标记占用）
// Ref: BedBlock.cpp（onBlockActivated 睡眠分支 OCCUPIED 翻转；onBlockPlacedBy 自动配对 head；updatePostPlacement 配对缺失自毁）
// Ref: ServerPlayer.cpp（trySleep 前置：isPlayerNearBed/isBedObstructed/canSleepAtTime/isBedSurroundedByMonsters）
// Ref: SleepManager.cpp（canSleepAtTime 晴天[12542,23459]/降雨[12010,23991]/雷暴任意）
// Ref: GameTestServer.cpp（batch("night") → TimeOfDayEnvironment(18000) 真正设夜晚）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 床脚 foot (3,2,2) facing=North，床头 head (3,2,1)（foot.offset(North)=(0,0,-1) → z-1）。
// 支撑：foot 下 (3,1,2) stone，head 下 (3,1,1) stone。
// SimulatedPlayer 站 (3,2,3)（foot 南侧相邻，靠近床，dy=0.5≤2 满足 isPlayerNearBed）。

// 读取床 occupied state（bool）。返回 null 表示读取失败或非床。
function getBedOccupied(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("occupied" as any);
    return typeof value === "boolean" ? value : null;
}

// 读取 (x,y,z) 方块 typeId。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 放支撑 + 双半红床：foot (3,2,2) part=foot facing=north，head (3,2,1) part=head facing=north。
// setBlockType 放 foot 默认 state（part=Foot facing=North）；setBlockWithStates 放 head（part=head facing=north）。
// facing=North 时床头在脚部 z-1 方向。
function placeBedSetup(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 2 }); // foot 支撑
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // head 支撑
    // foot 半（默认 part=Foot facing=North）。
    test.setBlockType("minecraft:red_bed", { x: 3, y: 2, z: 2 });
    // head 半（part=head facing=north，配对 foot）。
    (test as unknown as {
        setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
    }).setBlockWithStates("minecraft:red_bed", { x: 3, y: 2, z: 1 }, "part=head,facing=north");
}

// 场景 1：夜晚睡眠占用——双半红床 + batch("night") + interactWithBlock（空手右键脚部）→
//   床头+脚部 OCCUPIED 翻 true，返 true。
//
// 布局：foot (3,2,2) + head (3,2,1)，SimulatedPlayer 站 (3,2,3) 靠近 foot。
// batch("night") 设 dayTime=18000 满足 canSleepAtTime。interactWithBlock 空手右键 foot →
//   onBlockActivated：bedWorks(主世界 true) → bedHeadPos=foot.offset(North)=(3,2,1) →
//   headState 有 OCCUPIED 且未占用 → tryStartSleeping(headPos) → trySleep 前置全通过 → OK →
//   head with(OCCUPIED,true) + foot with(OCCUPIED,true) → Success。
//
// 判定：interactWithBlock 返 true（Success），head (3,2,1) 与 foot (3,2,2) 的 occupied 均 === true。
// one-sided：依赖 Cubium batch("night") + interactWithBlock 绑定。
function bedOccupiedWhenSleepingAtNight(test: Test): void {
    placeBedSetup(test);
    // 放置后断言双半床存活（updatePostPlacement 未自毁）且 occupied=false。
    test.assert(getBlockTypeId(test, 3, 2, 2) === "minecraft:red_bed", `foot should be red_bed after place, got ${getBlockTypeId(test, 3, 2, 2)}`);
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:red_bed", `head should be red_bed after place, got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getBedOccupied(test, 3, 2, 1) === false, `head occupied should be false before, got ${getBedOccupied(test, 3, 2, 1)}`);
    test.assert(getBedOccupied(test, 3, 2, 2) === false, `foot occupied should be false before, got ${getBedOccupied(test, 3, 2, 2)}`);

    // 新 spawn SimulatedPlayer 站 foot 南侧相邻格（靠近床，满足 isPlayerNearBed）。
    const farmer = test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "farmer");

    // interactWithBlock 空手右键 foot (3,2,2) → onBlockActivated 睡眠分支 → OCCUPIED 翻 true。
    // interactWithBlock 为 Cubium 补全的 SimulatedPlayer 方法（类型定义未声明），用 as any 绕过类型检查。
    const used = (farmer as unknown as { interactWithBlock: (pos: unknown, dir: unknown) => boolean })
        .interactWithBlock({ x: 3, y: 2, z: 2 }, Direction.Up);
    test.assert(used, "interactWithBlock should return true when sleeping in bed at night");

    // 判定：床头 (3,2,1) 与脚部 (3,2,2) occupied 均 === true（睡眠占用）。
    test.assert(getBedOccupied(test, 3, 2, 1) === true, `head occupied should be true after sleeping, got ${getBedOccupied(test, 3, 2, 1)}`);
    test.assert(getBedOccupied(test, 3, 2, 2) === true, `foot occupied should be true after sleeping, got ${getBedOccupied(test, 3, 2, 2)}`);

    test.succeed();
}

export function registerBedTests(): void {
    GameTest.register("BlockBehaviorTests", "bed_occupied_when_sleeping_at_night", bedOccupiedWhenSleepingAtNight)
        .structureName("gametests:glass_pit")
        .batch("night")
        .maxTicks(100);
}
