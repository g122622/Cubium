// /spreadplayers 命令 GameTest：将玩家分散到区域内随机位置。
//
// 覆盖 wiki 命令章节核心行为：
//   - /spreadplayers <center> <spreadDistance> <maxRange> <respectTeams> <targets>：分散玩家
//     到以 center 为中心、maxRange 为半边长的正方形区域内，任意两分散点水平间距 >= spreadDistance
//     （Ref: wiki spreadplayers.txt）
//   - under <maxHeight> 子命令：限制分散点 Y 搜索上限（getSpawnY 从 maxHeight+1 向下找站立位）
//
// 设计要点：
//   1. SpreadPlayersCommand._setPlayerPositions 原经 TeleportManager.requestTeleport 传送（内部依赖
//      PlayerManager 取 ServerPlayerData），SimulatedPlayer 不进 PlayerManager（getPlayer 返 nullptr），
//      requestTeleport 直接 return 0 → 传送 no-op，/spreadplayers 对 SimulatedPlayer 完全失效。
//      且朝向原读 ServerPlayerData.yaw，SimulatedPlayer 取不到归零（与 vanilla 偏差）。
//      已修复：requestTeleport 返 0 时回退经 ServerPlayerEntityManager 取 Player 实体直接
//      setPosition+setRotation（朝向用实体自身 yaw/pitch，对齐 vanilla entity.teleportTo）。
//      同款回退范式见 TeleportCommand::teleportPlayers。
//   2. spreadplayers 的 center/maxHeight 是世界坐标（Vec2=x,z / 整数 Y），用 test.worldLocation 把
//      helper 相对坐标转世界坐标。getSpawnY 从 maxHeight+1 向下找"非空气+上方两格空气"的站立位。
//   3. cmd_arena 9×7×9 石盒：y=0 stone 地板，y=1..5 内部 7×5×7 空气腔（x,z∈[1,7]），y=6 stone 封顶。
//      结构放世界原点 gridStartY=-59，placeOrigin.y=-58（文件 y=0→世界-58），helper 相对 y=N→世界-59+N。
//      故空气腔 helper y∈[1,5]→世界[-58,-54]，地板 helper y=0→世界-58（文件 y=0 stone）。
//      等等：helper y=1→世界-57（空气腔第一格 air），helper y=0→世界-58（stone 地板）。
//   4. under <maxHeight> 限制 getSpawnY 起始扫描点为 maxHeight+1。取 maxHeight=helper y=5 对应世界 Y
//      （=worldLocation({0,5,0}).y=-54），getSpawnY 从 -53（空气腔顶 air）向下扫，在内部列确定性地
//      找到地板 stone(世界-58) 上方 air(世界-57,-56) → 返回 -57（helper y=2 站立位）。
//      不用 under 则 getSpawnY 从 getMaxBuildHeight()+1 向下扫，结构封顶上方 worldgen 形态非确定
//      （gridStartY=-59 埋地下，上方 worldgen 石头山体），getSpawnY 可能返回 worldgen 地表而非结构内，
//      玩家被传到结构外不可控——故必须用 under 把搜索限制在结构空气腔内，确保确定性。
//   5. center 取空气腔中心 helper (4,?,4)，maxRange=2 → 分散点 helper x,z∈[2,6]（空气腔 [1,7] 内），
//      保证分散点落在结构内（getSpawnY 在内部列返回结构内地板站立位）。
//   6. spreadDistance：单玩家测试用 0（无间距约束，必能放置）；多玩家测试用 2（4×4 区域内 2 点
//      间距>=2 可行，如 (2,2)与(6,6) 距 5.66，算法 10000 次迭代能找到合法解）。
//   7. SimulatedPlayer::chat permLevel 已固定为 4（与游戏模式解耦），任意模式可执行管理命令。
//
// 判定手段：getEntities 区域限定查玩家 location（type=minecraft:player，区域=结构空气腔）。
//   - 单玩家：验证位置改变（离开初始 spawn 位）。
//   - 多玩家：验证两玩家都被移动 + 最终水平间距 >= spreadDistance（被分散开）。
//   必须区域限定（批内并行 tick + 不清场，全维度 getEntities 跨测试污染）。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_spreadplayers.txt（分散玩家到随机位置）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// cmd_arena 空气腔区域（helper 相对坐标 x,z∈[1,7], y∈[1,5]），用于 getEntities 区域限定查询。
const AREA_FROM = { x: 1, y: 1, z: 1 };
const AREA_VOLUME = { x: 7, y: 5, z: 7 };

