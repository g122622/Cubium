/*
 * index.ts — 用例注册表。
 *
 * 显式列出而非目录扫描：隐式发现会让「用例被误删/改名」这类问题静默通过，
 * 而显式注册表能让基线比对立刻发现条目缺失。
 */

import type { CaseDefinition } from "../case.ts";
import { handshakeCases } from "./handshake.ts";
import { chunkSyncCases } from "./chunk-sync.ts";
import { blockInteractionCases } from "./block-interaction.ts";
import { inventoryCases } from "./inventory.ts";
import { craftingCases } from "./crafting.ts";
import { furnaceCases } from "./furnace.ts";
import { containerCases } from "./containers.ts";
import { persistenceCases } from "./persistence.ts";
import { entityCases } from "./entities.ts";
import { movementCases } from "./movement.ts";
import { protocolCases } from "./protocol.ts";
import { statusEffectCases } from "./status-effects.ts";
import { chatCases } from "./chat.ts";
import { playerVisualCases } from "./player-visuals.ts";
import { blockEventCases } from "./block-events.ts";
import { playerLifecycleCases } from "./player-lifecycle.ts";

/** 全部用例。 */
export const ALL_CASES: readonly CaseDefinition[] = [
    ...handshakeCases,
    ...chunkSyncCases,
    ...blockInteractionCases,
    ...inventoryCases,
    ...craftingCases,
    ...furnaceCases,
    ...containerCases,
    ...persistenceCases,
    ...entityCases,
    ...movementCases,
    ...protocolCases,
    ...statusEffectCases,
    ...chatCases,
    ...playerVisualCases,
    ...blockEventCases,
    ...playerLifecycleCases,
];
