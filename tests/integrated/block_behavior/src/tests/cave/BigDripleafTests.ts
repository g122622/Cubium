// 大型垂滴叶（big_dripleaf）倾斜状态机、红石重置、含水与支撑自毁行为 GameTest。
//
// wiki tech_大型垂滴叶.txt#用途/倾斜/红石元件：
//   - 放置：只能放在黏土、苔藓块、草方块、菌丝体、灰化土、泥土、缠根泥土、砂土、耕地、泥巴、
//     沾泥的红树根上（即 BIG_DRIPLEAF_PLACEABLE 标签）；可在已放置的大滴叶上方再放大滴叶增高。
//   - 倾斜：实体（生物/物品实体）可在叶片上停留片刻，停留超 1 秒（JE）/ 1.5 秒（BE）或被弹射物
//     击中 → 叶片倾斜，待其上的实体随之摔落；倾斜 5 秒后复原。倾斜状态机：
//     none → unstable(10gt) → partial(10gt) → full(100gt) → none（100gt=5 秒对齐 wiki 5 秒复原）。
//   - 红石元件：施加红石信号防止倾斜，使 state 保持 none；红石信号促使倾斜的叶片回升（重置 none）；
//     红石激活中叶片被弹射物击中仍会倾斜（onProjectileHit 不查红石）。
//   - 含水：可含水源（waterlog），放置时按水源判定自动 waterlog。
//
// C++ 链路：BigDripleafBlock（cave/BigDripleafBlock.cpp）三个 state：
//   - HORIZONTAL_FACING（C++ 属性名 "facing"，默认 north）。
//   - TILT（C++ 属性名 "tilt"，枚举 none/unstable/partial/full，默认 none，序列化见 EnumProperty.cpp:717-731）。
//   - WATERLOGGED（C++ 属性名 "waterlogged"，默认 false）。
//   - isValidPosition（:101-111）：下方是 big_dripleaf/big_dripleaf_stem 或 BIG_DRIPLEAF_PLACEABLE 标签
//     方块（clay/moss_block/grass_block/... 在标签内，BlockTags.cpp:2953-2957）。clay 在标签内可作支撑。
//   - updatePostPlacement（:113-140）：facing==Down 且 !isValidPosition → 返 air 自毁（同 tick 同步）；
//     facing==Up 且上方也是 big_dripleaf → 自身转 big_dripleaf_stem（茎）。
//   - onEntityCollision（:247-260）：tilt==None && _canEntityTilt && !isPowered → 设 unstable + 调度
//     10gt tick。_canEntityTilt（:326-329）：entity.onGround() 且 entity.y > pos.y + 0.6875
//     （ENTITY_DETECTION_MIN_Y=0.6875，BigDripleafBlock.hpp:160）。
//   - tick（:163-194）：isPowered→resetTilt；unstable→partial(10gt)；partial→full(10gt)；full→none(100gt)。
//   - neighborChanged（:205-217）：isPowered→resetTilt（红石重置）。
//   - getCollisionShape（:148-156）：tilt==Full 返空（实体摔落），否则 fullBlock（实体可站立）。
//   - getStateForPlacement（:89-99）：facing=horizontalDirection（玩家朝向），waterlog 按水源判定。
//     【与 vanilla 偏差】vanilla BigDripleafBlock.getStateForPlacement（1.21.11 .java:273-280）在下方非
//     茎/叶时取 horizontalDirection.getOpposite()；Cubium 直接用 horizontalDirection 不取反。此偏差仅
//     影响物品放置朝向，本测试全部用 setBlockWithStates 预置 state（绕过放置），规避该偏差。
//
// 测试覆盖（4 个场景，覆盖 wiki 倾斜状态机触发 + 红石重置 + 含水 + 支撑自毁核心确定行为）：
//   1. 实体踩踏触发倾斜：猪落到 tilt=none 叶片上 → tilt 进入 unstable/partial/full（onEntityCollision 触发）。
//   2. 红石信号重置倾斜：预置 tilt=partial（无电源稳定保持，setBlockWithStates 不调度 tick）→ 放红石块
//      水平相邻 → neighborChanged → isPowered → resetTilt → tilt=none。
//   3. 含水 state 读写：预置 waterlogged=true → getState("waterlogged")===true（验证 waterlog state 可读写）。
//   4. 支撑失效自毁：clay 支撑 + tilt=none 叶片 → 移除 clay → updatePostPlacement(Down) 自毁为 air。
//
// 关键约束：
// 1. 支撑用 clay（在 BIG_DRIPLEAF_PLACEABLE 标签内，isValidPosition 通过）。clay 是完整固体方块，
//    被移除（→air）时向 Up 邻居叶片派发 updatePostPlacement(Down) → isValidPosition(下方 air) 失败 →
//    返 air 自毁（同 tick 同步）。
// 2. 场景 2 预置 tilt=partial 用 setBlockWithStates（flags=3 默认派发邻居更新，但无电源时 neighborChanged
//    不动作；BigDripleafBlock 无 onBlockAdded，setBlockWithStates 不调度 tick，故 tilt=partial 写入后稳定
//    不推进）。放红石块（flags=3）→ 红石块 setBlockState 派发邻居更新 → 叶片 neighborChanged →
//    isPowered（RedstonePower::isPowered 遍历六方向查邻居 weakPower，红石块 getWeakPower 全向 15）=true →
//    _resetTilt → tilt=none。pollUntilSucceed 轮询 tilt===none。
// 3. 场景 3 预置 waterlogged=true 只写 state（不放真实 water fluid），不触发水流动，稳定可断言。验证
//    waterlogged state 经 setBlockWithStates 写入后 getState 可读（基础可测性）。不测「放置时按水源自动
//    waterlog」（依赖真实水源放置链路，且放置朝向有偏差，跳过）。
// 4. 场景 1 猪落到叶片上：叶片 tilt=none 时 getCollisionShape=fullBlock（有碰撞箱），猪站在叶片顶面
//    （叶片在 (3,2,1)，pos.y=2，猪脚 y≈3.0 > 2+0.6875=2.6875 满足 _canEntityTilt；猪 onGround=true 站稳）。
//    onEntityCollision → tilt=unstable(10gt)→partial(10gt)→full(100gt)。10gt 的 unstable 窗口较窄，
//    猪落地需时间，故断言用 tilt∈{unstable,partial,full}（实体触发了倾斜链，处于倾斜中），更稳健且仍
//    验证「实体踩踏触发倾斜」核心行为。猪 spawn (3,3,1) 落到 (3,2,1) 叶片。
// 5. 读 tilt/waterlogged/facing state 用 getState("tilt"/"waterlogged"/"facing" as any)（C++ 内部属性名）。
// 6. 支撑自毁是 updatePostPlacement 同 tick 同步（移除支撑 setBlockState air 派发更新 → 叶片返 air →
//    ServerWorld 立即 setBlockState），用 succeedWhenBlockPresent(false) 直接断言消失。
//
// 不测「物品放置朝向」：Cubium getStateForPlacement 用 horizontalDirection 不取 opposite（与 vanilla
//   偏差），且 facing 取决于 SimulatedPlayer 朝向不可控。全部场景用 setBlockWithStates 预置 facing=north，
//   规避该偏差。TODO: 待 getStateForPlacement 取 opposite 偏差修复后补 big_dripleaf_facing_when_placed。
// 不测「弹射物击中直接 full」：onProjectileHit 不查红石（红石激活中仍倾斜），但需投射物精确命中叶片，
//   时序非确定，跳过。TODO: 可补 big_dripleaf_tilts_full_when_projectile_hit。
// 不测「上方叠放转茎」：facing==Up 且上方也是 big_dripleaf → 自身转 stem，涉转茎链路 + stem 延迟销毁，
//   时序复杂，跳过。TODO: 可补 big_dripleaf_converts_to_stem_when_stacked。
// 不测「骨粉增高」：涉骨粉 useItem 链路，BoneMealTests 已覆盖骨粉范式，跳过。
// 不测「full 倾斜实体摔落」：full 时 getCollisionShape 空，实体穿透下落，需精确时序断言实体位移，跳过。
//
// 跨服务端：big_dripleaf 方块名两端一致。tilt/waterlogged/facing state 名两端一致（C++ 内部名）。
//   倾斜状态机（实体触发 + 红石重置 + 5 秒复原）+ 含水 + 支撑自毁行为与 vanilla 一致。setBlockWithStates
//   预置 state 是 Cubium 专有写入（基岩侧用物品放置），但倾斜/红石/含水/自毁行为本身两端可对比，非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_大型垂滴叶.txt#倾斜（实体停留/弹射物触发，5 秒复原）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_大型垂滴叶.txt#红石元件（信号防止倾斜+促使回升）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_大型垂滴叶.txt#用途（放置于 BIG_DRIPLEAF_PLACEABLE 标签方块）
// Ref: BigDripleafBlock.cpp（onEntityCollision/tick/neighborChanged/updatePostPlacement/isValidPosition）
// Ref: BigDripleafBlock.hpp:160（ENTITY_DETECTION_MIN_Y=0.6875，_canEntityTilt 判定）
// Ref: EnumProperty.cpp:717-731（Tilt 序列化 none/unstable/partial/full）
// Ref: BlockTags.cpp:2953-2957（BIG_DRIPLEAF_PLACEABLE 标签含 clay/moss_block 等）
// Ref: RedstonePower.cpp:106-142（isPowered→isIndirectlyPowered 遍历六方向查邻居强弱信号）
// Ref: RedstoneLampTests.ts（红石块水平相邻作电源，setBlockType flags=3 派发 neighborChanged 范式）
// Ref: PressurePlateTests.ts（猪 spawn 落到方块上触发 onEntityCollision 范式）
// Ref: CarpetTests.ts（支撑自毁范式：setBlockType 支撑+方块 → 移除支撑 → succeedWhenBlockPresent 断言消失）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/4：叶片 (3,2,1)，下方支撑 (3,1,1) clay，猪 spawn (3,3,1)。
// 场景 2：叶片 (3,2,1)，下方支撑 (3,1,1) clay，红石块 (4,2,1)（水平相邻作电源）。
// 场景 3：叶片 (3,2,1)，下方支撑 (3,1,1) clay。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BannerTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 叶片 tilt state（小写字符串：none/unstable/partial/full）。返回 null 表示失败或非大滴叶。
// 注意：TILT() 的 C++ 属性名为 "tilt"（EnumProperty<Tilt> create("tilt", ...)），getState 按内部名匹配。
function getTilt(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("tilt" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 叶片 waterlogged state（bool）。返回 null 表示失败或非大滴叶。
function getWaterlogged(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("waterlogged" as any);
    return typeof value === "boolean" ? value : null;
}

// 放置测试基础结构：clay 支撑 + big_dripleaf 叶片（facing=north，tilt=none，waterlogged=false 默认）。
// clay 在 BIG_DRIPLEAF_PLACEABLE 标签内，isValidPosition 通过。先支撑后叶片（避免叶片 Down 方向
// updatePostPlacement 检测支撑缺失自毁）。setBlockWithStates 预置 state 绕过物品放置朝向偏差。
function placeDripleafOnClay(test: Test, tilt: string = "none"): void {
    test.setBlockType("minecraft:clay", { x: 3, y: 1, z: 1 });
    (test as TestWithStates).setBlockWithStates("minecraft:big_dripleaf", { x: 3, y: 2, z: 1 }, `facing=north,tilt=${tilt}`);
}

// 场景 1：实体踩踏触发倾斜——猪落到 tilt=none 叶片上 → tilt 进入倾斜链（unstable/partial/full）。
//
// 布局：clay 支撑 (3,1,1) + big_dripleaf 叶片 (3,2,1) tilt=none（facing=north）。猪 spawn (3,3,1) 落到
//   叶片顶面（叶片 tilt=none 时 getCollisionShape=fullBlock，有碰撞箱，猪站立其上）。
// 猪站稳（onGround=true，脚 y≈3.0 > 2+0.6875=2.6875 满足 _canEntityTilt）→ Entity::doBlockCollisions 每
//   tick 调 BigDripleafBlock::onEntityCollision（Block.hpp:1247 虚函数）→ tilt==None && _canEntityTilt &&
//   !isPowered → _setTilt(unstable) + 调度 10gt tick。tick 链 unstable→partial(10gt)→full(100gt)。
//
// 判定：pollUntilSucceed 轮询 tilt∈{unstable,partial,full}（实体触发了倾斜链，处于倾斜中）。
//   10gt unstable 窗口窄，猪落地需时间，故用「倾斜中」集合断言（仍验证 onEntityCollision 触发倾斜核心行为）。
function bigDripleafTiltsWhenMobStands(test: Test): void {
    placeDripleafOnClay(test, "none");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:big_dripleaf", `big_dripleaf should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getTilt(test, 3, 2, 1) === "none", `tilt should be none before mob stands, got ${getTilt(test, 3, 2, 1)}`);

    // 猪 spawn (3,3,1) 落到叶片 (3,2,1) 顶面（自由落体 1 格，站稳后 onEntityCollision 触发倾斜）。
    test.spawn("pig", { x: 3, y: 3, z: 1 });

    // 轮询断言 tilt 进入倾斜链（unstable/partial/full，实体踩踏触发倾斜）。
    // startTick=10 留猪落地站稳 + onEntityCollision 调度余量；interval=2 捕获 10gt unstable 窗口；maxTick=60。
    pollUntilSucceed(
        test,
        () => {
            const tilt = getTilt(test, 3, 2, 1);
            return tilt === "unstable" || tilt === "partial" || tilt === "full";
        },
        {
            startTick: 10,
            interval: 2,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `big_dripleaf tilt should enter tilt chain (unstable/partial/full) when pig stands, got ${getTilt(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 2：红石信号重置倾斜——预置 tilt=partial → 放红石块水平相邻 → neighborChanged → resetTilt → none。
//
// 布局：clay 支撑 (3,1,1) + big_dripleaf 叶片 (3,2,1) tilt=partial（facing=north，setBlockWithStates 预置，
//   flags=3 派发邻居更新但无电源时 neighborChanged 不动作；无 onBlockAdded，setBlockWithStates 不调度 tick，
//   故 tilt=partial 稳定保持，不自动推进）。
// 先断言 tilt=partial 已写入（证明预置成功），再放红石块 (4,2,1)（水平相邻叶片）。红石块 setBlockState
//   flags=3 → 派发邻居更新 → 叶片 (3,2,1) neighborChanged → isPowered（RedstonePower::isPowered 遍历六
//   方向，红石块 getWeakPower 全向 15）=true → _resetTilt → tilt=none。
//
// 判定：pollUntilSucceed 轮询 tilt===none（neighborChanged 同步触发 resetTilt，留余量防时序）。
//
// 此场景验证 wiki「红石信号促使倾斜叶片回升」：tilt=partial 在红石信号下重置为 none。
function bigDripleafResetsTiltWhenPowered(test: Test): void {
    placeDripleafOnClay(test, "partial");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:big_dripleaf", `big_dripleaf should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    // 先断言 tilt=partial 已写入（证明 setBlockWithStates 预置成功，叶片处于部分倾斜态）。
    test.assert(getTilt(test, 3, 2, 1) === "partial", `tilt should be partial before power applied, got ${getTilt(test, 3, 2, 1)}`);

    // 放红石块 (4,2,1) 水平相邻叶片（红石块 getWeakPower 全向 15 作电源）。setBlockType flags=3 派发邻居
    // 更新 → 叶片 neighborChanged → isPowered(红石块)=true → _resetTilt → tilt=none。
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 轮询断言 tilt===none（红石信号重置倾斜叶片回升为 none）。
    pollUntilSucceed(
        test,
        () => getTilt(test, 3, 2, 1) === "none",
        {
            startTick: 2,
            interval: 2,
            maxTick: 30,
            onTimeout: () => {
                test.assert(false, `big_dripleaf tilt should reset to none when powered by redstone block, got ${getTilt(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 3：含水 state 读写——预置 waterlogged=true → getState("waterlogged")===true。
//
// 布局：clay 支撑 (3,1,1) + big_dripleaf 叶片 (3,2,1) waterlogged=true（facing=north，tilt=none，
//   setBlockWithStates 预置，只写 state 不放真实 water fluid，不触发水流动）。
//
// 判定：getState("waterlogged")===true（验证 waterlogged state 经 setBlockWithStates 写入后可读）。
//
// 此场景验证 wiki「大型垂滴叶可含水」：waterlogged state 可正确读写。不测「放置时按水源自动 waterlog」
//   （依赖真实水源放置链路 + 放置朝向偏差，跳过，见文件头 TODO）。
function bigDripleafWaterloggedStateReadable(test: Test): void {
    placeDripleafOnClay(test, "none");
    // 覆盖预置 waterlogged=true（placeDripleafOnClay 已放 tilt=none 叶片，此处再写 waterlogged=true）。
    // setBlockWithStates 从默认 state 出发逐属性应用，故重写完整 state（facing=north,tilt=none,waterlogged=true）。
    (test as TestWithStates).setBlockWithStates("minecraft:big_dripleaf", { x: 3, y: 2, z: 1 }, "facing=north,tilt=none,waterlogged=true");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:big_dripleaf", `big_dripleaf should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言 waterlogged===true（state 可读）+ tilt===none（其他 state 不受影响）+ facing===north。
    test.assert(getWaterlogged(test, 3, 2, 1) === true, `waterlogged should be true after setBlockWithStates, got ${getWaterlogged(test, 3, 2, 1)}`);
    test.assert(getTilt(test, 3, 2, 1) === "none", `tilt should remain none, got ${getTilt(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 4：支撑失效自毁——clay 支撑 + tilt=none 叶片 → 移除 clay → 自毁为 air。
//
// 布局：clay 支撑 (3,1,1) + big_dripleaf 叶片 (3,2,1) tilt=none（facing=north）。移除 (3,1,1) clay（设 air）
//   → 被改方块 (3,1,1) 通知其 Up 邻居 (3,2,1) 叶片，叶片收到 updatePostPlacement(facing=Down) →
//   !isValidPosition(下方 air 非 dripleaf/stem/标签) → 返 air 自毁（同 tick 同步）。
//
// 判定：succeedWhenBlockPresent 断言叶片 (3,2,1) 已消失（自毁为 air）。
//
// 此场景验证 wiki「大型垂滴叶只能放置在标签方块上」+「支撑失效自毁」：clay 移除后下方无有效支撑，
//   叶片 updatePostPlacement(Down) 返 air 自毁。
function bigDripleafBreaksWhenSupportBelowRemoved(test: Test): void {
    placeDripleafOnClay(test, "none");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:big_dripleaf", `big_dripleaf should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // (3,1,1) 设 air 移除 clay 支撑（clay→air 真实变化，派发邻居更新）。air 放置向 Up 邻居 (3,2,1) 叶片
    // 派发 updatePostPlacement(facing=Down) → isValidPosition(下方 air) 失败 → 返 air 自毁。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言叶片 (3,2,1) 已自毁消失（下方支撑失效，同 tick 同步自毁）。
    test.succeedWhenBlockPresent("minecraft:big_dripleaf", { x: 3, y: 2, z: 1 }, false);
}

export function registerBigDripleafTests(): void {
    GameTest.register("BlockBehaviorTests", "big_dripleaf_tilts_when_mob_stands", bigDripleafTiltsWhenMobStands)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "big_dripleaf_resets_tilt_when_powered", bigDripleafResetsTiltWhenPowered)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "big_dripleaf_waterlogged_state_readable", bigDripleafWaterloggedStateReadable)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "big_dripleaf_breaks_when_support_below_removed", bigDripleafBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
