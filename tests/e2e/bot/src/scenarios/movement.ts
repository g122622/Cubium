/*
 * movement.ts — 场景 J：玩家移动（服务端是否受理并跟随）。
 *
 * 这一层此前完全没有覆盖：所有既有用例的 bot 都停在出生点，从不移动。而移动是最容易
 * "客户端自己演、服务端不认"的一段——客户端本地积分位置、服务端另有一份权威位置，
 * 两者不一致时服务端会下发位置纠正把玩家拽回去。判据正是这一点：
 *
 *   走完一段后**位置必须保持**（没有被拽回原处），且连接仍处于 play、仍能读方块。
 *
 * 第二个用例把移动距离拉开到跨区块的程度：服务端必须为新进入的区块流式推送数据，
 * 因此它同时是「移动触发的区块加载」的哨兵（在出生点附近是测不到的）。
 */

import type { CaseDefinition } from "../case.ts";
import { expectEq, expectTrue } from "../assert/expect.ts";
import { blockNameAt } from "../assert/surface.ts";
import { delay, waitForCondition } from "../bot/wait.ts";

/** 超平坦默认层（bedrock 1 / dirt 2 / grass_block 1），两侧应当一致。 */
const EXPECTED_LAYERS = ["grass_block", "dirt", "dirt", "bedrock"];

/** 按键按住的时间（毫秒）与停止后观察位置是否被纠正的时间。 */
const WALK_HOLD_MS = 1_200;
const SETTLE_AFTER_WALK_MS = 600;

/** 跨区块用例的目标水平距离（区块宽 16，故 20 格必然跨过一个区块边界）。 */
const CROSS_CHUNK_DISTANCE = 20;
const CROSS_CHUNK_TIMEOUT_MS = 15_000;

/** 操作移动按键。 */
function setForward(bot: never, pressed: boolean): void {
    const typed = bot as unknown as {
        setControlState(control: string, state: boolean): void;
    };
    typed.setControlState("forward", pressed);
}

/** 玩家当前位置的水平坐标。 */
function horizontalPosition(bot: never): { x: number; z: number } {
    const typed = bot as unknown as { entity: { position: { x: number; z: number } } };
    return { x: typed.entity.position.x, z: typed.entity.position.z };
}

function horizontalDistance(a: { x: number; z: number }, b: { x: number; z: number }): number {
    return Math.hypot(a.x - b.x, a.z - b.z);
}

export const movementCases: readonly CaseDefinition[] = [
    {
        id: "movement/walk_moves_player_and_position_holds",
        title: "向前行走后位置保持（服务端未把玩家拽回原处）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: false,
        skipReason: null,
        async run({ bot }): Promise<Record<string, unknown>> {
            const start = horizontalPosition(bot as never);

            setForward(bot as never, true);
            await delay(WALK_HOLD_MS);
            setForward(bot as never, false);

            const afterWalk = horizontalPosition(bot as never);
            const movedDistance = horizontalDistance(start, afterWalk);
            expectTrue(
                movedDistance >= 0.5,
                `按住前进 ${WALK_HOLD_MS}ms 后只移动了 ${movedDistance.toFixed(2)} 格（客户端物理未推进）`,
            );

            // 停止后再观察一段：服务端若拒绝这次移动，会用位置纠正把玩家送回原处。
            await delay(SETTLE_AFTER_WALK_MS);
            const settled = horizontalPosition(bot as never);
            const distanceFromStart = horizontalDistance(start, settled);
            expectTrue(
                distanceFromStart >= 0.5,
                `停止后玩家被拉回起点附近（距起点 ${distanceFromStart.toFixed(2)} 格）——服务端不接受该移动`,
            );

            const clientState = (bot._client as unknown as { state?: string }).state;
            expectEq(clientState, "play", "行走后客户端应仍处于 play 阶段");
            return {
                movedHorizontally: true,
                positionHeldAfterStop: true,
            };
        },
    },

    {
        id: "movement/walking_loads_new_chunks",
        title: "走到 20 格外仍能读方块（移动触发的区块流式加载）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: false,
        skipReason: null,
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            const start = horizontalPosition(bot as never);
            setForward(bot as never, true);
            try {
                await waitForCondition(
                    () => horizontalDistance(start, horizontalPosition(bot as never)) >= CROSS_CHUNK_DISTANCE,
                    {
                        timeoutMs: CROSS_CHUNK_TIMEOUT_MS,
                        pollMs: 100,
                        what: `走到 ${CROSS_CHUNK_DISTANCE} 格外`,
                        describe: () =>
                            `当前移动了 ${horizontalDistance(start, horizontalPosition(bot as never)).toFixed(2)} 格（被地形或服务端阻挡？）`,
                    },
                );
            } finally {
                setForward(bot as never, false);
            }

            const arrived = horizontalPosition(bot as never);
            const blockX = Math.floor(arrived.x);
            const blockZ = Math.floor(arrived.z);
            // 契约：跨过区块边界后，新位置的列必须已送达且地形与出生地一致（同一超平坦配置）。
            await waitForCondition(() => blockNameAt(bot, blockX, surfaceY, blockZ) !== undefined, {
                timeoutMs: 15_000,
                pollMs: 100,
                what: "新位置所在列区块送达",
                describe: () => `(${blockX},${blockZ}) 仍未加载`,
            });
            const layers: string[] = [];
            for (let i = 0; i < EXPECTED_LAYERS.length; i += 1) {
                layers.push(blockNameAt(bot, blockX, surfaceY - i, blockZ) ?? "<未加载>");
            }
            // 跨区块后地形仍是同一套超平坦层——区块边界处不会出现生成伪影。
            expectEq(
                JSON.stringify(layers),
                JSON.stringify(EXPECTED_LAYERS),
                `跨区块后的列分层不符：${layers.join(",")}`,
            );

            return {
                crossedChunkBoundary:
                    Math.floor(spawnX / 16) !== Math.floor(blockX / 16) ||
                    Math.floor(spawnZ / 16) !== Math.floor(blockZ / 16),
                destinationColumnLoaded: true,
            };
        },
    },
];