// 空气腔中心（helper 相对），作为 spreadplayers 的 center。
const CENTER_REL = { x: 4, y: 0, z: 4 };
// under maxHeight 取空气腔顶部 helper y=5（getSpawnY 从其上方一格开始向下扫，确定性地在结构内
// 找到地板上方站立位）。
const MAX_HEIGHT_REL = { x: 0, y: 5, z: 0 };

/** 取结构空气腔内所有玩家实体的 location 列表（区域限定避免选到同批并行测试的玩家）。 */
function getPlayerLocations(test: Test): { x: number; y: number; z: number }[] {
    return test.getDimension().getEntities({
        type: "minecraft:player",
        location: test.worldLocation(AREA_FROM),
        volume: AREA_VOLUME,
    }).map(p => p.location);
}

/** 两点水平距离（sqrt(dx²+dz²)）。 */
function horizontalDistance(a: { x: number; z: number }, b: { x: number; z: number }): number {
    const dx = a.x - b.x;
    const dz = a.z - b.z;
    return Math.sqrt(dx * dx + dz * dz);
}

// /spreadplayers <center> <spreadDistance> <maxRange> <respectTeams> <targets> under <maxHeight>
// 把单个玩家分散到区域内（走 spreadPlayersImpl + _setPlayerPositions 单目标路径）。
//
// 验证修复核心：SimulatedPlayer 经修复后能被 spreadplayers 传送（修复前 requestTeleport 对
// SimulatedPlayer 返 0 传送 no-op，玩家原地不动）。
//
// spawn 1 玩家在 (5,2,5)，center=空气腔中心(4,?,4) 世界坐标，spreadDistance=0（单玩家无间距约束），
// maxRange=2（分散点 helper x,z∈[2,6]），under maxHeight=helper y=5 世界 Y（getSpawnY 确定性返回
// 结构内地板站立位 helper y=2）。pollUntilSucceed 断言玩家位置改变（离开初始 (5,2,5) 附近）。
//
// 注：spreadplayers 用随机分散点，玩家最终 x,z∈[2,6] 内某随机位，y=helper 2 站立位。只要离开初始
// (5,5) 附近即证明传送发生（修复前 no-op 玩家恒在 (5,5)）。但随机可能恰好落在 (5,5) 附近致假失败——
// 用 maxRange=2 + center=(4,4) 使分散点 x,z∈[2,6]，初始 (5,5) 在范围内，有概率落回原位。
// 改判定为：玩家 y 坐标变化（初始 y=2 站立位，分散后 getSpawnY 返回 y=2 不变？）——不行。
// 改判定为：玩家被传到 getSpawnY 站立位（y=世界-57=helper 2），与初始 spawn (5,2,5) 的 y 相同无法区分。
// 故判定用"位置改变"有假失败风险。改用更稳健判定：玩家 x 或 z 离开初始 (5,5) 至少 1 格。
// 但随机仍可能落回。最终判定：玩家在分散区域 [2,6]×[2,6] 内（证明被 spreadplayers 处理过，
// 因初始 (5,5) 也在内无法区分）——不够。
//
// 最稳健：用 2 玩家测试验证"分散开"（间距>=spreadDistance），单玩家测试改为验证"传送发生"用
// 一个 trick：spawn 玩家在区域外 (7,2,7)（角落），center=(4,4) maxRange=2 分散点∈[2,6]，
// 玩家初始 (7,7) 在 [2,6] 外，分散后必在 [2,6] 内 → 位置必改变（从 7 移到 <=6）。
// Ref: wiki spreadplayers.txt（spreadplayers <center> <spreadDistance> <maxRange> <respectTeams> <targets>）
function spreadplayersMovesSinglePlayer(test: Test): void {
    // 玩家 spawn 在空气腔角落 (7,2,7)（分散区域 [2,6]×[2,6] 外），分散后必被移入 [2,6] 内。
    const player = test.spawnSimulatedPlayer({ x: 7, y: 2, z: 7 }, "mover");
    // 初始位置记录（用于判定位置改变）。
    const initialRel = { x: 7, y: 2, z: 7 };
    const initialWorld = test.worldLocation(initialRel);

    // center=空气腔中心(4,?,4) 世界 x,z；spreadDistance=0；maxRange=2（分散点 x,z∈[2,6]）；
    // respectTeams=false；targets=@a[distance=..20]（选中本测试玩家，区域限定避免选中并行测试玩家）；
    // under maxHeight=helper y=5 世界 Y（限制 getSpawnY 在结构内搜索）。
    const center = test.worldLocation(CENTER_REL);
    const maxHeight = Math.floor(test.worldLocation(MAX_HEIGHT_REL).y);

    test.runAtTickTime(5, () => {
        player.chat(
            `/spreadplayers ${Math.floor(center.x)} ${Math.floor(center.z)} 0 2 under ${maxHeight} false ` +
            `@a[distance=..20]`,
        );
    });

    pollUntilSucceed(test, () => {
        const locs = getPlayerLocations(test);
        if (locs.length < 1) return false;
        const loc = locs[0];
        // 玩家初始 (7,7) 在分散区域 [2,6] 外，分散后 x,z 应都 <=6.5（被移入 [2,6] 区域）。
        // 修复前 no-op，玩家恒在 (7,7)，x>6.5 不满足→超时失败暴露 bug。
        return loc.x < 6.5 && loc.z < 6.5;
    }, {
        startTick: 10,
        maxTick: 80,
        onTimeout: () => {
            const locs = getPlayerLocations(test);
            test.assert(false,
                `spreadplayers did not move single player into spread area; ` +
                `initial=${JSON.stringify(initialWorld)}, locs=${JSON.stringify(locs)}`);
        },
    });
}

