// 信标金字塔等级检测行为 GameTest。
//
// wiki tech_信标.txt#激活：信标需放置在由铁块/金块/绿宝石块/钻石块/下界合金块组成的金字塔顶部。
//   金字塔有 4 个等级：1级=3×3底座，2级=5×5+3×3，3级=7×7+5×5+3×3，4级=9×9+7×7+5×5+3×3。
//   金字塔层数决定信标等级（1-4），等级影响可用效果与作用范围。
//   组成金字塔的矿物方块可以相同也可以不同，不影响信标功能。若金字塔受损信标会失效（等级降低）。
// wiki tech_信标.txt#数据值：信标有方块实体，保存金字塔等级等附加数据。
// wiki tech_信标.txt#红石：信标比较器输出信号强度等于金字塔等级（1-4），未激活=0。
//
// C++ 链路：
//   - BeaconBlock（functional/BeaconBlock.cpp）：hasBlockEntity()=true + createBlockEntity 创建 BeaconEntity。
//     放置信标方块时 ServerWorld::setBlockState（ServerWorld.cpp:822）检测 hasBlockEntity 后调用
//     createBlockEntity 创建方块实体并注册到区块，使后续 tick 经 ServerWorld::tickBlockEntities() 驱动。
//   - BeaconEntity::tick（BeaconEntity.cpp:216-246）：每 tick 自增 m_tickCount，每 80 tick
//     （BEACON_UPDATE_INTERVAL=80）调用 _updateLevels 重新检测金字塔等级。
//   - BeaconEntity::_updateLevels（BeaconEntity.cpp:248-274）：遍历 1-4 层，第 l 层在 y=pos.y-l，
//     检查 (2l+1)×(2l+1) 个基座方块是否为有效矿物方块（isValidBeaconBaseBlock：IRON/GOLD/DIAMOND/
//     EMERALD/NETHERITE_BLOCK）。连续层数达标则 newLevel=该层数，遇不合格层则 break。setLevel 写回 m_level。
//     注释明确"金字塔检测不需要检查天空可见性"。
//   - Block.beaconLevel（Cubium 专有扩展，MinecraftModuleFactory.cpp Block 类注册）：经 ScriptBlockRef.world
//     调 getBlockEntity(pos) 取 BlockEntity，dynamic_cast 到 BeaconEntity 后读 getLevel()。非信标返 -1。
//
// 测试覆盖（3 个场景，覆盖 wiki 金字塔等级检测 1/3/4 级 + 无金字塔等级 0 核心行为）：
//   1. beacon_level1_pyramid：1 级铁块金字塔（3×3 底座）+ 信标 → 等 80+ tick _updateLevels → level=1。
//   2. beacon_level3_pyramid：3 级铁块金字塔（7×7+5×5+3×3）+ 信标 → 等 80+ tick → level=3。
//   3. beacon_level4_pyramid：4 级铁块金字塔（9×9+7×7+5×5+3×3）+ 信标 → 等 80+ tick → level=4。
//   4. beacon_no_pyramid_zero：信标无金字塔基座 → 等 80+ tick _updateLevels → level=0。
//
// 关键约束：
// 1. beacon_pit 结构 11×7×11（helper 相对坐标 x,z∈[0,10], y∈[0,6]），外壳玻璃内部全 air。
//    4 级金字塔 9×9 底座居中：信标放 (5,5,5)，第1层 9×9 在 y=4（x,z∈[1,9]），第2层 7×7 在 y=3
//    （x,z∈[2,8]），第3层 5×5 在 y=2（x,z∈[3,7]），第4层 3×3 在 y=1（x,z∈[4,6]）。beacon_pit 内部
//    x,z∈[1,9],y∈[1,5] 全 air（11×7×11 外壳玻璃），9×9 底座恰居中（x,z∈[1,9]）不越界。
// 2. BeaconEntity::tick 每 80 tick 调 _updateLevels。m_tickCount 从 0 自增，第 80 tick
//    （m_tickCount%80==0）首次检测。放置方块后方块实体从下一 tick 开始 tick，故需等约 81+ tick 确保
//    _updateLevels 已执行。用 pollUntilSucceed 轮询 beaconLevel，规避 80tick 延迟与时序不确定性。
// 3. 读 beaconLevel 用 test.getBlock(pos).beaconLevel（Cubium 专有 Block 扩展属性，返回 0-4 或 -1）。
// 4. Cubium 信标 _updateLevels 不检查天空可见性（BeaconEntity.cpp:250 注释），故 beacon_pit 顶部玻璃
//    不影响金字塔检测。wiki 说的"上方无遮挡"在 Cubium 仅影响光束渲染（_updateBeamSegments），不影响等级。
// 5. 不同矿物方块（铁/金/钻石/绿宝石/下界合金）均可组成金字塔，本测试用铁块（iron_block）。
//
// 不测「信标效果应用」：_applyEffects 需 m_primaryEffect 有值，而 m_primaryEffect 默认 nullopt，需玩家
//   通过 GUI 消耗矿物设置。信标 GUI 经 ContainerType::Beacon + openContainer 链路，无头 GameTest 模式下
//   GUI 交互链路复杂且脚本侧无法设置 primaryEffect。TODO: 待信标 GUI 脚本交互链路打通后补效果应用测试。
// 不测「光束颜色」：_updateBeamSegments 是客户端渲染，无头模式无渲染，不可验证。
// 不测「金字塔受损等级降低」：需先建金字塔再破坏部分方块验证 level 递减，场景复杂跳过。
//   TODO: 待信标测试稳定后补金字塔受损等级动态变化测试。
// 不测「混合矿物金字塔」：wiki 明确"组成金字塔的矿物方块可以相同也可以不同"，纯铁块金字塔已验证
//   isValidBeaconBaseBlock 检测链路，混合矿物无新行为点。
//
// 跨服务端：beacon 方块名两端一致。beaconLevel 是 Cubium 专有 Block 扩展（基岩无），仅 Cubium 端可读。
//   beacon_pit 结构是 Cubium 专有（基岩无），跨端对比时基岩需用其他结构或程序化构建金字塔。
//   金字塔等级检测行为与 vanilla 一致，可跨服务端对比（基岩端需适配结构与 level 读取方式）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_信标.txt#激活（金字塔等级 1-4，4级=9×9底座）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_信标.txt#数据值（方块实体保存等级）
// Ref: BeaconBlock.cpp:80-94（hasBlockEntity/createBlockEntity 创建方块实体；getComparatorInputOverride 返回 level）
// Ref: BeaconEntity.cpp:216-274（tick 每 80 tick _updateLevels；_updateLevels 遍历 1-4 层检测矿物方块）
// Ref: MinecraftModuleFactory.cpp Block 类（beaconLevel 属性：getBlockEntity→dynamic_cast BeaconEntity→getLevel）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// beacon_pit 结构尺寸 11×7×11（helper 相对坐标 x,z∈[0,10], y∈[0,6]）。
// 外壳玻璃，内部 x,z∈[1,9] y∈[1,5] 全 air。信标居中放 (5,5,5)，金字塔基座在其下方 y=4..1 层。

