// 末地传送门传送测试：验证玩家进入末地传送门方块后立即跨维度传送到末地出生点 (100,49,0)。
//
// wiki 机制（world_末地传送门.txt）：末地传送门是立即传送（无 80tick 等待，区别于下界传送门），
// 主世界→末地传送到固定出生点 (100,49,0) 并生成黑曜石平台。Cubium 实现：EndPortalBlock::
// onEntityCollision 调 entity.changeDimension(THE_END)（虚派发），ServerPlayer::changeDimension 内
// Teleporter::getEndSpawnPosition() 取固定 (100,49,0) + EndTeleporter::createEndSpawnPlatform 建平台，
// 然后迁移 EntityManager 到末地。
//
// 放置技巧：end_portal 无 state 属性，setBlockWithStates 强放（flags=3 不调 isValidPosition）。
// end_portal 方块无 isValidPosition 限制（不同于 nether_portal），但用 setBlockWithStates 保持一致。
//
// 跨服务端：基岩 BDS GameTest 跨维度传送受限，本用例 Cubium one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_末地传送门.txt

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PORTAL_POS = { x: 3, y: 1, z: 3 };

// 末地固定出生点（vanilla EndSpawnPoint）：changeDimension 内 getEndSpawnPosition() 返回值。
const END_SPAWN_X = 100;
const END_SPAWN_Z = 0;
const END_SPAWN_TOLERANCE = 5; // 容差：平台生成位置可能有微调

function endPortalTransfersPlayerToEnd(test: Test): void {
    // 强放 end_portal（无 state 属性，statesStr 空串）。
    test.setBlockWithStates("minecraft:end_portal", PORTAL_POS, "");

    // spawn SimulatedPlayer 于门方块内，onEntityCollision 立即触发 changeDimension(THE_END)。
    test.spawnSimulatedPlayer(PORTAL_POS, "traveler");

    // 主断言：主世界 chamber 无玩家。辅助断言：末地有玩家且在 (100,49,0) 附近。
    test.succeedWhen(() => {
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPlayers.length !== 0) {
            return; // 主世界仍有玩家，未传送完成
        }
        const end = world.getDimension("minecraft:the_end");
        const endPlayers = end.getEntities({ type: "minecraft:player" });
        test.assert(endPlayers.length > 0, "player not found in the_end after end portal teleport");
        const p = endPlayers[0];
        test.assert(
            Math.abs(p.location.x - END_SPAWN_X) < END_SPAWN_TOLERANCE &&
                Math.abs(p.location.z - END_SPAWN_Z) < END_SPAWN_TOLERANCE,
            `player end position ${p.location.x},${p.location.z} not near (100,0)`,
        );
    });
}

export function registerEndPortalTests(): void {
    GameTest.register("TeleportTests", "end_portal_transfers_player_to_end", endPortalTransfersPlayerToEnd)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