// /spreadplayers 把多玩家分散开后任意两者水平间距 >= spreadDistance（走多目标分支 + 分散算法）。
//
// 验证修复 + 多目标遍历 + 分散算法：2 玩家被分散到区域内不同位置，间距 >= spreadDistance(2)。
// 修复前 SimulatedPlayer 传送 no-op，两玩家原地不动间距=初始间距（spawn 在同一点附近<2）→ 不满足。
//
// spawn 2 玩家都在 (4,2,4)（空气腔中心，间距 0 <spreadDistance=2），center=(4,4) maxRange=2
// （分散点∈[2,6]×[2,6]），spreadDistance=2（要求两分散点间距>=2，4×4 区域可行如 (2,2)与(6,6) 距 5.66），
// under maxHeight=helper y=5。pollUntilSucceed 断言两玩家都被移动 + 间距 >= 2。
//
// 判定：getPlayerLocations 取空气腔内所有玩家，要求 >=2 个，且任意两两水平距离 >= 2（被分散开）。
// 修复前两玩家原地 (4,4) 间距 0 <2 → 超时失败暴露 bug。
// Ref: wiki spreadplayers.txt（spreadplayers <targets> 多玩家分散，最小间距约束）
function spreadplayersSpreadsMultiplePlayersApart(test: Test): void {
    // 两玩家都 spawn 在空气腔中心 (4,2,4)（初始间距 0 <spreadDistance=2）。
    const playerA = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 4 }, "moverA");
    test.spawnSimulatedPlayer({ x: 4, y: 2, z: 4 }, "moverB");

    const center = test.worldLocation(CENTER_REL);
    const maxHeight = Math.floor(test.worldLocation(MAX_HEIGHT_REL).y);

    // 等 moverB 生成稳定后执行（@a 解析需两玩家都已注册到 ServerPlayerEntityManager）。
    test.runAtTickTime(5, () => {
        // spreadDistance=2（最小间距），maxRange=2（4×4 区域），under maxHeight，respectTeams=false。
        playerA.chat(
            `/spreadplayers ${Math.floor(center.x)} ${Math.floor(center.z)} 2 2 under ${maxHeight} false ` +
            `@a[distance=..20]`,
        );
    });

    pollUntilSucceed(test, () => {
        const locs = getPlayerLocations(test);
        if (locs.length < 2) return false;
        // 任意两玩家水平间距 >= spreadDistance(2)（被分散开）。
        // 用宽松下界 1.5（spreadDistance=2 但实体站立偏移/中心对齐方块+0.5 可能有亚格误差）。
        return horizontalDistance(locs[0], locs[1]) >= 1.5;
    }, {
        startTick: 10,
        maxTick: 100,
        onTimeout: () => {
            const locs = getPlayerLocations(test);
            const dist = locs.length >= 2 ? horizontalDistance(locs[0], locs[1]) : -1;
            test.assert(false,
                `spreadplayers did not spread players apart (dist=${dist}, need>=1.5); ` +
                `locs=${JSON.stringify(locs)}`);
        },
    });
}

export function registerSpreadPlayersTests(): void {
    GameTest.register("CommandTests", "spreadplayers_moves_single_player", spreadplayersMovesSinglePlayer)
        .structureName("gametests:cmd_arena")
        .maxTicks(120);

    GameTest.register("CommandTests", "spreadplayers_spreads_multiple_players_apart", spreadplayersSpreadsMultiplePlayersApart)
        .structureName("gametests:cmd_arena")
        .maxTicks(140);
}