// 读取 (x,y,z) 信标方块的 pyramid level。返回 null 表示读取失败或非信标方块（beaconLevel=-1）。
function getBeaconLevel(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z }) as any;
    if (block === undefined || block === null) {
        return null;
    }
    const level = block?.beaconLevel;
    // beaconLevel 返回 -1 表示非信标方块，统一映射为 null（区分"非信标"与"等级 0"）。
    return typeof level === "number" ? (level < 0 ? null : level) : null;
}

// 在 (cx,cy,cz) 下方放置以 为顶的 level 级金字塔基座（铁块）。
// 第 l 层（l=1..level）在 y=cy-l 层，尺寸 (2l+1)×(2l+1)，居中。
// 信标本身放。
function placeIronPyramid(test: Test, cx: number, cy: number, cz: number, level: number): void {
    for (let l = 1; l <= level; l++) {
        const y = cy - l;
        const half = l; // (2l+1) 边长，半边长 l
        for (let dx = -half; dx <= half; dx++) {
            for (let dz = -half; dz <= half; dz++) {
                test.setBlockType("minecraft:iron_block", { x: cx + dx, y, z: cz + dz });
            }
        }
    }
}

// 轮询断言信标等级等于预期值。
// 信标 _updateLevels 每 80 tick 检测一次，startTick=85 留足首次 _updateLevels（第 80 tick）执行时间。
function assertBeaconLevel(test: Test, beaconPos: { x: number; y: number; z: number }, expected: number): void {
    pollUntilSucceed(
        test,
        () => getBeaconLevel(test, beaconPos.x, beaconPos.y, beaconPos.z) === expected,
        {
            startTick: 85,
            interval: 10,
            maxTick: 220,
            onTimeout: () => {
                const level = getBeaconLevel(test, beaconPos.x, beaconPos.y, beaconPos.z);
                test.assert(
                    false,
                    `beacon at (${beaconPos.x},${beaconPos.y},${beaconPos.z}) level should be ${expected}, `
                        + `got level=${level} (if null: beacon block entity not created [hasBlockEntity/`
                        + `createBlockEntity missing]; if 0: _updateLevels not detecting pyramid [tick not `
                        + `running or isValidBeaconBaseBlock false]; if wrong value: pyramid layer detection off)`,
                );
            },
        },
    );
}

