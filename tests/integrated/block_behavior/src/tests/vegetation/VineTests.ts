// 藤蔓墙面附着支撑自毁行为 GameTest（移除所附固体方块时藤蔓自毁）。
//
// wiki tech_藤蔓.txt（:46）："藤蔓会在其附着的方块更新后被破坏消失，除非其为附着在侧面的藤蔓且
// 上方有其他藤蔓相连。" 即藤蔓所附着的固体方块被移除（更新），且藤蔓无其他方向连接、上方无藤蔓
// 相连时，藤蔓自毁。
//
// C++ 链路：VineBlock::updatePostPlacement（VineBlock.cpp:137-189）当某方向 facing 的连接为 true
// 但 _canAttachTo(world, currentPos, facing) 失败（邻位非 isSolid）时：
//   1. 移除该方向连接（newState = state.with(prop, false)）。
//   2. 若移除后 _getConnectionCount==0 且 isValidPosition(newState) 失败（无任一方向可附着 + 下方
//      非同类藤蔓）→ 返回 air 自毁。
// _canAttachTo（:462-473）：邻位方块 isSolid() → true 可附着。
// isValidPosition（:111-135）：任一方向 state 连接 _canAttachTo 成立，或下方是同类藤蔓。
//
// 测试场景：放 east=true 藤蔓附在 East 邻位固体方块上，移除该固体方块 → 藤蔓 east 连接失效移除 →
// 全连接清零 + 无下方藤蔓 → 自毁变 air。
//
// 关键约束（同 SnowTests/SugarCaneTests 支撑自毁范式）：
// 1. setBlockType 走 _resolveBlock 取 defaultState（藤蔓连接全 false），不经 getStateForPlacement。
//    故需用 setBlockWithStates 显式放 east=true 藤蔓（VineBlock::updatePostPlacement 只移除连接、
//    不添加连接，setBlockType 默认全 false 放下后不会自动建立 east 连接）。
// 2. 放置藤蔓（east=true）时 (4,1,1) 邻位须已是固体方块（石头 isSolid=true），否则藤蔓被放下后
//    east 连接虽强设 true，但需先放石头再放藤蔓，避免藤蔓放置瞬间 east 无附着（实际放置不向自身
//    派发 updatePostPlacement，east=true 会被保留；但为贴近 vanilla 放置语义"附着在固体侧面"，
//    先放石头再放藤蔓更稳妥）。
// 3. 移除石头（→air）必须是非 no-op 写入以派发更新：石头→air 真实变化，向 West 邻位藤蔓派发
//    updatePostPlacement(East) → _canAttachTo(East)=air isSolid false → 移除 east → 全连接清零 →
//    isValidPosition 失败 → 返回 air，藤蔓自毁。
//
// 不测藤蔓 randomTick 蔓延（:54-66）：概率性 + 9x9x3 密度判定，非确定，按准则跳过。
// 不测「上方有藤蔓相连时侧面藤蔓不自毁」（:46 例外）：需纵向多格藤蔓链，且 isValidPosition 下方
// 同类藤蔓分支即可覆盖"下方有藤蔓不自毁"，但放置多格藤蔓 + 移除中间格的链路复杂，本文件聚焦核心
// 「所附固体被移除 → 自毁」行为点。TODO: 可补 vine_survives_when_vine_below 测试覆盖下方藤蔓保活分支。
//
// 跨服务端：藤蔓 north/east/south/west/up bool state 名两端一致（Java 式），自毁行为与 vanilla 一致
// （所附固体移除且无其他连接即自毁，同步），可跨服务端对比。getState("east") 用 as any 绕过
// BlockStateSuperset 白名单（同栅栏/树叶范式）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_藤蔓.txt（附着方块更新后破坏消失）
// Ref: VineBlock.cpp（updatePostPlacement 连接失效自毁 / _canAttachTo / isValidPosition）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 移除藤蔓所附着的 East 邻位固体方块时藤蔓自毁变 air。
//
// 布局：(4,1,1) 放 stone（East 邻位固体方块，isSolid true，作藤蔓 east 附着面），(3,1,1) 用
// setBlockWithStates 放 east=true 藤蔓（附着 East 邻位石头）。再 (4,1,1) 设 air 移除石头。
// air 放置向 West 邻位藤蔓派发 updatePostPlacement(East) → _canAttachTo(East)=air isSolid false →
// 移除 east → 全连接清零 → isValidPosition 失败（无下方藤蔓）→ 返回 air，藤蔓自毁。
//
// 判定：succeedWhenBlockPresent 断言藤蔓格 (3,1,1) 藤蔓消失（同 tick 同步）。
function vineBreaksWhenAttachedBlockRemoved(test: Test): void {
    // (4,1,1) 放 stone（East 邻位固体方块，isSolid=true，作藤蔓 east 附着面）。先放石头保证藤蔓
    // 放置时 east 连接有附着（贴近 vanilla 放置语义）。
    test.setBlockType("minecraft:stone", { x: 4, y: 1, z: 1 });

    // (3,1,1) 用 setBlockWithStates 放 east=true 藤蔓（附着 East 邻位石头）。setBlockType 只放
    // defaultState（连接全 false），需 setBlockWithStates 显式设 east=true。藤蔓放置不向自身派发
    // updatePostPlacement，east=true 被保留（_canAttachTo(East)=石头 isSolid true 亦成立）。
    test.setBlockWithStates("minecraft:vine", { x: 3, y: 1, z: 1 }, "east=true");

    // (4,1,1) 设 air 移除石头（stone→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向 West
    // 邻位藤蔓派发 updatePostPlacement(East) → _canAttachTo(East)=air isSolid false → 移除 east →
    // 全连接清零 → isValidPosition 失败 → 返回 air。
    test.setBlockType("minecraft:air", { x: 4, y: 1, z: 1 });

    // 断言藤蔓格 (3,1,1) 藤蔓已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:vine", { x: 3, y: 1, z: 1 }, false);
}

export function registerVineTests(): void {
    GameTest.register("BlockBehaviorTests", "vine_breaks_when_attached_block_removed", vineBreaksWhenAttachedBlockRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
