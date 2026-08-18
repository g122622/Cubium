// 脚本传送测试：验证 Entity.teleport / tryTeleport API 的同维度与跨维度传送。
//
// 基岩 API（@minecraft/server Entity）：teleport(location: Vector3, teleportOptions?: TeleportOptions): void
// tryTeleport(location: Vector3, teleportOptions?: TeleportOptions): boolean。TeleportOptions.dimension
// 指定目标维度则跨维度传送。Cubium 绑定（MinecraftModuleFactory.cpp Entity.teleport/tryTeleport）：
// 同维度走 attemptTeleport（带碰撞检测），跨维度走虚派发 changeDimension。
//
// 跨维度 location 处理：changeDimension 当前签名 (DimensionId) 不接受位置（位置由 Teleporter 计算：
// 末地固定 (100,49,0)，下界 1:8 缩放），故跨维度时 location 参数被忽略。与基岩 teleport 跨维度支持
// location 有差异（TODO，见绑定注释）。测试用"目标维度有玩家"断言规避位置不精确。
//
// 跨服务端：基岩 BDS 支持 Entity.teleport 跨维度（官方 API），但 GameTest 场景下基岩是否加载下界
// 维度未知。scriptTeleportCrossDimension 倾向 Cubium one-sided；sameDimensionTeleportBaseline 两端可测。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界传送门.txt#传送（坐标缩放机制）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 用例3：脚本 Entity.teleport 跨维度（主世界→下界）。
function scriptTeleportCrossDimension(test: Test): void {
    const player = test.spawnSimulatedPlayer({ x: 2, y: 1, z: 2 }, "traveler");

    // 延迟 5 tick 后跨维度传送（给 spawn/物理稳定时间）。
    // location 跨维度时被忽略（位置由 changeDimension 内 Teleporter 算），传 {0,64,0} 占位。
    test.runAtTickTime(5, () => {
        const nether = world.getDimension("minecraft:nether");
        // node_modules 中 @minecraft/server 存在两份（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
        // Dimension 类型分裂（TeleportOptions.dimension 期望的 Dimension 带 placeFeature/placeFeatureRule，
        // world.getDimension 返回的 Dimension 无），致类型不兼容；运行时两者均为同一 Cubium Dimension
        // opaque，强转绕过编译期（对齐 CowTests 的 ItemStack 强转模式）。
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        player.teleport({ x: 0, y: 64, z: 0 }, { dimension: nether as unknown as any });
    });

    test.succeedWhen(() => {
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPlayers.length !== 0) {
            return; // 主世界仍有玩家，未传送完成
        }
        const nether = world.getDimension("minecraft:nether");
        const netherPlayers = nether.getEntities({ type: "minecraft:player" });
        test.assert(netherPlayers.length > 0, "player not found in nether after script teleport");
    });
}

// 用例5：同维度 teleport 基准（确保同维度传送不回归，对比跨维度用例）。
//
// 基岩 Entity.teleport(location) 的 location 是世界绝对坐标（非结构相对坐标），必须经
// test.worldLocation() 转换。attemptTeleport 内部还会向下找固体地面（glass_pit y=0 grass
// 满足），adjustedY 落到 y=1。断言用世界坐标体积查询 + 容差。
function sameDimensionTeleportBaseline(test: Test): void {
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "traveler");

    // 目标结构相对坐标 (4.5,1,4.5) → 世界绝对坐标后传 teleport。
    // 基岩 Entity.teleport(location) 的 location 是世界绝对坐标（非结构相对坐标），必须经
    // test.worldLocation() 转换。teleport 默认 checkForBlocks=false 强制传送（对齐基岩），故
    // 不经碰撞检测，目标 air 格直接生效。
    const targetWorld = test.worldLocation({ x: 4.5, y: 1, z: 4.5 });
    test.runAtTickTime(5, () => {
        player.teleport(targetWorld);
    });

    // 断言玩家出现在 (4.5,1,4.5) 附近（同维度强制传送位置精确）。
    // getEntities 的 location+volume 是世界坐标体积查询（location 为基准角，volume 为边长），
    // 用 (4,0,4)~(7,5,7) 包围 (4.5,1,4.5)。
    test.succeedWhen(() => {
        const players = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation({ x: 4, y: 0, z: 4 }),
            volume: { x: 3, y: 5, z: 3 },
        });
        test.assert(players.length > 0, "player not found near (4.5,1,4.5) after same-dimension teleport");
        const p = players[0];
        test.assert(
            Math.abs(p.location.x - targetWorld.x) < 1.5 && Math.abs(p.location.z - targetWorld.z) < 1.5,
            `player position ${p.location.x},${p.location.z} not near target ${targetWorld.x},${targetWorld.z}`,
        );
    });
}

export function registerScriptTeleportTests(): void {
    GameTest.register("TeleportTests", "script_teleport_cross_dimension", scriptTeleportCrossDimension)
        .structureName("gametests:glass_pit")
        .maxTicks(80);

    GameTest.register("TeleportTests", "same_dimension_teleport_baseline", sameDimensionTeleportBaseline)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