// 场景 1：1 级铁块金字塔（3×3 底座）+ 信标 → level=1。
//
// 布局（beacon_pit 11×7×11）：
//   信标 (5,5,5)
//   第1层 3×3 铁块 y=4（x,z∈[4,6]）
//   下方 y=3..1 留 air（不构成第2层，故 level=1）
function beaconLevel1Pyramid(test: Test): void {
    placeIronPyramid(test, 5, 5, 5, 1);
    test.setBlockType("minecraft:beacon", { x: 5, y: 5, z: 5 });
    assertBeaconLevel(test, { x: 5, y: 5, z: 5 }, 1);
}

// 场景 2：3 级铁块金字塔（7×7+5×5+3×3）+ 信标 → level=3。
//
// 布局（beacon_pit 11×7×11）：
//   信标 (5,5,5)
//   第1层 7×7 铁块 y=4（x,z∈[2,8]）
//   第2层 5×5 铁块 y=3（x,z∈[3,7]）
//   第3层 3×3 铁块 y=2（x,z∈[4,6]）
//   下方 y=1 留 air（不构成第4层，故 level=3）
function beaconLevel3Pyramid(test: Test): void {
    placeIronPyramid(test, 5, 5, 5, 3);
    test.setBlockType("minecraft:beacon", { x: 5, y: 5, z: 5 });
    assertBeaconLevel(test, { x: 5, y: 5, z: 5 }, 3);
}

// 场景 3：4 级铁块金字塔（9×9+7×7+5×5+3×3）+ 信标 → level=4。
//
// 布局（beacon_pit 11×7×11）：
//   信标 (5,5,5)
//   第1层 9×9 铁块 y=4（x,z∈[1,9]）—— beacon_pit 内部 x,z∈[1,9] 恰好容纳 9×9 居中
//   第2层 7×7 铁块 y=3（x,z∈[2,8]）
//   第3层 5×5 铁块 y=2（x,z∈[3,7]）
//   第4层 3×3 铁块 y=1（x,z∈[4,6]）
function beaconLevel4Pyramid(test: Test): void {
    placeIronPyramid(test, 5, 5, 5, 4);
    test.setBlockType("minecraft:beacon", { x: 5, y: 5, z: 5 });
    assertBeaconLevel(test, { x: 5, y: 5, z: 5 }, 4);
}

// 场景 4：信标无金字塔基座 → level=0。
//
// 布局（beacon_pit 11×7×11）：信标 (5,5,5) 直接放 air 上（下方 y=4 是 air，无矿物基座）。
//   _updateLevels 检测第1层（y=4 的 3×3）无有效矿物方块 → newLevel=0 → setLevel(0)。
function beaconNoPyramidZero(test: Test): void {
    // 信标 (5,5,5) 下方无金字塔（y=4 是 beacon_pit 内部 air）。
    test.setBlockType("minecraft:beacon", { x: 5, y: 5, z: 5 });
    assertBeaconLevel(test, { x: 5, y: 5, z: 5 }, 0);
}

export function registerBeaconTests(): void {
    GameTest.register("BlockBehaviorTests", "beacon_level1_pyramid", beaconLevel1Pyramid)
        .structureName("gametests:beacon_pit")
        .maxTicks(260);
    GameTest.register("BlockBehaviorTests", "beacon_level3_pyramid", beaconLevel3Pyramid)
        .structureName("gametests:beacon_pit")
        .maxTicks(260);
    GameTest.register("BlockBehaviorTests", "beacon_level4_pyramid", beaconLevel4Pyramid)
        .structureName("gametests:beacon_pit")
        .maxTicks(260);
    GameTest.register("BlockBehaviorTests", "beacon_no_pyramid_zero", beaconNoPyramidZero)
        .structureName("gametests:beacon_pit")
        .maxTicks(260);
}
